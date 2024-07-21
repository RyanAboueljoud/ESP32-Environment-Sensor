// mqtt.h
#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <bsec.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "../initWifi/initWifi.h"
#include "../errLeds/errLeds.h"

class MQTT : private WiFiClient
{
private:
    Preferences pref;
    PubSubClient client;
    Bsec iaqSensor;
    String mqtt_server, mqtt_user, mqtt_pass;
    int mqtt_port;
    const int TIMEOUT_MS = 10000; // 10 seconds
    const int LED_BUILTIN = 2;
    bool isBrokerOnline = false;

    void callback(const char *topic, byte *payload, unsigned int length);

public:
    void setup(PubSubClient newClient, String serverAddr, int port, String user, String pass, Bsec iaq)
    {
        mqtt_user = user;
        mqtt_pass = pass;
        mqtt_server = serverAddr;
        mqtt_port = port;
        client = newClient;
        client.setServer(mqtt_server.c_str(), mqtt_port);

        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;
        client.setCallback(std::bind(&MQTT::callback, this, _1, _2, _3));

        reconnectMQTT();

        iaqSensor = iaq;
    }

    String getHostname();
    void reconnectMQTT();
    void loop();
    bool isConnected();
    bool publish(const char *topic, const char *payload);
};

#endif // MQTT_H
