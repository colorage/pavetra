#include <WiFiManager.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "PMS.h"

int deviceId = 601;
int pm25;
int pm10;
String pm_data;

PMS pms(Serial);
PMS::DATA data;

void setup() {
  Serial.begin(9600); 

  // === Connect to Internet ===
  WiFiManager wifiManager;
  wifiManager.autoConnect("pavetra");

  // === Get PM data ===
  if (pms.readUntil(data)) {
    pm25 = data.PM_AE_UG_2_5;
    pm10 = data.PM_AE_UG_10_0;
    pm_data = "{\"sensor_2_5\": " + String(pm25) + ", \"sensor_10\": " + String(pm10) + " }";
  } else {
    pm_data = "{\"sensor_2_5\": 0, \"sensor_10\": 0}";
  }
  
  // === HTTPS Request ===
  HTTPClient http;
  http.begin("http://pavetra.online/devices/data");
  http.addHeader("Authorization", "Token eyJhbGciOiJIUzI1NiJ9.eyJkZXZpY2VfaWQiOjQsImV4cCI6MTU4NDAyNDQzOH0.4GauS8uDUhX8LfGbGLL2VZ9ZeO6j7C4uXAj2RoLx3XU");
  int httpCode = http.POST(pm_data);

  http.end();

  // === Sleep Mode ===
  ESP.deepSleep(1*60*1000*1000); // Sleep 1 minute
}

void loop() {
  
}
