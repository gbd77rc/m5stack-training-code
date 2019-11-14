#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

#include <Arduino.h>
#include <WiFi.h>
#include "esp_wpa2.h"

typedef enum {
    Public,
    Enterprise
} ConnectType;

class wifiConnectClass
{
    public:
        void begin(const char* ssid, const char* ssid_pwd);
        void begin(const char* ssid, const char* user_name, const char* user_pwd);
        boolean connect();
        void printStatus();        
        boolean isConnected();
    private:
        void printHeader();
        boolean _connected;
        ConnectType _type;
        String _ssid;
        String _ssid_pwd;
        String _user;
        String _user_pwd;
};

extern wifiConnectClass WifiConnection;

#endif