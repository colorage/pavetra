#include <M5Stack.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include "WebServer.h"
#include <HTTPClient.h>

#include <Preferences.h>

#define DATA_LEN 32

int pm25 = 0;
int pm10 = 0;

uint8_t Air_val[32]={0};
int16_t p_val[16]={0};
uint8_t i=0;

const IPAddress apIP(192, 168, 4, 1);
const char* apSSID = "pavetra";
boolean settingMode;
String ssidList;
String wifi_ssid;
String wifi_password;

// DNSServer dnsServer;
WebServer webServer(80);

// wifi config store
Preferences preferences;

void setup() {
  M5.begin();
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  pinMode(10, OUTPUT);
  digitalWrite(10, 1);
  
  preferences.begin("wifi-config");

  delay(10);
  if (restoreConfig()) {
    if (checkConnection()) {
      settingMode = false;
      startWebServer();
      return;
    }
  }
  settingMode = true;
  setupMode();
}

void loop() {
  if (settingMode) {
  }

  webServer.handleClient();
  
  if(Serial2.available()) {
      Air_val[i] = Serial2.read();
      Serial.write( Air_val[i]);
      i++;
   }else{
      i=0;
   }

  if(i==DATA_LEN){
    readDataFromPMS();    
    //sendDataToServer();
  }
}

boolean restoreConfig() {
  wifi_ssid = preferences.getString("WIFI_SSID");
  wifi_password = preferences.getString("WIFI_PASSWD");
  M5.Lcd.print("WIFI-SSID: ");
  M5.Lcd.println(wifi_ssid);
  M5.Lcd.print("WIFI-PASSWD: ");
  M5.Lcd.println(wifi_password);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

  if(wifi_ssid.length() > 0) {
    return true;
} else {
    return false;
  }
}

boolean checkConnection() {
  int count = 0;
  M5.Lcd.print("Waiting for Wi-Fi connection");
  while ( count < 30 ) {
    if (WiFi.status() == WL_CONNECTED) {
      M5.Lcd.println();
      M5.Lcd.println("Connected!");
      return (true);
    }
    delay(500);
    M5.Lcd.print(".");
    count++;
  }
  M5.Lcd.println("Timed out.");
  return false;
}

void startWebServer() {
  if (settingMode) {
    M5.Lcd.print("Starting Web Server at ");
    M5.Lcd.println(WiFi.softAPIP());
    webServer.on("/settings", []() {
      String s = "<h1>Wi-Fi Settings</h1><p>Please enter your password by selecting the SSID.</p>";
      s += "<form method=\"get\" action=\"setap\"><label>SSID: </label><select name=\"ssid\">";
      s += ssidList;
      s += "</select><br>Password: <input name=\"pass\" length=64 type=\"password\"><input type=\"submit\"></form>";
      webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    });
    webServer.on("/setap", []() {
      String ssid = urlDecode(webServer.arg("ssid"));
      M5.Lcd.print("SSID: ");
      M5.Lcd.println(ssid);
      String pass = urlDecode(webServer.arg("pass"));
      M5.Lcd.print("Password: ");
      M5.Lcd.println(pass);
      M5.Lcd.println("Writing SSID to EEPROM...");

      // Store wifi config
      M5.Lcd.println("Writing Password to nvr...");
      preferences.putString("WIFI_SSID", ssid);
      preferences.putString("WIFI_PASSWD", pass);

      M5.Lcd.println("Write nvr done!");
      String s = "<h1>Setup complete.</h1><p>device will be connected to \"";
      s += ssid;
      s += "\" after the restart.";
      webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
      delay(3000);
      ESP.restart();
    });
    webServer.onNotFound([]() {
      String s = "<h1>AP mode</h1><p><a href=\"/settings\">Wi-Fi Settings</a></p>";
      webServer.send(200, "text/html", makePage("AP mode", s));
    });
  }
  else {
    M5.Lcd.print("Starting Web Server at ");
    M5.Lcd.println(WiFi.localIP());
    webServer.on("/", []() {
      String s = "<h1>STA mode</h1><p><a href=\"/reset\">Reset Wi-Fi Settings</a></p>";
      webServer.send(200, "text/html", makePage("STA mode", s));
    });
    webServer.on("/reset", []() {
      // reset the wifi config
      preferences.remove("WIFI_SSID");
      preferences.remove("WIFI_PASSWD");
      String s = "<h1>Wi-Fi settings was reset.</h1><p>Please reset device.</p>";
      webServer.send(200, "text/html", makePage("Reset Wi-Fi Settings", s));
      delay(3000);
      ESP.restart();
    });
  }
  webServer.begin();
}

void setupMode() {
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  M5.Lcd.println("");
  for (int i = 0; i < n; ++i) {
    ssidList += "<option value=\"";
    ssidList += WiFi.SSID(i);
    ssidList += "\">";
    ssidList += WiFi.SSID(i);
    ssidList += "</option>";
  }
  delay(100);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID);
  WiFi.mode(WIFI_MODE_AP);
  startWebServer();
  M5.Lcd.print("Starting Access Point at \"");
  M5.Lcd.print(apSSID);
  M5.Lcd.println("\"");
}

String makePage(String title, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += contents;
  s += "</body></html>";
  return s;
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}

void sendDataToServer() {
  // === HTTPS Request ===
  HTTPClient http;
  M5.Lcd.println("Sending data to pavetra");
  http.begin("https://pavetra.online/devices/data", "F5 1B 7D A0 A5 8E 9E 07 9E 9F 7E F9 0E 0E 8A BE 57 D1 9A 5A");
  http.addHeader("Authorization", "Token ***");
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST("{\"sensor_2_5\": " + String(pm25) + ", \"sensor_10\": " + String(pm10) + " }");
  http.end();
  M5.Lcd.println("Done");
}

void readDataFromPMS() {

  for(int i=0,j=0;i<32;i++){
        if(i%2==0){
          p_val[j] = Air_val[i];
          p_val[j] = p_val[j] <<8;
        }else{
          p_val[j] |= Air_val[i];
          j++;
        }
     }

  

  pm25 = p_val[3];
  M5.Lcd.println("PM2.5: "+ String(pm25));
  pm10 = p_val[4];
  M5.Lcd.println("PM10: "+ String(pm10));
}
