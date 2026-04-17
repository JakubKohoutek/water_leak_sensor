#ifndef MQTT_H
#define MQTT_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

extern WiFiClient wifiClient;
extern PubSubClient mqttClient;

// Must be called once in setup() before any other mqtt function.
// `place` becomes part of the topic path and the HA friendly name.
void initiateMqtt (const char* place);

// Connects to the broker with LWT on the availability topic.
// Publishes "online" on success. Returns true on success.
bool connectMqtt ();

// Cleanly disconnect. Call before ESP.deepSleep().
void disconnectMqtt ();

// Publish Home Assistant MQTT auto-discovery for battery + leak entities.
void publishDiscovery ();

// Publish current battery voltage (retained).
void publishBattery (float voltage);

// Publish leak state — true = wet (ON), false = dry (OFF) (retained).
void publishLeak (bool wet);

#endif // MQTT_H
