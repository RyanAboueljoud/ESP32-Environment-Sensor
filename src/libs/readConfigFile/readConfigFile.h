// readConfigFile.h
#ifndef INIT_READ_CONFIG_FILE_H
#define INIT_READ_CONFIG_FILE_H

#include <Arduino.h>
#include <SPIFFS.h>

bool readConfigFile(String &wifiSsid, String &wifiPass, String &mqttServer, int &mqttPort, String &mqttUser, String &mqttPass, String &influxdbServer, String &influxdbPort, String &influxdbUser, String &influxdbPass);

#endif // INIT_READ_CONFIG_FILE_H
