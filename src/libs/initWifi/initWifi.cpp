// initWifi.cpp
#include "initWifi.h"

void initWifi(int LED, String hostname, String SSID, String SSID_PASSWD)
{
  int ledStatus = HIGH;
  int count = 0;
  int startTime = -1;
  int timeout_ms = 300000; // 5 min in milliseconds
  int maxRetry = 30;
  Preferences preferences;

  pinMode(LED, OUTPUT);

  // Set WiFi to station mode and disconnect from an AP if it was previously connected.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname.c_str()); // define hostname
  delay(100);

  digitalWrite(LED, ledStatus);
  WiFi.begin(SSID, SSID_PASSWD);
  Serial.print("Connecting to " + SSID + " ..");

  startTime = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    count++;
    delay(1000);
    if (count > maxRetry)
    {
      Serial.println("\nUnable to connect to  " + SSID + " - Retrying in " + (String)(timeout_ms / 60000) + " minutes...");
      errLeds(LED, timeout_ms);
      Serial.print("\nConnecting to " + SSID + " ..");
    }
    // Blink LED
    ledStatus = !ledStatus;
    digitalWrite(LED, ledStatus);
  }
  Serial.print(WiFi.localIP());
  Serial.print(" ");
  Serial.println(hostname);

  preferences.begin("esp32", false);
  preferences.putString("ssid", SSID);
  preferences.putString("ssid_passwd", SSID_PASSWD);
  preferences.end();
}