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

  Serial.print("Connecting...");
  int tryCount = 40;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    if (tryCount-- <= 0) {
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected");
  } else {
    Serial.print("Connectiong failed");
  }

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

     getStatus();
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
    const long db_update = root["db_update"].as<long>();
    const char* open      = root["open"];
    const bool  lock      = root["lock"];

    if (EEPROMReadLong(OFFSET_db_timestamp) != db_update) {
      Serial.println("OLD DB, UPPDATE!!! ");
      getUsers();  
      EEPROMWriteLong(OFFSET_db_timestamp, db_update);
    }
   
    //open lock from server
    if (EEPROMReadLong(OFFSET_open_timestamp) != open) {
      Serial.println("OPEN DOOR - SERVER COMMAND ");
      doOpen("SERVER", "SERVER");
      EEPROMWriteLong(OFFSET_open_timestamp, open);
    }

    //TODO do global lock (disable unlock, until server unable it again. server only.)

    //TODO update device timestamp, somehow

    //TODO store users to EEPROM

    //TODO expand open form (add user key) - store it on device in cookie

    //TODO check if user has access

    //TODO add led status

    //TODO print QR-code with SSID and IP address of this geeklock


    
    http.end();  
  } else {
    Serial.print("HTTP code ");
    Serial.println(httpCode);
  }
}

void doOpen(String mac, String key) {
    Serial.print("DO OPEN DOOR! MAC: ");
    Serial.print(mac);
    Serial.print(", KEY:  ");
    Serial.println(key);
    Serial.println("PUSH TO QUERY");
    Serial.println("TRY PUSH QUERY TO SERVER");
}


void getUsers() {
  Serial.println("getUsers()");

  //do clear users
  int users_array_length = sizeof(users) /sizeof(users[0]);
  for (int i = 0; i < users_array_length; i++) {
    users[i].mac = "";
    users[i].key = "";
  }
  
  
  bool readNextBatch;
  String offset_url = "0";

  do {
    readNextBatch = false;
  
    HTTPClient http;
    StaticJsonBuffer<2000> jsonBuffer;
    
    http.begin("http://garik.pp.ua/prj/geeklock/users/?offset=" + offset_url);
    int httpCode = http.GET();
    String json = "[empty]";
    if(httpCode>0){ 
      json = http.getString();
      http.end();
      Serial.println(json);
  
      JsonObject& root = jsonBuffer.parseObject(json);
      if (!root.success()) {
        Serial.println("parseObject() failed");
        return;
      }
  
      const int total = root["total"];
      const int users_count = root["length"];
      const int offset = root["offset"];
      offset_url = String(offset + users_count);

      for (int i = 0; i < users_count; i++) {
        const char* mac = root["users"][i]["mac"];
        const char* key = root["users"][i]["key"];
        users[offset + i].mac = mac;
        users[offset + i].key = key;   
      } 

      if (offset + users_count < total) {
        readNextBatch = true;
      }  
    } else {
      Serial.print("HTTP code ");
      Serial.println(httpCode);
    }
  } while (readNextBatch);


  for (int i = 0; i < users_array_length; i++) {
    Serial.print("user ");
    Serial.print(i);
    Serial.print(": MAC = ");
    Serial.print(users[i].mac);
    Serial.print(", KEY = ");
    Serial.println(users[i].key);
  } 

  //do store in EEPROM here
  Serial.println("do store in EEPROM here");
}


















