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

long timestamp,
     timestamp_update_time; 

String apikey;

typedef struct {
  String mac;
  String key;
} user_struct;

user_struct users[100];

typedef struct {
  user_struct user;
  long timestamp;
} logrecord;

logrecord queue[25];
int queue_length = 0;


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
    Serial.println("Connected");
  } else {
    Serial.println("Connectiong failed");
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

  apikey = getEEPROMString(OFFSET_server_apikey, LENGHT_server_apikey);
  loadUsers();
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  currentMillis = millis();

  if(currentMillis - previousMillis > INTERVAL_status || previousMillis > currentMillis) {
    //previousMillis > currentMillis: It just show the elapsed time from the start of the board. This number will overflow (go back to zero), after approximately 50 days (source: arduino.cc/en/Reference/millis) 
     previousMillis = currentMillis;  

     getStatus();
     pushQueue();
  }
}

void pushQueue() {
  Serial.println("pushQueue()");

  Serial.println("== QUEUE ==");
  for (int i = 0; i < queue_length; i++) {
       Serial.print((i+1));
       Serial.print(" user: [");
       Serial.print(queue[i].user.mac);
       Serial.print(", ");
       Serial.print(queue[i].user.key);
       Serial.print("], ts: ");
       Serial.println(queue[i].timestamp);
  }

  if (queue_length) {
    
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonArray& queueArr = root.createNestedArray("queue");
    
    for (int i = 0; i < queue_length; i++) {
      JsonObject& record = queueArr.createNestedObject();
      record["mac"] = queue[i].user.mac;
      record["key"] = queue[i].user.key;
      record["timestamp"] = queue[i].timestamp;
    }
    root.prettyPrintTo(Serial);
    char buffer[1000];
    root.printTo(buffer, sizeof(buffer));
  
    HTTPClient http;
    http.begin("http://garik.pp.ua/prj/geeklock/access-log/");
    http.addHeader("apikey", apikey);
    int httpCode = http.POST(buffer);
    if (httpCode == 200) {
      queue_length = 0;
    }
    http.writeToStream(&Serial);
    Serial.println(http.getString());
    http.end();
  
  
  } 
}

void getStatus() {
  Serial.println("getStatus()");
  
  HTTPClient http;
  StaticJsonBuffer<200> jsonBuffer;

  http.begin("http://garik.pp.ua/prj/geeklock/status/");
  http.addHeader("apikey", apikey);
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

    timestamp = root["timestamp"].as<long>();
    timestamp_update_time = millis();    
    const long db_update = root["db_update"].as<long>();
    const long open      = root["open"].as<long>();
    const bool lock      = root["lock"];

    if (EEPROMReadLong(OFFSET_db_timestamp) != db_update) {
      Serial.println("OLD DB, UPPDATE!!! ");
      getUsers();  
      EEPROMWriteLong(OFFSET_db_timestamp, db_update);
      EEPROM.commit();
    }
   
    //open lock from server
    if (EEPROMReadLong(OFFSET_open_timestamp) != open) {
      Serial.println("OPEN DOOR - SERVER COMMAND ");
      doOpen("SERVER", "SERVER");
      EEPROMWriteLong(OFFSET_open_timestamp, open);
      EEPROM.commit();
    }

    //TODO do global lock (disable unlock, until server unable it again. server only.)

    //TODO do ESP.reset(); on save config

    //TODO add led status

    //TODO print QR-code with SSID and IP address of this geeklock

    http.end();  
  } else {
    Serial.print("HTTP code ");
    Serial.println(httpCode);
  }
}


void loadUsers() {
  int users_array_length = sizeof(users) /sizeof(users[0]);
  for (int i = 0; i < users_array_length; i++) {
    users[i].mac = getEEPROMString(OFFSET_users + i * (mac_length + key_length),              mac_length);;
    users[i].key = getEEPROMString(OFFSET_users + i * (mac_length + key_length) + mac_length, key_length);;
  }

  Serial.println("== USERS LOADED ==");

  for (int i = 0; i < users_array_length; i++) {
    Serial.print("user ");
    Serial.print(i);
    Serial.print(": MAC = ");
    Serial.print(users[i].mac);
    Serial.print(", KEY = ");
    Serial.println(users[i].key);
  } 
}

void saveUsers() {
  int users_array_length = sizeof(users) /sizeof(users[0]);
  for (int i = 0; i < users_array_length; i++) {
    setEEPROMString(OFFSET_users + i * (mac_length + key_length),              mac_length, users[i].mac);
    setEEPROMString(OFFSET_users + i * (mac_length + key_length) + mac_length, key_length, users[i].key);
  }

  EEPROM.commit();  
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
    http.addHeader("apikey", apikey);
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

        users[offset + i].mac.replace("-", "");
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
  saveUsers();
}
