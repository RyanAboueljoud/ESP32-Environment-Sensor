#include "mqtt.h"

/**
 * @brief MQTT callback function to handle incoming messages and perform various actions based on the topic.
 *
 * This function processes messages received on different MQTT topics and takes corresponding actions,
 * such as publishing sensor values, updating hostname, restarting or resetting the device, and broadcasting
 * the hostname. Supported topics include:
 * - `/get_sensor_values`: Publishes current sensor values (e.g., IAQ, temperature, pressure, humidity).
 * - `/get_iaq`: Publishes current IAQ sensor values.
 * - `/get_gas`: Publishes current gas sensor values.
 * - `/get_temperature`: Publishes current temperature sensor values.
 * - `/get_pressure`: Publishes current pressure sensor values and calculates altitude.
 * - `/get_humidity`: Publishes current humidity sensor values.
 * - `/update_hostname`: Updates the device hostname and reconnects to WiFi and MQTT.
 * - `/restart`: Restarts the device.
 * - `/reset`: Resets the device preferences and restarts.
 * - `all/get_hostname`: Broadcasts the current hostname.
 * - `homeassistant/status`: Performs actions based on Home Assistant status messages.
 *
 * @param topic The topic on which the message was received.
 * @param payload The message payload.
 * @param length The length of the message payload.
 */
void MQTT::callback(const char *topic, byte *payload, unsigned int length)
{
  const float SEA_LVL_PRESSURE = 1010; // hPa (Auburn - 500m)
  float pressure_hPa = 0;
  Preferences pref;
  String hostname = getHostname();
  String ssid;
  String ssid_passwd;
  String messageTemp;
  String pubMessage;
  String hostTopic;
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println();

  if (String(topic) == hostname + "/get_sensor_values")
  {
    if (messageTemp == "true")
    {
      Serial.println("Publishing current sensor values...");
      JsonDocument doc;
      // Add values to the JSON document - Max Keys: 10
      doc["iaq"] = iaqSensor->iaq;
      doc["staticIaq"] = iaqSensor->staticIaq;
      doc["iaqAccuracy"] = iaqSensor->iaqAccuracy;
      doc["stabStatus"] = iaqSensor->stabStatus;
      doc["gasResistance"] = iaqSensor->gasResistance;
      doc["co2Equivalent"] = iaqSensor->co2Equivalent;
      doc["breathVocEquivalent"] = iaqSensor->breathVocEquivalent;
      doc["temperature"] = iaqSensor->temperature;
      doc["pressure"] = iaqSensor->pressure;
      doc["humidity"] = iaqSensor->humidity;

      serializeJson(doc, pubMessage);

      hostTopic = hostname + "/sensor";

      publish(hostTopic.c_str(), pubMessage.c_str());
    }
  }

  if (String(topic) == hostname + "/get_iaq")
  {
    if (messageTemp == "true")
    {
      Serial.println("Publishing current IAQ sensor values...");
      JsonDocument doc;
      // Add values to the JSON document - Max Keys: 10
      doc["iaq"] = iaqSensor->iaq;
      doc["staticIaq"] = iaqSensor->staticIaq;
      doc["iaqAccuracy"] = iaqSensor->iaqAccuracy;
      doc["runInStatus"] = iaqSensor->runInStatus;

      serializeJson(doc, pubMessage);

      hostTopic = hostname + "/iaq";

      publish(hostTopic.c_str(), pubMessage.c_str());
    }
  }

  if (String(topic) == hostname + "/get_gas")
  {
    if (messageTemp == "true")
    {
      Serial.println("Publishing current gas sensor values...");
      JsonDocument doc;
      // Add values to the JSON document - Max Keys: 10
      doc["co2Equivalent"] = iaqSensor->co2Equivalent;
      doc["breathVocEquivalent"] = iaqSensor->breathVocEquivalent;
      doc["gasResistance"] = iaqSensor->gasResistance;
      doc["gasPercentage"] = iaqSensor->gasPercentage;

      serializeJson(doc, pubMessage);

      hostTopic = hostname + "/gas";

      publish(hostTopic.c_str(), pubMessage.c_str());
    }
  }

  if (String(topic) == hostname + "/get_temperature")
  {
    if (messageTemp == "true")
    {
      Serial.println("Publishing current gas sensor values...");
      JsonDocument doc;
      // Add values to the JSON document - Max Keys: 10
      doc["rawTemperature"] = iaqSensor->rawTemperature;
      doc["temperature_C"] = iaqSensor->temperature;
      doc["temperature_F"] = celsiusToFahrenheit(iaqSensor->temperature);

      serializeJson(doc, pubMessage);

      hostTopic = hostname + "/temperature";

      publish(hostTopic.c_str(), pubMessage.c_str());
    }
  }

  if (String(topic) == hostname + "/get_pressure")
  {
    if (messageTemp == "true")
    {
      Serial.println("Publishing current pressure sensor values...");
      JsonDocument doc;
      // Add values to the JSON document - Max Keys: 10
      doc["pressure"] = iaqSensor->pressure;

      // Altitude in meters (https://github.com/adafruit/Adafruit_CircuitPython_BME680/)
      pressure_hPa = iaqSensor->pressure * 0.01; // Convert Pa to hPa
      doc["altitude"] = calcAltitude(iaqSensor->pressure);

      serializeJson(doc, pubMessage);

      hostTopic = hostname + "/pressure";

      publish(hostTopic.c_str(), pubMessage.c_str());
    }
  }

  if (String(topic) == hostname + "/get_humidity")
  {
    if (messageTemp == "true")
    {
      Serial.println("Publishing current humidity sensor values...");
      JsonDocument doc;
      // Add values to the JSON document - Max Keys: 10
      doc["rawHumidity"] = iaqSensor->rawHumidity;
      doc["humidity"] = iaqSensor->humidity;

      serializeJson(doc, pubMessage);

      hostTopic = hostname + "/humidity";

      publish(hostTopic.c_str(), pubMessage.c_str());
    }
  }

  // If a message is received on the topic [hostname]/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == hostname + "/update_hostname")
  {
    if (!messageTemp.isEmpty())
    {
      hostname = messageTemp;
      pref.begin("esp32", false);
      pref.putString("hostname", hostname);
      ssid = pref.getString("ssid");
      ssid_passwd = pref.getString("ssid_passwd");
      pref.end();
      initWifi(LED_BUILTIN, hostname, ssid, ssid_passwd);
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
      pref.begin("esp32", false);
      pref.putBool("reset", true);
      pref.end();
      delay(100);
      ESP.restart();
    }
  }

  // Publish current hostname
  if (String(topic) == "all/get_hostname")
  {
    publish("all/broadcast_hostname", hostname.c_str());
  }

  if (String(topic) == "homeassistant/status")
  {
    if (messageTemp == "online")
    {
      Serial.println("Home Assistant Online");
      MQTT::isBrokerOnline = true;
    }
    if (messageTemp == "offline")
    {
      Serial.println("Home Assistant Offline");
      MQTT::isBrokerOnline = false;
    }
  }
}

