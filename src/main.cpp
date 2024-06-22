#include <Arduino.h>
#include <bsec.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <nvs_flash.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

#include "libs/initWifi/initWifi.h"
#include "libs/errLeds/errLeds.h"
// #include "libs/mqtt/mqtt.h" // TODO: Rewrite as a class

#define BME680_I2C_ADDR_LOW 0x77

const char *SPIFFS_SECRETS_PATH = "/.secrets.env"; // Path on the SPIFFS file system intended for SPI NOR flash devices on embedded targets.

// Wi-Fi Network Connection Configuration
String SSID;
String SSID_PASSWD;
String hostname = "esp32";

// MQTT Broker IP Address and Login
String MQTT_SERVER;
int MQTT_PORT = 1883;
String MQTT_USER;
String MQTT_PASS;
String INFLUXDB_SERVER;
String INFLUXDB_PORT;
String INFLUXDB_USER;
String INFLUXDB_PASS;

Preferences preferences;

// Helper functions declarations
void checkIaqSensorStatus(void);
void errLeds(void);

// Prepare MQTT Client Configurations
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
bool brokerIsOnline = false;

// Create an object of the class Bsec
Bsec iaqSensor;

String output;
int LED_BUILTIN = 2;

int const TIMEOUT_MS = 10000;

float celsiusToFahrenheit(float celsius) {
  return ((celsius * 1.8) + 32);
}

float calcAltitude(float pressure) {
  const float sea_level_pressure = 1010; // hPa (Auburn - 500m)
  pressure = pressure * 0.01; // Convert Pa to hPa
  return 44330 * (1.0 - pow(pressure / sea_level_pressure, 0.1903)); // Altitude in meters (https://github.com/adafruit/Adafruit_CircuitPython_BME680/)
}

bool readEnvFile(const char* filename, String& wifiSsid, String& wifiPass,String& mqttServer,int& mqttPort, String& mqttUser, String& mqttPass, String& influxdbServer, String& influxdbPort, String& influxdbUser, String& influxdbPass) {
    if (!SPIFFS.begin(true))
    {
      Serial.println("An Error has occurred while mounting SPIFFS");
      return false;
    }
    
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.println("Unable to open file.");
        return false;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        int pos = line.indexOf('=');
        if (pos != -1) {
            String key = line.substring(0, pos);
            String value = line.substring(pos + 1);
            value.trim();  // Remove any trailing newline characters

            if (key == "WIFI_SSID") {
                wifiSsid = value;
            } else if (key == "WIFI_PASSWORD") {
                wifiPass = value;
            } else if (key == "MQTT_SERVER") {
            mqttUser = value;
            } else if (key == "MQTT_PORT") {
                mqttUser = value.toInt();
            } else if (key == "MQTT_USERNAME") {
                mqttUser = value;
            } else if (key == "MQTT_PASSWORD") {
                mqttPass = value;
            } else if (key == "INFLUXDB_SERVER") {
                influxdbServer = value;
            } else if (key == "INFLUXDB_PORT") {
                influxdbPort = value;
            } else if (key == "INFLUXDB_USER") {
                influxdbUser = value;
            } else if (key == "INFLUXDB_PASS") {
                influxdbPass = value;
            }
        }
    }

    file.close();
    return true;
}

// TODO: Move all MQTT code to separate class file
void reconnectMQTT()
{
  int const MAX_RETRY = 3;
  int retryCount = 0;

  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.println("Attempting MQTT connection...");
    if (retryCount < MAX_RETRY)
    {
      // Attempt to connect
      if (client.connect(hostname.c_str(), MQTT_USER.c_str(), MQTT_PASS.c_str()))
      {
        Serial.println("connected");
        // Subscribe
        client.subscribe((hostname + "/get_sensor_values").c_str());
        client.subscribe((hostname + "/get_iaq").c_str());
        client.subscribe((hostname + "/get_gas").c_str());
        client.subscribe((hostname + "/get_pressure").c_str());
        client.subscribe((hostname + "/get_humidity").c_str());
        client.subscribe((hostname + "/get_temperature").c_str());
        client.subscribe((hostname + "/update_hostname").c_str());
        client.subscribe((hostname + "/restart").c_str());
        client.subscribe((hostname + "/reset").c_str());
        client.subscribe("all/get_hostname");
        client.subscribe("homeassistant/status");
      }
      else
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in " + String(TIMEOUT_MS / 1000) + " seconds");
        retryCount++;
        // Wait 10 seconds before retrying
        errLeds(2, TIMEOUT_MS); // FIX: remove "magic" number for LED pin when converting to a class
      }
    }
    else
    {
      Serial.println("Reached max retry limit (" + String(MAX_RETRY) + "): Skipping MQTT connection for now...");
      return;
    }
  }
}

