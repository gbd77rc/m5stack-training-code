#ifndef SENSORS_H
#define SENSORS_H
#include <M5Stack.h>
#include "ArduinoJson.h"
#include <Wire.h> //The DHT12 uses 1 Wire comunication.
#include "Adafruit_Sensor.h"
#include <Adafruit_BMP280.h>

typedef enum {
    ENV_CELSIUS = 1,
    ENV_KELVIN = 2,
    ENV_FAHRENHEIT = 3
} ScaleType;

class sensorsClass
{
    public:
        //sensorsClass();
      sensorsClass(ScaleType scaleType,       // Which Temperature scale to use
                   uint8_t triggerPin,        // Which pin to assign for manual trigger
                   uint8_t y = 0,             // How far down the screen should the status be printed
                   uint16_t autoInterval = 0, // How many ms a the read function will be called automatically
                   boolean testing = false,   // Should static testing values be used (only require if temp sensor not connected)
                   uint8_t id = 0x5c );          // Identifier of the temp sensor when connected to the grove port.
      void begin();
      void tick();                          
      boolean canRead();
      boolean read();
      void printStatus();
      void isrHandler();
      float getTemperature();
      float getHumidity();
      float getPressure();
      uint64_t getLastRead();
      JsonObject toJson();
    private:
      uint8_t _id;
      byte datos[5];
      ScaleType _scaleType;
      String _symbol;
      uint8_t _triggerPin;
      uint8_t _y;
      volatile float _temperature;
      volatile float _humidity;
      volatile float _pressure;
      volatile unsigned long _trigger_count;
      volatile boolean _can_read;        
      boolean _testOnly;
      byte readDevice();
      boolean _callAuto;
      uint16_t _autoInterval;
      float readTemperature();
      float readHumidity(); 
      float readPressure();     
      uint64_t _lastreading;  
};

#endif
