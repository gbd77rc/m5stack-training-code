#include <M5Stack.h>
#include "sensors.h"
#include "wifi-connect.h"
#include "ntp-utility.h"

const uint8_t TRIGGER_PIN = 39;
const uint16_t BACKGROUND = BLUE;

// Setup the sensor instance to automatically read every 5 seconds 
// and have manual update as well.
sensorsClass sensors(ENV_CELSIUS, TRIGGER_PIN, 130, 5000);

void setup()
{
    Serial.begin(115200);
    // Initialise the LCD screen
    M5.begin();
    M5.Lcd.clear(BACKGROUND);
    M5.Lcd.setTextSize(2);
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
    }
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
        M5.Lcd.printf("Time: %s\r\n", NTPUtility.getISO8601Formatted().c_str());

        // Tell the sensors timer to increment.
        sensors.tick();
        sensors.printStatus();
    }
    // Go to sleep for a second
    delay(1000);
}
