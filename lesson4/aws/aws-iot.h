#ifndef AWS_IOT_H
#define AWS_IOT_H

#include <M5Stack.h>
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include "aws-config.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "callbacks.h"

class AWSIoTClass
{
    public:
        AWSIoTClass();
        void begin(TWINUPDATECALLBACK callback, uint8_t y = 70);
        boolean connect();
        void sendMessage(JsonObject json, boolean reported = false);
        void checkForMessage();
        void enableSending();
        void disableSending();        
        uint32_t getSendInterval();   
        void reportStatus();
        void desiredUpdate(byte *payload, unsigned int length);         
        void sendDesiredAccepted(String property, JsonVariant value);
        void sendDesiredRejected(String property);
        uint32_t getMsgCount();
        uint32_t getLastSent();
    
    private:
        String readFile(const char* filename);
        void setSendInterval(uint32_t interval);
        PubSubClient _mqttClient;
        boolean _connected;
        boolean _send_enabled;
        uint32_t _send_interval_ms;
        TWINUPDATECALLBACK _twinCallback;
        uint32_t _twin_update;
        uint32_t _control_update;
        uint32_t _msg_sent;
        uint32_t _msg_built;
        uint32_t _last_sent;
        uint8_t _y;
        String _ca_cert;
        String _device_cert;
        String _private_key;
};

extern AWSIoTClass AWSIoT;
#endif
