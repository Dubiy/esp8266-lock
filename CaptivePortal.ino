#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif

#include <ESP8266WiFi.h>
#include "./DNSServer.h"                  // Patched lib
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include <ArduinoJson.h>


const byte        DNS_PORT = 53;          // Capture DNS requests on port 53
IPAddress         apIP(10, 10, 10, 1);    // Private network for server
DNSServer         dnsServer;              // Create the DNS object
ESP8266WebServer  webServer(80);          // HTTP server

unsigned long previousMillis = 0, currentMillis = 0; // last time update
long INTERVAL_status = 30000; // interval at which to do something (milliseconds)

typedef struct {
  String mac;
  String key;
} user;

user users[100];


#include "./WIFIlibGARY_eeprom.h"
#include "./WIFIlibGARY_pages.h"


void setup() {
  EEPROM.begin(4096);
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  WiFi.mode(WIFI_AP_STA);

  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  //create AP
  if (getEEPROMString(OFFSET_ap_password, LENGHT_ap_password).length() >= 8) {
    WiFi.softAP(getEEPROMString(OFFSET_ap_ssid, LENGHT_ap_ssid).c_str(), getEEPROMString(OFFSET_ap_password, LENGHT_ap_password).c_str());
  } else {
    WiFi.softAP(getEEPROMString(OFFSET_ap_ssid, LENGHT_ap_ssid).c_str());
  }

  //connect to wifi
  WiFi.begin(getEEPROMString(OFFSET_client_ssid, LENGHT_client_ssid).c_str(), getEEPROMString(OFFSET_client_password, LENGHT_client_password).c_str());

  Serial.print(WiFi.status());
  Serial.print(" - status = mode - ");
  Serial.println(WL_CONNECTED);

  int tryCount = 40;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(WiFi.status());
    delay(500);
    if (tryCount-- <= 0) {
      break;
    }
  }
  Serial.println(WiFi.status());

  Serial.print("users sizeof ");
  Serial.println(sizeof(users));

  
  wifi_station_dhcpc_start();
  // if DNSServer is started with "*" for domain name, it will reply with provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);

  // replay to all requests with same HTML
  webServer.onNotFound([]() {
    webServer.send(200, "text/html", indexHtml());
  });

  webServer.on("/", []() {
    webServer.send(200, "text/html", indexHtml());
  });

  webServer.on("/config", []() {
    webServer.send(200, "text/html", handle_config());
  });

  webServer.begin();

}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  currentMillis = millis();

  if(currentMillis - previousMillis > INTERVAL_status || previousMillis > currentMillis) {
    //previousMillis > currentMillis: It just show the elapsed time from the start of the board. This number will overflow (go back to zero), after approximately 50 days (source: arduino.cc/en/Reference/millis) 
     previousMillis = currentMillis;  

     // do something
     Serial.println("run start");
//     getStatus();
     getUsers();
     Serial.println("run end");
  }
}

void getStatus() {
  Serial.println("getStatus()");
  
  HTTPClient http;
  StaticJsonBuffer<200> jsonBuffer;

  http.begin("http://garik.pp.ua/prj/geeklock/status/");
  int httpCode = http.GET();
  String json = "[empty]";
  if(httpCode>0){ 
    json = http.getString();
    Serial.println(json);

    //TODO parse json here
    JsonObject& root = jsonBuffer.parseObject(json);
    if (!root.success()) {
      Serial.println("parseObject() failed");
      return;
    }

    const char* timestamp = root["timestamp"];
    const char* db_update = root["db_update"];
    const char* open      = root["open"];
    const bool  lock      = root["lock"];
    
    http.end();  
  } else {
    Serial.print("HTTP code ");
    Serial.println(httpCode);
  }
}




void getUsers() {
  Serial.println("getUsers()");
  
  HTTPClient http;
  StaticJsonBuffer<2000> jsonBuffer;

  http.begin("http://garik.pp.ua/prj/geeklock/users/");
  int httpCode = http.GET();
  String json = "[empty]";
  if(httpCode>0){ 
    json = http.getString();
    http.end();
    Serial.println(json);

    //TODO parse json here
    JsonObject& root = jsonBuffer.parseObject(json);
    if (!root.success()) {
      Serial.println("parseObject() failed");
      return;
    }

    const int users_count = root["length"];
    const int offset = root["offset"];
    Serial.print("users_count: ");
    Serial.println(users_count);

    for (int i = 0; i < users_count; i++) {
      const char* mac = root["users"][i]["mac"];
      const char* key = root["users"][i]["key"];

      Serial.print("user ");
      Serial.print(i);
      Serial.print(": MAC = ");
      Serial.print(mac);
      Serial.print(", KEY = ");
      Serial.println(key);

      users[offset + i].mac = mac;
      users[offset + i].key = key;
 
    } 

    Serial.println("done");
    Serial.print("users sizeof ");
    Serial.println(sizeof(users));
       
  } else {
    Serial.print("HTTP code ");
    Serial.println(httpCode);
  }
  Serial.println("done2/return");


  for (int i = 0; i < 100; i++) {
      Serial.print("user ");
      Serial.print(i);
      Serial.print(": MAC = ");
      Serial.print(Serial.print(users[i].mac));
      Serial.print(", KEY = ");
      Serial.println(Serial.print(users[i].key));

      users[i].mac = users[0].mac;
      users[i].key = users[0].key;

    } 

    Serial.println("finish output");
}


















