#include "aws-iot.h"
#include "ntp-utility.h"
#include "SPIFFS.h"

// Internal WiFi Connection
WiFiClientSecure httpsClient;

// Pointer to class instance
AWSIoTClass *pointerToAWSClass;

// AWS Shadow Delta callback
void awsMqttCallback(char *topic, byte *payload, unsigned int length)
{
    Serial.printf("Callback happened: %s\r\n",topic);
    pointerToAWSClass->desiredUpdate(payload, length);
}

// Constructor
AWSIoTClass::AWSIoTClass()
    : _mqttClient(httpsClient), _send_interval_ms(30000), _send_enabled(true), _msg_built(0), _last_sent(0)
{
}

// Read the certificate from the Flash File System
String AWSIoTClass::readFile(const char* filename)
{
    Serial.printf("File being opened : %s\r\n", filename);
    File file = SPIFFS.open(filename);
    
    if(!file){
        Serial.println("Failed to open file for reading");
        return "";
    }
  
    while(file.available()){
        String debugLogData;
    
        while (file.available()){
          debugLogData += char(file.read());
        }
        file.close();
        return debugLogData;
  }
  file.close();
}

// Initialise the AWS instance
void AWSIoTClass::begin(TWINUPDATECALLBACK callback, uint8_t y)
{
    if(!SPIFFS.begin(true))
    {
      Serial.println("An Error has occurred while mounting SPIFFS");
    }   

    // Put the certs into variables that are going to stay around for the lifetime.
    this->_ca_cert = this->readFile(AWS_CA_NAME.c_str());
    this->_device_cert = this->readFile(AWS_DEVICE_CERTNAME.c_str());
    this->_private_key = this->readFile(AWS_PRIVATE_CERTNAME.c_str());

    // Setup the security certificates for TLS/SSL tunnel
    httpsClient.setCACert(this->_ca_cert.c_str());
    httpsClient.setCertificate(this->_device_cert.c_str());
    httpsClient.setPrivateKey(this->_private_key.c_str());

    // Setup the endpoint data and initialise callbacks
    this->_mqttClient.setServer(AWS_EP.c_str(), AWS_PORT);
    this->_mqttClient.setCallback(awsMqttCallback);
    pointerToAWSClass = this;
    this->_twinCallback = callback;
    this->_y = y;   
    Serial.println("Completed AWS Setup!");    
}

// Connect to AWS Endpoint
boolean AWSIoTClass::connect()
{
    this->_connected = false;
    uint8_t retries = 0;
    uint16_t y = M5.Lcd.getCursorY();
    M5.Lcd.printf("Connecting to AWS IoT (%s)", AWS_THING_NAME.c_str());
    while (!this->_mqttClient.connected() && retries < AWS_RECONNECT_RETRIES)
    {
        if (this->_mqttClient.connect(AWS_THING_NAME.c_str()))
        {
            boolean subbed = this->_mqttClient.subscribe(AWS_SHADOW_DELTA_TOPIC.c_str(), AWS_QOS_LEVEL);
            M5.Lcd.setCursor(0, y);
            M5.Lcd.printf("Connected to AWS IoT Core (%s)\r\n", AWS_THING_NAME.c_str());
            M5.Lcd.printf("Shadow Delta Subscribed: %s\r\n", subbed ? "True" : "False");
        }
        else
        {
            Serial.print("State :");
            Serial.println(this->_mqttClient.state());
            M5.Lcd.print(".");
            delay(100);
            retries++;
        }
    }

    if (!this->_mqttClient.connected())
    {
        M5.Lcd.println(" Timed out!");
        return false;
    }
    this->_connected = true;
    return true;
}

// Process the delta message for twin/shadow update from the cloud
void AWSIoTClass::desiredUpdate(byte *payload, unsigned int length)
{
    this->_twin_update++;
    DynamicJsonDocument doc(length+1);
    DeserializationError err = deserializeJson(doc, (char *)payload);
    boolean enabled;
    int interval;

    JsonObject root = doc["state"].as<JsonObject>();

    for (JsonObject::iterator it=root.begin(); it!=root.end(); ++it)  {
        DynamicJsonDocument doc(MQTT_MAX_PACKET_SIZE);
        String property = String(it->key().c_str());
        if(property.equals("send_enabled"))
        {
            enabled = it->value().as<boolean>();
            Serial.printf("Send Enabled is %s\r\n", enabled ? "True": "False");
            if (enabled != this->_send_enabled)
            {
                if (enabled)
                {
                    this->enableSending();
                } else {
                    this->disableSending();
                }
                
                this->sendDesiredAccepted("send_enabled", it->value());
            }
        }
        if (property.equals("send_interval"))
        {
            interval = it->value().as<int>();
            Serial.printf("Send Interval is %s\r\n", interval);
            if (interval != this->_send_interval_ms)
            {
                this->setSendInterval(interval);
                doc["send_interval"] = interval;
                this->sendDesiredAccepted("send_interval", it->value());                    
            }
        }
    }
    this->_twinCallback(root);
}


