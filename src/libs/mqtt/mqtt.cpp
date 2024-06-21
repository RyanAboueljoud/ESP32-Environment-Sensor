#include "mqtt.h"

Preferences pref;
const int TIMEOUT_MS = 10000;

String getHostname(){
  String hostname = "ERR-HOSTNAME";

  Serial.println("Retreiving hostname from flash memory...");
  pref.begin("esp32", false);
  if (pref.isKey("hostname")){
          hostname = pref.getString("hostname");
          Serial.println("hostname retrieved: " + hostname);
  }
  pref.end();

  return hostname;
}

String *getMQTTCreds() {
  String *mqtt_creds = new String[2];

  pref.begin("esp32", false);
  if (pref.isKey("mqtt_user")){
          mqtt_creds[0] = pref.getString("mqtt_user");
  }
  if (pref.isKey("mqtt_pass")){
          mqtt_creds[1] = pref.getString("mqtt_pass");
  }
  pref.end();

  return mqtt_creds;
}

void reconnectMQTT(PubSubClient client, const String MQTT_USER, const String MQTT_PASS) {
  int const MAX_RETRY = 3;
  int retryCount = 0;
  // String hostname = getHostname();
  String hostname = "ESP32123456789";

  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (retryCount < MAX_RETRY){
      // Attempt to connect
      Serial.println("DEBUG: retryCount: " + String(retryCount) + " hostname: " + hostname.c_str());
      if (client.connect(hostname.c_str(), MQTT_USER.c_str(), MQTT_PASS.c_str())) {
        Serial.println("connected");
        // Subscribe
        // client.subscribe((hostname+"/get_sensor_vals").c_str()); // TODO: Return sensors values on demand
        client.subscribe((hostname+"/update_hostname").c_str());
        client.subscribe((hostname+"/restart").c_str());
        client.subscribe((hostname+"/reset").c_str());
        client.subscribe("all/get_hostname");
        client.subscribe("homeassistant/status");

        // pref.begin("esp32", false);
        // pref.putString("mqtt_user", MQTT_USER);
        // pref.putString("mqtt_pass", MQTT_PASS);
        // pref.end();      
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in " + String(TIMEOUT_MS/1000) + " seconds");
        retryCount++;
        // Wait 10 seconds before retrying
        // errLeds(2, TIMEOUT_MS); // FIX: remove "magic" number for LED pin when converting to a class
        delay(10000);
      }
    } else {
      Serial.println("Reached max retry limit (" + String(MAX_RETRY) + "): Skipping MQTT connection for now...");
    }
  }
}

// TEMP callback function - need to fix incorrect func signature bug on primary callback func first
void callback(const char* topic, byte* payload, unsigned int length) {
  // digitalWrite(LED_BUILTIN, HIGH);
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println();

  if (String(topic) == "homeassistant/status") {
    if(messageTemp == "online"){
      
    }
    if(messageTemp == "offline"){
      delay(10000);
    }
    
  }
}


// TODO: Fix this primary callback to match expected signature: (const char* topic, byte* payload, unsigned int length)  
//       This will likely require moving mqtt code to a class

// void callback(PubSubClient client, char* topic, byte* message, unsigned int length, int LED, const String SSID, const String SSID_PASSWD) {
//   String hostname = getHostname();
//   String *mqtt_creds = getMQTTCreds();
//   String messageTemp;
//   digitalWrite(LED, HIGH);
//   Serial.print("Message arrived on topic: ");
//   Serial.print(topic);
//   Serial.print(". Message: ");

//   for (int i = 0; i < length; i++) {
//     Serial.print((char)message[i]);
//     messageTemp += (char)message[i];
//   }
//   Serial.println();

//   // Feel free to add more if statements to control more GPIOs with MQTT

//   // If a message is received on the topic [hostname]/output, you check if the message is either "on" or "off". 
//   // Changes the output state according to the message
//   if (String(topic) == hostname + "/update_hostname") {
//     if (!messageTemp.isEmpty()) {
//       hostname = messageTemp;
//       pref.begin("esp32", false);
//       pref.putString("hostname", hostname);
//       pref.end();
//       initWifi(LED, hostname, SSID, SSID_PASSWD);
//       delay(100);
//       reconnectMQTT(client, hostname, mqtt_creds[0], mqtt_creds[1]);
//     }
//   }

//   // If a message is received on the topic [hostname]/output, you check if the message is either "on" or "off". 
//   // Changes the output state according to the message
//   if (String(topic) == hostname + "/restart") {
//     if (messageTemp == "true"){
//       Serial.println("Restarting device...");
//       delay(1000);
//       ESP.restart();
//     }
//   }

//   if (String(topic) == hostname + "/reset") {
//     if (messageTemp == "true"){
//       pref.begin("esp32", false);
//       pref.putBool("reset", true);
//       pref.end();
//       delay(100);
//       ESP.restart();
//     }
//   }

//   // If a message is received on the topic all/output, you check if the message is either "on" or "off". 
//   // Changes the output state according to the message
//   if (String(topic) == "all/get_hostname") {
//     client.publish("all/broadcast_hostname", hostname.c_str());
//   }

//   if (String(topic) == "homeassistant/status") {
//     if(messageTemp == "online"){
      
//     }
//     if(messageTemp == "offline"){
//       delay(10000);
//     }
    
//   }
// }




