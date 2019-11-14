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

// Initialise Global Variables
const uint16_t MAX_MSG_SIZE = 512; // This is the MQTT package size
String room = String("Kitchen");   // Where the device is located
String deviceState = String("On"); // Is the device current on/off - Rejection
boolean isConnected = false;       // Is currently connected to AWS

// Analysis the delta from AWS Shadow
void digitalTwinCallback(JsonObject payload)
{
    String newRoom = String();
    String newState = String();

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
            Serial.println("Resetting the State");
        }
        else
        {
            // If not known rejected for now
            AWSIoT.sendDesiredRejected(property);
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

// Build a telemetry message that can be sent out
void buildMessageAndSend()
{
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
        }
    }
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
    }

    delay(500);
}
