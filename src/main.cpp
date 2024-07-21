#include <Arduino.h>
#include <bsec.h>
#include <Preferences.h>
#include <nvs_flash.h>

#include "libs/initWifi/initWifi.h"
#include "libs/readConfigFile/readConfigFile.h"
#include "libs/mqtt/mqtt.h"
#include "libs/errLeds/errLeds.h"

#define BME680_I2C_ADDR_LOW 0x77

Preferences preferences;

// Create an object of the class Bsec
Bsec iaqSensor;

String output;
int LED_BUILTIN = 2;

int const TIMEOUT_MS = 10000;

// Wi-Fi Network Connection Configuration
String ssid;
String ssid_passwd;
String hostname = "esp32";

WiFiClient espClient;
PubSubClient client(espClient);

// MQTT Broker IP Address and Login
String mqtt_server;
int mqtt_port;
String mqtt_user;
String mqtt_pass;

MQTT mqtt;

// InfluxDB server address and login
String influxdb_server;
String influxdb_port;
String influxdb_user;
String influxdb_pass;

// Helper functions declarations
void checkIaqSensorStatus(void);

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

  readConfigFile(ssid, ssid_passwd, mqtt_server, mqtt_port, mqtt_user, mqtt_pass, influxdb_server, influxdb_port, influxdb_user, influxdb_pass);

  if (preferences.isKey("hostname"))
  {
    hostname = preferences.getString("hostname");
    Serial.println("\nhostname loaded from flash memory: " + hostname + "\n");
  }
  else
  {
    preferences.putString("hostname", hostname);
  }
  // Close flash storage filesystem
  preferences.end();

  initWifi(LED_BUILTIN, hostname, ssid, ssid_passwd);

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
  
  mqtt.setup(client, mqtt_server, mqtt_port, mqtt_user, mqtt_pass, iaqSensor);

  digitalWrite(LED_BUILTIN, LOW);
}

void loop()
{
  unsigned long time_trigger = millis();
  digitalWrite(LED_BUILTIN, LOW);

  if (!WiFi.isConnected())
  {
    initWifi(LED_BUILTIN, hostname, String(ssid), String(ssid_passwd));
  }

  if (!mqtt.isConnected())
  {
    mqtt.reconnectMQTT();
  }
  else
  {
    mqtt.loop();
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
      errLeds(LED_BUILTIN, TIMEOUT_MS); /* Halt in case of failure */
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
      errLeds(LED_BUILTIN, TIMEOUT_MS); /* Halt in case of failure */
    }
    else
    {
      output = "BME68X warning code : " + String(iaqSensor.bme68xStatus);
      Serial.println(output);
    }
  }
}
