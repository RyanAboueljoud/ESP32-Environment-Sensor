// initWifi.h
#ifndef INIT_WIFI_H
#define INIT_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

#include "../errLeds/errLeds.h"

void initWifi(int LED, String hostname, String SSID, String SSID_PASSWD);

#endif // INIT_WIFI_H
