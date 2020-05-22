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
  http.begin("https://pavetra.online/devices/data", "8E B6 79 2C 41 0B D7 5A C1 52 C6 2D 48 DA 6C 0B DD 2F FA 25");
  http.addHeader("Authorization", "Token ***");
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(pm_data);
  http.end();
}
