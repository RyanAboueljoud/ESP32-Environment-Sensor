// mqtt.h
#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <WiFi.h>
#include <nvs_flash.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include "../initWifi/initWifi.h"
#include "../errLeds/errLeds.h"

void reconnectMQTT(PubSubClient client, const String MQTT_USER, const String MQTT_PASS);
void callbackk(PubSubClient client, char *topic, byte *message, unsigned int length, int LED, const String SSID, const String SSID_PASSWD);
void callback(const char* topic, byte* payload, unsigned int length);

#endif // MQTT_H
