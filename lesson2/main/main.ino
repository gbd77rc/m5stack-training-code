#include <M5Stack.h>
#include <ArduinoJson.h>

// This sketch is to test if the device is connected to 
// the student computer and can be uploaded.
const size_t capacity = JSON_OBJECT_SIZE(3);
DynamicJsonDocument doc(capacity);
int count = 0;

// the setup routine runs once when M5Stack starts up
void setup()
{
    doc["count"] = 0;
    doc["uptime"] = 0;
    M5.begin();  // Initialise the M5 Library
    M5.Lcd.fillScreen(GREEN);  // Turn the LCD to Blue
}

// the loop routine runs over and over again forever
void loop()
{
   doc["count"] = ++count;  // Incrementatal counter
   doc["uptime"] = millis(); // How long the session been running in ms
   serializeJsonPretty(doc, Serial);  // Stream the JSON document to the Serial Monitor
   delay(5000);  // Delay the loop for 5 seconds
}
