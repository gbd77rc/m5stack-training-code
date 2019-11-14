#include <M5Stack.h>
#include "sensors.h"
#include "wifi-connect.h"
#include "ntp-utility.h"
#include "ArduinoJson.h" // Json Library
#include "aws-iot.h"

const uint16_t BACKGROUND = PURPLE;
const uint8_t TRIGGER_PIN = 39;

// Setup the sensor instance to automatically read every 5 seconds
// and have manual update as well.
sensorsClass sensors(ENV_CELSIUS, TRIGGER_PIN, 130, 5000);

// LCD Wakeup/Sleep Variables
const uint8_t WAKEUP_PIN = 38;
uint64_t gone_sleep = 0;            // When we went to sleep
uint32_t go_to_sleep = 15000;       // How long before we go to sleep
boolean is_awake = true;            // Is currently asleep
boolean send_state = false;         // As WiFi uses a timer interrupt we cannot send state update on button press.


// Initialise Global Variables
const uint16_t MAX_MSG_SIZE = 512; // This is the MQTT package size
String room = String("Kitchen");   // Where the device is located
String deviceState = String("On"); // Is the device current on/off - Rejection
boolean isConnected = false;       // Is currently connected to AWS

// Wake up the LCD
void wakeupCallback()
{
    changeLcdState(true);
}

// LCD goes to sleep
void changeLcdState(boolean wakeUp)
{
    Serial.println(F("LCD going to change state!"));
    if (wakeUp)
    {
        Serial.println(F("LCD Waking Up!"));
        M5.Lcd.wakeup();
        M5.Lcd.setBrightness(100);
    }
    else
    {
        Serial.println(F("LCD Going To Sleep!"));
        M5.Lcd.sleep();
        M5.Lcd.setBrightness(0);
    }
    is_awake = wakeUp;
    gone_sleep = millis();
    send_state = true;
}

// Analysis the delta from AWS Shadow
void digitalTwinCallback(JsonObject payload)
{
    String newRoom = String();
    String newState = String();
    boolean newFlag = false;

    serializeJsonPretty(payload, Serial);
    for (JsonObject::iterator it = payload.begin(); it != payload.end(); ++it)
    {
        String property = String(it->key().c_str());
        if (property.equals("location"))
        {
            newRoom = it->value().as<JsonObject>()["room"].as<String>();
            Serial.printf("New Room is %s\r\n", newRoom.c_str());

            // Check if the room has changed or not and accept it if it has
            if (newRoom != room)
            {
                room = newRoom;
                // As room change will only happen via the desired state we don't need to clear it.
                AWSIoT.sendDesiredAccepted(property, it->value());
                displayRoom();
            }
        }
        else if (property.equals("device"))
        {
            // Always reject the device property update
            newState = it->value().as<String>();
            Serial.printf("Device State is %s\r\n", newState.c_str());
            AWSIoT.sendDesiredRejected(property);
            Serial.println(F("Resetting the State"));
        }
        else if (property.equals("lcd"))
        {
            // Always reject the device property update
            newFlag = it->value().as<boolean>();
            Serial.printf("LCD New State is %s\r\n", newFlag ? "on" : "off");
            if ( newFlag != is_awake )
            {
               changeLcdState(newFlag);
                // LCD state can be set via desired state and device so we need to clear it as the 
                // desired state could override the device state i.e. keep switching the LCD on when it just
                // gone to sleep.
                AWSIoT.sendDesiredAcceptedAndClear(property, it->value());
            } else {
                // Ok its the same status so lets just clear it/reject it.
                AWSIoT.sendDesiredRejected(property);
            }
            Serial.println(F("Setting LCD State"));
        }
        else
        {
            // If not known rejected for now
            AWSIoT.sendDesiredRejected(property);
            Serial.println(F("Rejecting LCD State"));
        }
    }
    AWSIoT.reportStatus();
}

// Display the current room setting
void displayRoom()
{
    M5.Lcd.setCursor(0, 140);
    M5.Lcd.printf("New Room      : %s                       ", room.c_str());
}

// Send LCD Status telemetry
void buildLcdAndSend()
{
    send_state = false;
    Serial.println(F("Sending LCD Status...."));
    StaticJsonDocument<MAX_MSG_SIZE> doc;
    JsonObject root = doc.to<JsonObject>();
    root["lcd"] = is_awake;

    AWSIoT.sendMessage(root, true);
    M5.Lcd.setCursor(0, 60);
    M5.Lcd.printf("Messages Build : %i\r\n", AWSIoT.getMsgCount());
}

// Build a telemetry message that can be sent out
void buildMessageAndSend()
{
    Serial.println(F("Sending Telemetry Status...."));
    StaticJsonDocument<MAX_MSG_SIZE> doc;
    JsonObject root = doc.to<JsonObject>();
    root["device"] = deviceState;

    JsonObject telemetry = root.createNestedObject("telemetry");
    telemetry.set(sensors.toJson());

    JsonObject location = root.createNestedObject("location");
    location["room"] = room;

    AWSIoT.sendMessage(root, true);
    M5.Lcd.setCursor(0, 60);
    M5.Lcd.printf("Messages Build : %i\r\n", AWSIoT.getMsgCount());
}

void setup()
{
    Serial.begin(115200);
    // Initialise the LCD screen
    M5.begin();
    M5.Lcd.clear(BACKGROUND);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(WHITE, BACKGROUND);

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
        sensors.begin();
        AWSIoT.begin(digitalTwinCallback);
        isConnected = AWSIoT.connect();
        AWSIoT.reportStatus();
        displayRoom();

        // Did we connect to AWS
        if (isConnected)
        {
            // Force the read of the sensors at start
            sensors.read();
            sensors.printStatus();
            buildMessageAndSend();
            buildLcdAndSend();
        }
    }
    pinMode(WAKEUP_PIN, INPUT);    
    attachInterrupt(digitalPinToInterrupt(WAKEUP_PIN), 
              wakeupCallback, RISING);    
}

void loop()
{
    // Check we are connected to the internet
    if (WifiConnection.isConnected())
    {
        NTPUtility.tick();
        M5.Lcd.setCursor(0, 50);
        M5.Lcd.printf("ISO  : %s\r\n", NTPUtility.getISO8601Formatted().c_str());

        if (((millis() - AWSIoT.getLastSent()) >= AWSIoT.getSendInterval()) && NTPUtility.getEpoch() > 1546300800 && isConnected)
        {
            buildMessageAndSend();
            AWSIoT.reportStatus();
        }
        if (isConnected)
        {
            AWSIoT.checkForMessage();
        }
        sensors.printStatus();
    }

    // Check when to go to asleep
    if (((millis() - gone_sleep) >= go_to_sleep) 
        && is_awake )
    {
        changeLcdState(false);
    }    

    if (send_state)
    {
       buildLcdAndSend();
    }

    delay(500);
}