// Accept the desired property
void AWSIoTClass::sendDesiredAccepted(String property, JsonVariant value)
{
    String payload;
    Serial.println("Accepting the state");
    DynamicJsonDocument doc(MQTT_MAX_PACKET_SIZE);
    JsonObject state = doc.createNestedObject("state");
    JsonObject reported = state.createNestedObject("reported");
    reported[property] = value;
    serializeJson(doc, payload);
    serializeJsonPretty(doc, Serial);
    this->_mqttClient.publish(AWS_SHADOW_TOPIC.c_str(), payload.c_str());
}

void AWSIoTClass::sendDesiredAcceptedAndClear(String property, JsonVariant value)
{
    String payload;
    Serial.println("Accepting the state");
    DynamicJsonDocument doc(MQTT_MAX_PACKET_SIZE);
    JsonObject state = doc.createNestedObject("state");
    JsonObject reported = state.createNestedObject("reported");
    reported[property] = value;
    // Make sure the desired is cleared so not to return a delta.
    JsonObject desired = state.createNestedObject("desired");
    desired[property] = serialized("null");
    serializeJson(doc, payload);
    serializeJsonPretty(doc, Serial);
    this->_mqttClient.publish(AWS_SHADOW_TOPIC.c_str(), payload.c_str());
}

// Rejecte the desired property
void AWSIoTClass::sendDesiredRejected(String property)
{
    String payload;
    Serial.println("Rejecting the state");
    DynamicJsonDocument doc(MQTT_MAX_PACKET_SIZE);
    JsonObject state = doc.createNestedObject("state");
    JsonObject reported = state.createNestedObject("desired");
    reported[property] = serialized("null");
    serializeJson(doc, payload);
    serializeJsonPretty(doc, Serial);
    this->_mqttClient.publish(AWS_SHADOW_TOPIC.c_str(), payload.c_str());
}

// Get current message count
uint32_t AWSIoTClass::getMsgCount()
{
    return this->_msg_built;
}

// Get when last time message was sent
uint32_t AWSIoTClass::getLastSent()
{
    return this->_last_sent;
}

// Send the message to either standard topic or shadow
void AWSIoTClass::sendMessage(JsonObject json, boolean reported)
{
    String payload;
    boolean sent = false;
    if (this->_connected && this->_send_enabled)
    {
        _last_sent = millis();
        json["msg_number"] = ++_msg_built;
        json["timestamp"] = NTPUtility.getEpoch();
        Serial.printf("Publish to %s\r\n", reported ? AWS_SHADOW_TOPIC.c_str() : AWS_TOPIC.c_str());
        if (reported)
        {
            json["send_enabled"] = this->_send_enabled;
            json["send_interval"] = this->_send_interval_ms;
            DynamicJsonDocument doc(MQTT_MAX_PACKET_SIZE);
            JsonObject state = doc.createNestedObject("state");
            JsonObject reported = state.createNestedObject("reported");
            reported.set(json);
            Serial.println("------ Send Shadow Data -----");
            serializeJsonPretty(doc, Serial);
            Serial.println();
            serializeJson(doc, payload);
            sent = this->_mqttClient.publish(AWS_SHADOW_TOPIC.c_str(), payload.c_str());
            Serial.printf("\r\nJSON Size : %u\r\n", measureJson(doc));
        }
        else
        {
            Serial.println("------ Send Telemetry Data -----");
            serializeJsonPretty(json, Serial);
            Serial.println();
            serializeJson(json, payload);
            sent = this->_mqttClient.publish(AWS_TOPIC.c_str(), payload.c_str());
            Serial.printf("\r\nJSON Size : %u\r\n", measureJson(json));
        }
        this->_msg_sent++;
    }
    Serial.printf("Current sent status is %s\r\n", sent ? "True": "False");
}

void AWSIoTClass::enableSending()
{
    this->_control_update++;
    this->_send_enabled = true;
}

void AWSIoTClass::disableSending()
{
    this->_control_update++;
    this->_send_enabled = false;
}

void AWSIoTClass::checkForMessage()
{
    this->_mqttClient.loop();
}

uint32_t AWSIoTClass::getSendInterval()
{
    return this->_send_interval_ms;
}

void AWSIoTClass::setSendInterval(uint32_t interval)
{
    this->_control_update++;
    this->_send_interval_ms = interval;
}

void AWSIoTClass::reportStatus()
{
    M5.Lcd.setCursor(0, this->_y + 10);
    M5.Lcd.printf("Messages Sent    : %i\r\n", this->_msg_sent);
    M5.Lcd.printf("Control Messages : %i\r\n", this->_control_update);
    M5.Lcd.printf("Shadow Updates   : %i\r\n", this->_twin_update);
    M5.Lcd.printf("Sending Enabled  : %s\r\n", this->_send_enabled ? "True " : "False");
    M5.Lcd.printf("Send Interval    : %d seconds\r\n", this->_send_interval_ms / 1000);
}

AWSIoTClass AWSIoT;
