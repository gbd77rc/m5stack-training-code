#include "sensors.h"
#include "ntp-utility.h"
#include <timer.h>

Adafruit_BMP280 bme;

sensorsClass *pointerToClass;  // Pointer to the class instance so that the ISR function can call it.
auto _timer = timer_create_default();

// ISR callback function based on interrupt PIN
static void manualISR(){
    Serial.println(F("Manual Trigger...."));
    pointerToClass->isrHandler();
}

// ISR callback function based on timer interrupt
static boolean timerFunc(void *){
    Serial.println(F("Auto Timer Trigger...."));
    pointerToClass->read();
    return true;
}

// id = the selected device on the Grove plugin.
sensorsClass::sensorsClass(ScaleType scaleType, uint8_t triggerPin, uint8_t y, uint16_t autoInterval, boolean testing, uint8_t id)
    :_scaleType(scaleType), _triggerPin(triggerPin), _y(y), _testOnly(testing), _id(id), _autoInterval(autoInterval), _callAuto(autoInterval > 0 ? true: false)
{
}

// Initialised the internal variables and setup the ISR functions
void sensorsClass::begin()
{
    Wire.begin();
    switch(this->_scaleType) {
        case ENV_CELSIUS:
            this->_symbol="C";
            break;
        case ENV_FAHRENHEIT:
            this->_symbol="F";
            break;
        case ENV_KELVIN:
            this->_symbol="K";
            break;
    };    
    this->_humidity = 0.0;
    this->_temperature = 0.0;
    this->_pressure = 0.0;
    pointerToClass = this;
    sensorsClass::_trigger_count = 0;
    sensorsClass::_can_read = false;
    
    pinMode(this->_triggerPin, INPUT);    
    attachInterrupt(digitalPinToInterrupt(this->_triggerPin), 
              manualISR, RISING);
    if (!bme.begin(0x76))
    {  
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
        while (1);
    }   
             
    if (this->_callAuto)
    {
        _timer.every(this->_autoInterval, timerFunc);
    }
}

// ISR function will call this function to signal that the read function can be called now.
void sensorsClass::isrHandler()
{
    this->_trigger_count++;
    this->_can_read = true;
}

// Signal that the read function can now be called now.
boolean sensorsClass::canRead()
{
    this->tick();
    return this->_can_read;
}

// Allow the automatic timer to update internal state and cause the trigger to fire if required
void sensorsClass::tick()
{
    Serial.println(F("Calling Tick.."));
    _timer.tick();
}

// Read the sensor if not in test mode.  
boolean sensorsClass::read()
{
    sensorsClass::_can_read = false;
    this->_lastreading = NTPUtility.getEpoch();
    if (this->_testOnly == false)
    {
        this->_temperature = this->readTemperature();
        this->_humidity = this->readHumidity();
        this->_pressure = bme.readPressure();
    }
    else 
    {
        this->_temperature = 23.3;
        this->_humidity = 45.5;
        this->_pressure = 10856.0;
        Serial.println(F("Testing Mode - Using Dummy Values!"));        
    }
    // check if we got something sensible.
    if (isnan(this->_temperature))
    {
        Serial.println(F("No temperature read!!!!"));
        return false;
    }
    return true;
}

// Printout the sensor information to the LCD.  The default background color is black but can be overridden.
void sensorsClass::printStatus()
{
    M5.Lcd.setCursor(0, this->_y);
    M5.Lcd.printf("Manual Triggered : %i\r\n", this->_trigger_count);
    M5.Lcd.setCursor(0, this->_y + 20);
    // make sure we have sensible information
    if (isnan(this->_temperature) == false)
    {
        Serial.println(F("Sensors Data...."));
        M5.Lcd.printf("Temperature : %2.1f%s    \r\nHumidity : %2.0f%%    \r\nPressure : %2.2f Pa    ", 
            this->_temperature, this->_symbol.c_str(), this->_humidity, this->_pressure);
    }
    else
    {
        Serial.println(F("Sensors Invalid...."));
        M5.Lcd.println("Temperature : Invalid Sensor Reading     ");
        M5.Lcd.println("");
        M5.Lcd.println("");
    }
}

// Get the temperature last read
float sensorsClass::getTemperature()
{
    return this->_temperature;
}

// Get the humidity last read
float sensorsClass::getHumidity()
{
    return this->_humidity;
}

// Get the pressure last read
float sensorsClass::getPressure()
{
    return this->_pressure;
}

// Get the temperature last read
uint64_t sensorsClass::getLastRead()
{
    return this->_lastreading;
}

// Get all data as JSON
JsonObject sensorsClass::toJson()
{
    DynamicJsonDocument doc(256);
    JsonObject root = doc.to<JsonObject>();
    root["temperature"] = this->_temperature;
    root["temp_symbol"] = this->_symbol.c_str();
    root["humidity"] = this->_humidity;
    root["pressure"] = this->_pressure;
    root["triggered"] = this->_trigger_count;
    root["last_read"] = this->_lastreading;
    return root;
}

// Read the sensor data from the 1 Wire
byte sensorsClass::readDevice()
{
    Wire.beginTransmission(this->_id);
    Wire.write(0);
    if (Wire.endTransmission()!=0) 
    {
        return 1;  
    }
    Wire.requestFrom(this->_id, (uint8_t)5);
    
    for (int i=0;i<5;i++) 
    {
        datos[i]=Wire.read();
    };
    // This requires ISR/Trigger/Timer shell out.
    delay(50);
    if (Wire.available()!=0) 
        return 2;
    if (datos[4]!=(datos[0]+datos[1]+datos[2]+datos[3])) 
        return 3;
    return 0;
}

// convert the data read from sensor to temperature
float sensorsClass::readTemperature()
{
    float resultado=0;
    byte error=this->readDevice();
    
    if (error!=0) 
        return (float)error/100;
    
    switch(this->_scaleType) 
    {
        case ENV_CELSIUS:
            resultado=(datos[2]+(float)datos[3]/10);
            break;
        case ENV_FAHRENHEIT:
            resultado=((datos[2]+(float)datos[3]/10)*1.8+32);
            break;
        case ENV_KELVIN:
            resultado=(datos[2]+(float)datos[3]/10)+273.15;
            break;
    };
    return resultado;
}

// convert the data read from sensor to humidity
float sensorsClass::readHumidity()
{
    float resultado;
    byte error=this->readDevice();
    if (error!=0) 
        return (float)error/100;
    resultado=(datos[0]+(float)datos[1]/10);
    return resultado;
}
