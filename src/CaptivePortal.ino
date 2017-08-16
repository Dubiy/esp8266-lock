#include <Arduino.h>

#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif

#define GREEN_LED_PIN     4    //D2
#define RED_LED_PIN       5    //D1
#define GERKON_BUTTON_PIN 12   //D6
#define ALERT_PIN         16   //D0     // Led in NodeMCU at pin GPIO16 (D0).

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

unsigned long previousMillis = 0, currentMillis = 0, lockAtMillis = 0;
volatile unsigned long alertAtMillis = 0; // last time update
long INTERVAL_status = 30000; // interval at which to do something (milliseconds)
long INTERVAL_lock = 10000;
long ALERT_lock = 10000;

long timestamp,
     timestamp_update_time;

String apikey;
String host;

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
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  WiFi.mode(WIFI_AP_STA);

  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  //create AP
  if (getEEPROMString(OFFSET_ap_password, LENGHT_ap_password).length() >= 8) {
    WiFi.softAP(getEEPROMString(OFFSET_ap_ssid, LENGHT_ap_ssid).c_str(), getEEPROMString(OFFSET_ap_password, LENGHT_ap_password).c_str());
  } else {
    WiFi.softAP(getEEPROMString(OFFSET_ap_ssid, LENGHT_ap_ssid).c_str());
  }

  Serial.println("Config data");
  Serial.print("SSID: ");
  Serial.print(getEEPROMString(OFFSET_ap_ssid, LENGHT_ap_ssid));
  Serial.print("  password: ");
  Serial.println(getEEPROMString(OFFSET_ap_password, LENGHT_ap_password));

  Serial.print("Inet SSID: ");
  Serial.print(getEEPROMString(OFFSET_client_ssid, LENGHT_client_ssid));
  Serial.print("  password: ");
  Serial.println(getEEPROMString(OFFSET_client_password, LENGHT_client_password));

  Serial.print("Admin password: ");
  Serial.print(getEEPROMString(OFFSET_password, LENGHT_password));

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
  host = getEEPROMString(OFFSET_server_host, LENGHT_server_host);
  loadUsers();

  pinMode(ALERT_PIN, OUTPUT);   // LED pin as output.
  pinMode(GERKON_BUTTON_PIN, INPUT_PULLUP); // ==> FLASH BUTTON DEFAULT IS HIGH !!
  digitalWrite(ALERT_PIN, LOW);  // turn the ALERT off.

  //GPIO0 - gerkon pin
  //GPIO5 - red LED pin
  //GPIO4 - green LED pin
  //GPIO16 - alert pin

  attachInterrupt(digitalPinToInterrupt(GERKON_BUTTON_PIN), pressedButtonInt, CHANGE); // CHANGE, RISING, FALLING
  // attachInterrupt(digitalPinToInterrupt(GERKON_BUTTON_PIN), unPressedButtonInt, RISING); // CHANGE, RISING, FALLING
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

     if (WiFi.status() != WL_CONNECTED) {
       Serial.println("Not connected, try to reconnect");
       WiFi.begin(getEEPROMString(OFFSET_client_ssid, LENGHT_client_ssid).c_str(), getEEPROMString(OFFSET_client_password, LENGHT_client_password).c_str());
     } else {
       Serial.println("Ok, connected");
     }
  }

  if(lockAtMillis && lockAtMillis < currentMillis) {
    //do lock after 10 seconds
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    lockAtMillis = 0;
  }

  if(alertAtMillis && alertAtMillis < currentMillis) {
      digitalWrite(ALERT_PIN, HIGH);
  } else {
      digitalWrite(ALERT_PIN, LOW);
  }
}


void pressedButtonInt() {
  //http://www.esp8266.com/viewtopic.php?f=32&t=4694&sid=4940a5d2d4b3658c7f8ff8b3e52c1595&start=4#sthash.2nZ3VJGH.dpuf
  // interrupt service routine (ISR) can ONLY modify VOLATILE variables

  if (digitalRead(GERKON_BUTTON_PIN) == HIGH) {
    Serial.println(F("Door opened!"));
    alertAtMillis = millis() + ALERT_lock;
  } else {
    Serial.println(F("Door closed!"));
    alertAtMillis = 0;
  }
}

void pushQueue() { //push log to server
  Serial.println("pushQueue()");
  int queue_array_length = sizeof(queue) /sizeof(queue[0]);
  int total = queue_length;
  if (total > queue_array_length) {
    total = queue_array_length;
  }

  Serial.println("== QUEUE ==");
  for (int i = 0; i < total; i++) {
       Serial.print((i+1));
       Serial.print(" user: [");
       Serial.print(queue[i].user.mac);
       Serial.print(", ");
       Serial.print(queue[i].user.key);
       Serial.print("], ts: ");
       Serial.println(queue[i].timestamp);
  }

  if (total) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonArray& queueArr = root.createNestedArray("queue");

    for (int i = 0; i < total; i++) {
      JsonObject& record = queueArr.createNestedObject();
      record["mac"] = queue[i].user.mac;
      record["key"] = queue[i].user.key;
      record["timestamp"] = queue[i].timestamp;
    }
    root.prettyPrintTo(Serial);
    char buffer[1000];
    root.printTo(buffer, sizeof(buffer));

    HTTPClient http;
    http.begin(host + "/api/access-log");
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

  http.begin(host + "/api/status");
  http.addHeader("apikey", apikey);
  int httpCode = http.GET();
  String json = "[empty]";
  if(httpCode>0){
    json = http.getString();
    Serial.println(json);

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

      if (getUsers()) {
          EEPROMWriteLong(OFFSET_db_timestamp, db_update);
          EEPROM.commit();
      }
    }

    //open lock from server
    if (EEPROMReadLong(OFFSET_open_timestamp) != open) {
      Serial.println("OPEN DOOR - SERVER COMMAND ");
      doOpen("SERVER", "SERVER");
      EEPROMWriteLong(OFFSET_open_timestamp, open);
      EEPROM.commit();
    }

    //---TODO do global lock (disable unlock, until server unable it again. server only.)

    //TODO do ESP.reset(); on save config

    //TODO print QR-code with SSID and IP address of this geeklock

    //swagger.io

    http.end();
  } else {
    Serial.print("HTTP code ");
    Serial.println(httpCode);
  }
}

void loadUsers() { //load from EEPROM
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

void saveUsers() { //save to EEPROM

  int users_array_length = sizeof(users) /sizeof(users[0]);
  for (int i = 0; i < users_array_length; i++) {
    setEEPROMString(OFFSET_users + i * (mac_length + key_length),              mac_length, users[i].mac);
    setEEPROMString(OFFSET_users + i * (mac_length + key_length) + mac_length, key_length, users[i].key);
  }

  EEPROM.commit();
}

bool getUsers() { //load from server
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

    http.begin(host + "/api/users?offset=" + offset_url);
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
        return false;
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
      Serial.println("LOAD USERS AGAIN!!!!!!");
      Serial.println("LOAD USERS AGAIN!!!!!!");
      Serial.println("LOAD USERS AGAIN!!!!!!");
      Serial.println("LOAD USERS AGAIN!!!!!!");
      Serial.println("LOAD USERS AGAIN!!!!!!");
      Serial.println("LOAD USERS AGAIN!!!!!!");
      return false;
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
  return true;
}
