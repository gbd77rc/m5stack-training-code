#include <M5Stack.h>
#include "sensors.h"

const uint8_t TRIGGER_PIN = 39;  // Trigger on left button press

sensorsClass sensors(ENV_CELSIUS, TRIGGER_PIN, 0);  // 0 means how far down

void setup()
{
    Serial.begin(115200);
    // Initialise LCD and M5Stack libraries
    M5.begin();
    M5.Lcd.clear();  
    M5.Lcd.setTextSize(2);
    sensors.begin();
    // Read the sensor initially so we can show something on start.
    sensors.read(); // read the sensor
    sensors.printStatus(); 
    serializeJsonPretty(sensors.toJson(), Serial); // Will be explained in JSON Sketch   
}

void loop()
{  
    // Check if the manual trigger been done
    if (sensors.canRead())
    {
        sensors.read(); // read the sensor
        sensors.printStatus(); 
    }
    delay(500);
}
