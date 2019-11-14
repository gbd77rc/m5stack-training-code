#include <M5Stack.h>
#include "sensors.h"
#include "wifi-connect.h"
#include "ntp-utility.h"
#include "ArduinoJson.h" // Json Library

uint16_t BACKGROUND = BLUE;
const uint8_t WAKEUP_PIN = 38;
const uint8_t TRIGGER_PIN = 39;

// Setup the sensor instance to automatically read every 5 seconds 
// and have manual update as well.
sensorsClass sensors(ENV_CELSIUS, TRIGGER_PIN, 130, 5000);

// Initialise Global Variables
uint64_t gone_sleep = 0;            // When we went to sleep
uint32_t go_to_sleep = 15000;       // How long before we go to sleep
boolean is_asleep = false;          // Is currently asleep
uint32_t send_interval_ms = 10000;  // 10 Second interval 
uint64_t last_sent = 0;             // Last time sent (
const uint16_t MAX_MSG_SIZE = 512;  // This is the MQTT package size
uint32_t msg_count = 0;             // How many messages built so far

// Wake up the LCD
void wakeupCallback()
{
    Serial.println("LCD is waking up!");
    M5.Lcd.wakeup();
    M5.Lcd.setBrightness(100);
    is_asleep = false;
    gone_sleep = millis();
}

// Build a telemetry message that can be sent out
void buildMessage()
{
    msg_count++;
    StaticJsonDocument<MAX_MSG_SIZE> doc;
    JsonObject root = doc.to<JsonObject>();
    root["msgCount"] = msg_count;
    root["timestamp"] = NTPUtility.getEpoch();
    root["sendinterval"] = send_interval_ms;

    JsonObject telemetry = root.createNestedObject("telemetry");
    telemetry.set(sensors.toJson());
    
    last_sent = millis();
    serializeJsonPretty(root,Serial);
    Serial.printf("\r\nJSON Size : %u\r\n", measureJson(root));
}

void setup()
{
    Serial.begin(115200);
    // Initialise the LCD screen
    M5.begin();
    sensors.begin();
    M5.Lcd.clear(BACKGROUND);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(WHITE, BACKGROUND);
    pinMode(CHANGE_PIN, INPUT);    
    attachInterrupt(digitalPinToInterrupt(CHANGE_PIN), 
              changeBackground, RISING);   
    // Initialise the WiFi connection
    // There are two signatures for the begin method.  Hotspot with 2 parameters and Enterprise with 3 parameters
    // Hotspot
    //    1st parameter is the SSID
    //    2nd parameter is the password or blank
    // Enterprise
    //    1st parameter is the SSID
    //    2nd parameter is the user identifier
    //    3rd parameter is the password/token
    
    WifiConnection.begin("", "");
    WifiConnection.connect();
    WifiConnection.printStatus();    
    if (WifiConnection.isConnected())
    {
        // Initialise the Network Time Protocol and sensors libraries
        NTPUtility.begin();
        sensors.read();
    }
    pinMode(WAKEUP_PIN, INPUT);    
    attachInterrupt(digitalPinToInterrupt(WAKEUP_PIN), 
              wakeupCallback, RISING);
}

void loop()
{
    // No point if we are not connected to the network.
    if (WifiConnection.isConnected())
    {
        // Need to check if the epoch time needs resyncing with the NTP service
        NTPUtility.tick();

        // Echo out the current time
        M5.Lcd.setCursor(0,70);
        M5.Lcd.printf("ISO  : %s\r\n", NTPUtility.getISO8601Formatted().c_str());
        
        // Check when to send out a message
        if (((millis() - last_sent) >= send_interval_ms)
            && NTPUtility.getEpoch() > 1546300800)
        {
            sensors.read();
            buildMessage();
            sensors.printStatus();
        }     
    }

    // Check when to go to asleep
    if (((millis() - gone_sleep) >= go_to_sleep) 
        && is_asleep == false )
    {
        Serial.println("LCD going to sleep!");
        M5.Lcd.sleep();
        M5.Lcd.setBrightness(0);
        gone_sleep = millis();
        is_asleep = true;
    }     
    // Delay for a second the loop
    delay(1000);
}
