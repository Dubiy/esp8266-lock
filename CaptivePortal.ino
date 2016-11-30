#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif

#include <ESP8266WiFi.h>
#include "./DNSServer.h"                  // Patched lib
#include <ESP8266WebServer.h>
#include <EEPROM.h>


const byte        DNS_PORT = 53;          // Capture DNS requests on port 53
IPAddress         apIP(10, 10, 10, 1);    // Private network for server
DNSServer         dnsServer;              // Create the DNS object
ESP8266WebServer  webServer(80);          // HTTP server
int ledStatus = 0;

#include "./WIFIlibGARY_eeprom.h"
#include "./WIFIlibGARY_pages.h"


void setup() {
  EEPROM.begin(4096);
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  WiFi.mode(WIFI_AP);

  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
//  WiFi.softAP(getEEPROMString(OFFSET_ap_ssid, LENGHT_ap_ssid), getEEPROMString(OFFSET_ap_password, LENGHT_ap_password));

//  WiFi.mode(WIFI_AP);
  if (getEEPROMString(OFFSET_ap_password, LENGHT_ap_password).length() >= 8) {
    WiFi.softAP(getEEPROMString(OFFSET_ap_ssid, LENGHT_ap_ssid).c_str(), getEEPROMString(OFFSET_ap_password, LENGHT_ap_password).c_str());
  } else {
    WiFi.softAP(getEEPROMString(OFFSET_ap_ssid, LENGHT_ap_ssid).c_str());
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
}