void callback(char *topic, byte *message, unsigned int length)
{
  String messageTemp;
  String pubMessage;
  String hostTopic;
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");


  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == hostname + "/get_sensor_values")
  {
    if (messageTemp == "true")
    {
      Serial.println("Publishing current sensor values...");
      JsonDocument doc;
      // Add values to the JSON document - Max Keys: 10
      doc["iaq"] = iaqSensor.iaq;
      doc["staticIaq"] = iaqSensor.staticIaq;
      doc["iaqAccuracy"] = iaqSensor.iaqAccuracy;
      doc["stabStatus"] = iaqSensor.stabStatus;
      doc["gasResistance"] = iaqSensor.gasResistance;
      doc["co2Equivalent"] = iaqSensor.co2Equivalent;
      doc["breathVocEquivalent"] = iaqSensor.breathVocEquivalent;
      doc["temperature"] = iaqSensor.temperature;
      doc["pressure"] = iaqSensor.pressure;
      doc["humidity"] = iaqSensor.humidity;

      serializeJson(doc, pubMessage);

      hostTopic = hostname + "/sensor";

      client.publish(hostTopic.c_str(), pubMessage.c_str());
    }
  }

  if (String(topic) == hostname + "/get_iaq")
  {
    if (messageTemp == "true")
    {
      Serial.println("Publishing current IAQ sensor values...");
      JsonDocument doc;
      // Add values to the JSON document - Max Keys: 10
      doc["iaq"] = iaqSensor.iaq;
      doc["staticIaq"] = iaqSensor.staticIaq;
      doc["iaqAccuracy"] = iaqSensor.iaqAccuracy;
      doc["runInStatus"] = iaqSensor.runInStatus;

      serializeJson(doc, pubMessage);

      hostTopic = hostname + "/iaq";

      client.publish(hostTopic.c_str(), pubMessage.c_str());
    }
  }

  if (String(topic) == hostname + "/get_gas")
  {
    if (messageTemp == "true")
    {
      Serial.println("Publishing current gas sensor values...");
      JsonDocument doc;
      // Add values to the JSON document - Max Keys: 10
      doc["co2Equivalent"] = iaqSensor.co2Equivalent;
      doc["breathVocEquivalent"] = iaqSensor.breathVocEquivalent;
      doc["gasResistance"] = iaqSensor.gasResistance;
      doc["gasPercentage"] = iaqSensor.gasPercentage;


      serializeJson(doc, pubMessage);

      hostTopic = hostname + "/gas";

      client.publish(hostTopic.c_str(), pubMessage.c_str());
    }
  }

  if (String(topic) == hostname + "/get_temperature")
  {
    if (messageTemp == "true")
    {
      Serial.println("Publishing current gas sensor values...");
      JsonDocument doc;
      // Add values to the JSON document - Max Keys: 10
      doc["rawTemperature"] = iaqSensor.rawTemperature;
      doc["temperature_C"] = iaqSensor.temperature;
      doc["temperature_F"] = celsiusToFahrenheit(iaqSensor.temperature);

      serializeJson(doc, pubMessage);

      hostTopic = hostname + "/temperature";

      client.publish(hostTopic.c_str(), pubMessage.c_str());
    }
  }

  if (String(topic) == hostname + "/get_pressure")
  {
    if (messageTemp == "true")
    {
      Serial.println("Publishing current pressure sensor values...");
      JsonDocument doc;
      // Add values to the JSON document - Max Keys: 10
      doc["pressure"] = iaqSensor.pressure;
      doc["altitude"] = calcAltitude(iaqSensor.pressure);

      serializeJson(doc, pubMessage);

      hostTopic = hostname + "/pressure";

      client.publish(hostTopic.c_str(), pubMessage.c_str());
    }
  }

  if (String(topic) == hostname + "/get_humidity")
  {
    if (messageTemp == "true")
    {
      Serial.println("Publishing current humidity sensor values...");
      JsonDocument doc;
      // Add values to the JSON document - Max Keys: 10
      doc["rawHumidity"] = iaqSensor.rawHumidity;
      doc["humidity"] = iaqSensor.humidity;

      serializeJson(doc, pubMessage);

      hostTopic = hostname + "/humidity";

      client.publish(hostTopic.c_str(), pubMessage.c_str());
    }
  }

  // If a message is received on the topic [hostname]/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == hostname + "/update_hostname")
  {
    if (!messageTemp.isEmpty())
    {
      hostname = messageTemp;
      preferences.begin("esp32", false);
      preferences.putString("hostname", hostname);
      preferences.end();
      initWifi(LED_BUILTIN, hostname, SSID, SSID_PASSWD);
      delay(100);
      reconnectMQTT();
    }
  }

  // If a message is received on the topic [hostname]/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == hostname + "/restart")
  {
    if (messageTemp == "true")
    {
      Serial.println("Restarting device...");
      delay(1000);
      ESP.restart();
    }
  }

  if (String(topic) == hostname + "/reset")
  {
    if (messageTemp == "true")
    {
      preferences.begin("esp32", false);
      preferences.putBool("reset", true);
      preferences.end();
      delay(100);
      ESP.restart();
    }
  }

  // If a message is received on the topic all/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == "all/get_hostname")
  {
    client.publish("all/broadcast_hostname", hostname.c_str());
  }

  if (String(topic) == "homeassistant/status")
  {
    if (messageTemp == "online")
    {
    }
    if (messageTemp == "offline")
    {
      delay(10000);
    }
  }
}



