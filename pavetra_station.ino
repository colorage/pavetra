#include <WiFiManager.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "PMS.h"

int pm25 = 0;
int pm10 = 0;
String pm_data;

PMS pms(Serial);
PMS::DATA data;

void setup() {
  Serial.begin(9600);
  // === Connect to Internet ===
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(120);
  wifiManager.setTimeout(120);
  wifiManager.autoConnect("pavetra");

  // === Get PM data ===
  pms.wakeUp();
}

void loop() {
  delay(30*1000);
  readDataFromPMS();
  sendDataToServer();
  // === Sleep Mode ===
  pms.sleep();
  ESP.deepSleep(20*60*1000*1000); // Sleep 20 minutes
}

void readDataFromPMS() {
  int count_read = 0;
  int count_try_to_read = 0;
  while (count_read < 10 && count_try_to_read < 10) {
    if (pms.readUntil(data)) {
      pm25 += data.PM_AE_UG_2_5;
      pm10 += data.PM_AE_UG_10_0;
      ++count_read;
    } else {
      ++count_try_to_read;
    }
  }
  pm_data = "{\"sensor_2_5\": " + String(pm25 / 10.0) + ", \"sensor_10\": " + String(pm10 / 10.0) + " }";
  Serial.println(pm_data);
}

void sendDataToServer() {
  // === HTTPS Request ===
  HTTPClient http;
  http.begin("https://pavetra.online/devices/data", "F5 1B 7D A0 A5 8E 9E 07 9E 9F 7E F9 0E 0E 8A BE 57 D1 9A 5A");
  http.addHeader("Authorization", "Token ***");
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(pm_data);
  http.end();
}