/**
 * @brief Retrieves the device hostname from flash memory.
 *
 * This function attempts to retrieve the hostname stored in flash memory. If the hostname key
 * exists, it returns the stored hostname. If the key does not exist, it returns "ERR-HOSTNAME".
 * It logs the retrieval process to the serial console.
 *
 * @return The hostname as a String.
 */
String MQTT::getHostname()
{
  String hostname = "ERR-HOSTNAME";

  pref.begin("esp32", true);
  if (pref.isKey("hostname"))
  {
    hostname = pref.getString("hostname");
  }
  pref.end();

  return hostname;
}

/**
 * @brief Attempts to reconnect to the MQTT server with a maximum number of retries.
 *
 * This function continuously tries to connect to the MQTT server using the stored hostname,
 * user credentials, and server address. If the connection is successful, it subscribes to
 * relevant topics. The function retries the connection up to a maximum number of attempts
 * specified by MAX_RETRY. If the connection fails, it logs the failure and increments the
 * retry count, performing an error indication and waiting before retrying.
 */
void MQTT::reconnectMQTT()
{
  int const MAX_RETRY = 3;
  int retryCount = 0;
  String hostname = getHostname();

  // Loop until we're reconnected
  while (!client->connected())
  {
    Serial.print("\nAttempting MQTT connection...");
    if (retryCount < MAX_RETRY)
    {
      // Attempt to connect
      if (client->connect(hostname.c_str(), mqtt_user.c_str(), mqtt_pass.c_str()))
      {
        Serial.println("connected to: " + mqtt_server + " as " + hostname);
        // Subscribe
        client->subscribe((hostname + "/get_sensor_values").c_str());
        client->subscribe((hostname + "/get_iaq").c_str());
        client->subscribe((hostname + "/get_gas").c_str());
        client->subscribe((hostname + "/get_pressure").c_str());
        client->subscribe((hostname + "/get_humidity").c_str());
        client->subscribe((hostname + "/get_temperature").c_str());
        client->subscribe((hostname + "/update_hostname").c_str());
        client->subscribe((hostname + "/restart").c_str());
        client->subscribe((hostname + "/reset").c_str());
        client->subscribe("all/get_hostname");
        client->subscribe("homeassistant/status");
      }
      else
      {
        Serial.print("failed, rc=");
        Serial.print(client->state());
        Serial.println(" try again in " + String(TIMEOUT_MS / 1000) + " seconds");
        retryCount++;
        errLeds(LED_BUILTIN, TIMEOUT_MS);
      }
    }
    else
    {
      Serial.println("Reached max retry limit (" + String(MAX_RETRY) + "): Skipping MQTT connection for now...");
    }
  }
}

bool MQTT::isConnected()
{
  if (!client->connected())
  {
    return false;
  }
  else
  {
    return true;
  }
}

void MQTT::loop()
{
  client->loop();
}

/**
 * @brief Publishes a message to a specified MQTT topic.
 *
 * This function sends a message payload to a given MQTT topic. It returns a boolean value
 * indicating whether the publish operation was successful.
 *
 * @param topic The MQTT topic to publish the message to.
 * @param payload The message payload to be published.
 * @return true if the message was successfully published, false otherwise.
 */
bool MQTT::publish(const char *topic, const char *payload)
{
  return client->publish(topic, payload);
}