void setup()
{
  /* Initializes the Serial communication */
  Serial.begin(115200);
  delay(1000);

  // Enable built-in led and set to HIGH during setup
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Open flash storage filesystem
  preferences.begin("esp32", false);

  if (preferences.isKey("reset"))
  {
    if (preferences.getBool("reset") == true)
    {

      preferences.end();

      Serial.println("Erasing the NVS partition...");
      delay(100);
      nvs_flash_erase(); // erase the NVS partition and...
      Serial.println("Initialize the NVS partition...");
      nvs_flash_init(); // initialize the NVS partition.
    }
  }

  // TODO: Keep secrets in a separate file on flash memory that can be read from
  readEnvFile(SPIFFS_SECRETS_PATH, SSID, SSID_PASSWD, MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASS, INFLUXDB_SERVER, INFLUXDB_PORT, INFLUXDB_USER, INFLUXDB_PASS);

  if (preferences.isKey("hostname"))
  {
    hostname = preferences.getString("hostname");
    Serial.println("hostname loaded from flash memory: " + hostname);
  }
  else
  {
    preferences.putString("hostname", hostname);
  }

  // Close flash storage filesystem
  preferences.end();

  initWifi(LED_BUILTIN, hostname, SSID, SSID_PASSWD);

  client.setServer(MQTT_SERVER.c_str(), MQTT_PORT);
  client.setCallback(callback);

  iaqSensor.begin(BME680_I2C_ADDR_LOW, Wire);
  output = "\nBSEC library version " + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix);
  Serial.println(output);
  checkIaqSensorStatus();

  bsec_virtual_sensor_t sensorList[13] = {
      BSEC_OUTPUT_IAQ,                   // Measurement assuming dynamic environment, Indoor-air-quality (0-500).
      BSEC_OUTPUT_STATIC_IAQ,            // Measurement assuming static environment.
      BSEC_OUTPUT_CO2_EQUIVALENT,        // Estimates a CO2-equivalent (CO2eq) concentration [ppm] in the environment. Calculated based on sIAQ, derived VOC measurements, lookup table.
      BSEC_OUTPUT_BREATH_VOC_EQUIVALENT, // (bVOCeq) Estimates the total VOC concentration [ppm] in the environemnt. Calculated based on sIAQ and lookup table.
      BSEC_OUTPUT_RAW_TEMPERATURE,
      BSEC_OUTPUT_RAW_PRESSURE,
      BSEC_OUTPUT_RAW_HUMIDITY,
      BSEC_OUTPUT_RAW_GAS,
      BSEC_OUTPUT_STABILIZATION_STATUS,                // Indicates if the sensor is undergoing initial stabilization during its first use after production.
      BSEC_OUTPUT_RUN_IN_STATUS,                       // Indicates when the sensor is ready after after switch-on.
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE, // Temperature which is compensated for internal cross-influences caused by the BME sensor.
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,    // Relative humidity which is compensated for internal cross-influences caused by the BME sensor.
      BSEC_OUTPUT_GAS_PERCENTAGE,                      // Percentage of filtered gas value
  };

  iaqSensor.updateSubscription(sensorList, 13, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();

  // Print the header
  output = "Timestamp [ms], IAQ, IAQ accuracy, Static IAQ, CO2 equivalent, breath VOC equivalent, raw temp[°C], pressure [hPa], raw relative humidity [%], gas [Ohm], Stab Status, run in status, comp temp[°C], comp humidity [%], gas percentage";
  Serial.println(output);
  digitalWrite(LED_BUILTIN, LOW);
}

     
void loop()
{
  unsigned long time_trigger = millis();
  digitalWrite(LED_BUILTIN, LOW);

  if (!WiFi.isConnected())
  {
    initWifi(LED_BUILTIN, hostname, String(SSID), String(SSID_PASSWD));
  }

  if (!client.connected())
  {
    reconnectMQTT();
  } else {
    client.loop();
  }

  if (iaqSensor.run())
  { // If new data is available
    output = "\n\nTimestamp [ms], IAQ, IAQ accuracy, Static IAQ, CO2 equivalent, breath VOC equivalent, raw temp[°C], pressure [hPa], raw relative humidity [%]";
    Serial.println(output);
    output = String(time_trigger);
    output += ", " + String(iaqSensor.iaq);
    output += ", " + String(iaqSensor.iaqAccuracy); // A value of 1 or more indicates that the sensor values are stable, providing a valid IAQ index, with the maximum accuracy value being 3.
    output += ", " + String(iaqSensor.staticIaq);
    output += ", " + String(iaqSensor.co2Equivalent);
    output += ", " + String(iaqSensor.breathVocEquivalent);
    output += ", " + String(iaqSensor.rawTemperature);
    output += ", " + String(iaqSensor.pressure);
    output += ", " + String(iaqSensor.rawHumidity);
    output += "\n\ngas [Ohm], Stab Status, run in status, comp temp[°C], comp humidity [%], gas percentage";
    output += ", \n" + String(iaqSensor.gasResistance);
    output += ", " + String(iaqSensor.stabStatus);
    output += ", " + String(iaqSensor.runInStatus);
    output += ", " + String(iaqSensor.temperature);
    output += ", " + String(iaqSensor.humidity);
    output += ", " + String(iaqSensor.gasPercentage);
    Serial.println(output);
  }
  else
  {
    checkIaqSensorStatus();
  }
}

// Helper function definitions
void checkIaqSensorStatus(void)
{
  if (iaqSensor.bsecStatus != BSEC_OK)
  {
    if (iaqSensor.bsecStatus < BSEC_OK)
    {
      output = "BSEC error code : " + String(iaqSensor.bsecStatus);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    }
    else
    {
      output = "BSEC warning code : " + String(iaqSensor.bsecStatus);
      Serial.println(output);
    }
  }

  if (iaqSensor.bme68xStatus != BME68X_OK)
  {
    if (iaqSensor.bme68xStatus < BME68X_OK)
    {
      output = "BME68X error code : " + String(iaqSensor.bme68xStatus);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    }
    else
    {
      output = "BME68X warning code : " + String(iaqSensor.bme68xStatus);
      Serial.println(output);
    }
  }
}

void errLeds(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}