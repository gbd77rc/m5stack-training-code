#include <M5Stack.h>
#include "wifi-connect.h"

#define EAP_ANONYMOUS_IDENTITY "anonymous@example.com"

const uint8_t disconnect = 30;

// Printout the current WiFi information
void wifiConnectClass::printHeader()
{
    M5.Lcd.setCursor(0,0);      
    M5.Lcd.printf("Access Point is %s\r\n", this->_ssid.c_str());
    M5.Lcd.printf("Using %s WiFi\r\n", (this->_type == Public ? F("Public") : F("Enterprise")));  
}

// Initialise the hotspot AP
void wifiConnectClass::begin(const char *ssid, const char *ssid_pwd)
{
    this->_ssid = String(ssid);
    this->_ssid_pwd = String(ssid_pwd);    
    WiFi.disconnect();
    WiFi.mode(WIFI_AP_STA);
    this->_type = Public;
    this->printHeader();
}

// Initialise the enterprise AP
void wifiConnectClass::begin(const char *ssid, const char *user_name, const char *user_pwd)
{
    this->_ssid = ssid;
    this->_user = user_name;
    this->_user_pwd = user_pwd;   
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_ANONYMOUS_IDENTITY, strlen(EAP_ANONYMOUS_IDENTITY)); 
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)this->_user.c_str(), this->_user.length());
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)this->_user_pwd.c_str(), this->_user_pwd.length());
    esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT(); //set config settings to default
    esp_wifi_sta_wpa2_ent_enable(&config); //set config settings to enable function
    this->_type = Enterprise;
    this->printHeader();
}

// Do the actual connection to the AP
boolean wifiConnectClass::connect()
{
    uint8_t loop = 0;
    this->_connected = false;
    switch(this->_type){
        case Public:
            WiFi.begin(this->_ssid.c_str(), this->_ssid_pwd.c_str());
            break;
        case Enterprise:
            WiFi.begin(this->_ssid.c_str()); //connect to wifi
            break;
    }
    uint8_t status = WiFi.status();
    while (status != WL_CONNECTED) {

        // Tried too many times so give up.
        if (loop > disconnect)
        {
            Serial.printf("Failure Status is : %i\r\n", status);
            return false;
        }
        // Give it time to sort itself out.
        delay(1000);
        status = WiFi.status();
        loop++;
    }
    this->_connected = true;
    return true;    
}

// Print out the current status of the WiFi connection the IPv4 address
void wifiConnectClass::printStatus()
{
    M5.Lcd.setCursor(0,0);    
    M5.Lcd.printf("Access Point is %s\r\n", this->_ssid.c_str());
    M5.Lcd.printf("Using %s WiFi\r\n", (this->_type == Public ? F("Public") : F("Enterprise")));      
    M5.Lcd.printf("WiFi is %s\r\n", this->_connected ? F("Connected") : F("NOT Connected"));
    if (this->_connected)
    {
        M5.Lcd.printf("IP address: %s\r\n", WiFi.localIP().toString().c_str());
    }    
}

// Are we connected to the AP or not
boolean wifiConnectClass::isConnected()
{
    return this->_connected;
}

wifiConnectClass WifiConnection;
