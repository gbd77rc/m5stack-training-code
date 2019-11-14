# Introduction
This repo contains the lesson examples and possible solutions to the exercises within the IoT Fundementals Training course.

## Requirements
The following device and sensors are used with these sketches.

* [M5Stack Core](https://m5stack.com/collections/m5-core/products/grey-development-core)
* [ENV Sensor](https://m5stack.com/products/mini-env-sensor-unit)
* [PIR Sensor](https://www.amazon.co.uk/gp/product/B07554HYVF/ref=ppx_yo_dt_b_asin_title_o01_s01?ie=UTF8&psc=1)

## Arduino Libraries Used
Latest version should be fine to use.

* Adafruit BMP280 Library
* Adafruit Unified Sensor
* arduino-timer
* ArduinoJson
* M5Stack
* MQTT
* NTPClient
* PubSubClient

> _*PubSubClient Library*_  
The packet size is too small, and needs to be increased.  Update `src/PubSubClient.h` file so that `#define MQTT_MAX_PACKET_SIZE` is set to `512`

## Lesson 2
The [main.ino](./lesson2/main/main.ino) sketch is used to prove that the M5Stack device can be programmed via the USB serial port and will serialize a JSON object to the serial monitor.

## Lesson 3
The [button.ino](./lesson3/button/button.ino) sketch is used to demostrate interrupts and the button press/release action.

The [pir.ino](./lesson3/pir/pir.ino) sketch is used to show the only difference between button and pir is just the pin number.

The [sensors.ino](./lesson3/sensors/sensors.ino) sketch is used to demostrate how to connect the ENV sensor and retrieve the temperature, humidity and pressure values from it. 

The [internet.ino](./lesson3/internet/internet.ino) sketch is used to demostrate how to connect the IoT to the internet and how to retrieve the EPOCH timestamp.

The [json.ino](./lesson3/json/json.ino) sketch is used to demostrate how build a JSON object and serialize it to the serial monitor every n seconds.

The [battery.ino](./lesson3/battery/battery.ino) sketch is used to demostrate how automatically sleep the LCD panel and wake it up on button press.

## Lesson 4
The [aws.ino](./lesson4/aws/aws.ino) sketch is used to demostrate how to use AWS IoT Core Shadow and standard topics.  It relies on the SPIFF uploader to copy the certificates from the `data` folder to the device.

The [azure.ino](./lesson4/azure/azure.ino) sketch is used to demostrate how to use Azure IoT Hub.  It is based on the example code supplied by [Azure IoT Device Workbench](https://marketplace.visualstudio.com/items?itemName=vsciot-vscode.vscode-iot-workbench) extension.

## Execises
The [ex-01.ino](./exercises/ex-01/ex-01.ino) sketch shows how the Lesson 3 exercise could have be done.

The [ex-02.ino](./exercises/ex-02/ex-02.ino) sketch shows how the main cloud exercise could have be done.  It also shows how to get around the bug that if the LCD goes to sleep when the desired state is still set to true.  For this one I had to add a new method to the AWS class called `sendDesiredAcceptedAndClear`.

