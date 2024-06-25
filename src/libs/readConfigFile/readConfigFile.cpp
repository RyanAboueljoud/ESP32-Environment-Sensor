#include "readConfigFile.h"

const char *SPIFFS_CONFIG_PATH = "/.secrets.env"; // Path on the SPIFFS file system intended for SPI NOR flash devices on embedded targets.

bool readConfigFile(String &wifiSsid, String &wifiPass, String &mqttServer, int &mqttPort, String &mqttUser, String &mqttPass, String &influxdbServer, String &influxdbPort, String &influxdbUser, String &influxdbPass)
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return false;
    }

    File file = SPIFFS.open(SPIFFS_CONFIG_PATH, "r");
    if (!file)
    {
        Serial.println("Unable to open file.");
        return false;
    }

    while (file.available())
    {
        String line = file.readStringUntil('\n');
        int pos = line.indexOf('=');
        if (pos != -1)
        {
            String key = line.substring(0, pos);
            String value = line.substring(pos + 1);
            value.trim(); // Remove any trailing newline characters

            if (key == "WIFI_SSID")
            {
                wifiSsid = value;
            }
            else if (key == "WIFI_PASSWORD")
            {
                wifiPass = value;
            }
            else if (key == "MQTT_SERVER")
            {
                mqttServer = value;
            }
            else if (key == "MQTT_PORT")
            {
                mqttPort = value.toInt();
            }
            else if (key == "MQTT_USERNAME")
            {
                mqttUser = value;
            }
            else if (key == "MQTT_PASSWORD")
            {
                mqttPass = value;
            }
            else if (key == "INFLUXDB_SERVER")
            {
                influxdbServer = value;
            }
            else if (key == "INFLUXDB_PORT")
            {
                influxdbPort = value;
            }
            else if (key == "INFLUXDB_USER")
            {
                influxdbUser = value;
            }
            else if (key == "INFLUXDB_PASSWORD")
            {
                influxdbPass = value;
            }
        }
    }

    file.close();
    return true;
}