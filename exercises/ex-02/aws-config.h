#ifndef AWS_CONFIG_H
#define AWS_CONFIG_H

#include <Arduino.h>

// AWS Setup           
const String AWS_EP = "<custom endpoint>.amazonaws.com";    // AWS IoT Core Endpoint
const String AWS_THING_NAME = "";       // Device Name
const String AWS_CERT_ID = "";          // 10 Character Certificate ID from AWS
const String AWS_SHADOW_TOPIC = "$aws/things/" + AWS_THING_NAME + "/shadow/update";
const String AWS_SHADOW_DELTA_TOPIC = "$aws/things/" + AWS_THING_NAME + "/shadow/update/delta";
const String AWS_TOPIC = "dev-tel/" + AWS_THING_NAME;
const uint8_t AWS_RECONNECT_RETRIES = 20;  // How many times do we retry before giving up!
const uint16_t AWS_PORT = 8883;
const uint8_t AWS_QOS_LEVEL = 0;

const String AWS_CA_NAME = "/ca.pem";
const String AWS_DEVICE_CERTNAME = "/" + AWS_CERT_ID + "-certificate.pem.crt";
const String AWS_PRIVATE_CERTNAME = "/" + AWS_CERT_ID + "-private.pem.key";

#endif
