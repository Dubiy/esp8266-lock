#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif

#include <ESP8266WiFi.h>
#include "./DNSServer.h"                  // Patched lib
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "./WIFIlibGARY.h"

const byte        DNS_PORT = 53;          // Capture DNS requests on port 53
IPAddress         apIP(10, 10, 10, 1);    // Private network for server
DNSServer         dnsServer;              // Create the DNS object
ESP8266WebServer  webServer(80);          // HTTP server


String htmlPage(String title, String body) {
  return "<!DOCTYPE html><html lang=\"en\">"
         "<head>"
           "<meta charset=\"UTF-8\">"
           "<title>" + title + "</title>"
         "</head>"
         "<body>" + 
           "<h2>" + title + "</h2>" +
           body +
           "<style>" 
             "h2 {text-align: center;}label {display: block;margin-bottom: 15px;}form {width: 800px;max-width: 100%;margin: 0 auto;display: block;border: 1px solid red;padding: 20px;}textarea {width: 100%;display: block;border: 1px solid gray;border-radius: 4px;outline: 0;padding: 10px 0;margin: 10px 0;}span {color: gray;font-family: \"Courier New\", Serif;display: block;}"
           "</style>"
           "</body>"
"</html>";
}

String handle_config() {
  String mac_list = webServer.arg("mac_list");
  if(mac_list != "") {
    setEEPROMString(0, 4096, mac_list);
    EEPROM.commit();
  }
  
  String body = "<form action='/config'>"
                  "<label>Mac addresses:"
                    "<textarea name='mac_list'>" + getEEPROMString(0, 4096) + "</textarea>"
                    "<span>Enter MAC addresses in format A1:B2:C3:D4:E5:F6. Record per line</span>"
                  "</label>"
                  "<input type='submit' value='Update list'>"
                "</form>";
  return htmlPage("GeekLock: config", body);
}



void client_status(String myIP) {
  unsigned char number_client;
  struct station_info *stat_info;

  struct ip_addr *IPaddress;
  IPAddress address;
  int i=1;

  number_client= wifi_softap_get_station_num(); // Count of stations which are connected to ESP8266 soft-AP
  stat_info = wifi_softap_get_station_info();

  Serial.print("myIP = ");
  Serial.println(myIP);

  Serial.print(" Total connected_client are = ");
  Serial.println(number_client);

  while (stat_info != NULL) {
    IPaddress = &stat_info->ip;
    address = IPaddress->addr;

    Serial.print("client= ");

    Serial.print(i);
    Serial.print(" ip adress is = ");
    Serial.print((address));
    Serial.print(" with mac adress is = ");

    Serial.print(stat_info->bssid[0],HEX);
    Serial.print(stat_info->bssid[1],HEX);
    Serial.print(stat_info->bssid[2],HEX);
    Serial.print(stat_info->bssid[3],HEX);
    Serial.print(stat_info->bssid[4],HEX);
    Serial.print(stat_info->bssid[5],HEX);

    stat_info = STAILQ_NEXT(stat_info, next);
    i++;
    Serial.println();
  }
  delay(500);
}




















String getMac(String myIP) {
  unsigned char number_client;
  struct station_info *stat_info;

  struct ip_addr *IPaddress;
  IPAddress address;
  int i=1;

  number_client= wifi_softap_get_station_num(); // Count of stations which are connected to ESP8266 soft-AP
  stat_info = wifi_softap_get_station_info();

  Serial.print("myIP = ");
  Serial.println(myIP);

  Serial.print(" Total connected_client are = ");
  Serial.println(number_client);

  while (stat_info != NULL) {
    IPaddress = &stat_info->ip;
    address = IPaddress->addr;

    String ipp = address.toString();

    if (ipp == myIP) {
      return stat_info->bssid;   
    }

    Serial.print("client= ");

    Serial.print(i);
    Serial.print(" ip adress is = ");
    Serial.print((address));
    Serial.print(" with mac adress is = ");

    Serial.print(stat_info->bssid[0],HEX);
    Serial.print(stat_info->bssid[1],HEX);
    Serial.print(stat_info->bssid[2],HEX);
    Serial.print(stat_info->bssid[3],HEX);
    Serial.print(stat_info->bssid[4],HEX);
    Serial.print(stat_info->bssid[5],HEX);

    stat_info = STAILQ_NEXT(stat_info, next);
    i++;
    Serial.println();
  }
  delay(500);
}





























String indexHtml() {
client_status(webServer.client().remoteIP().toString());
//  setEEPROMString(0, 250, "000 Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book$");
//  setEEPROMString(250, 500, "001 Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book$");
//  setEEPROMString(500, 750, "002 Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book$");
//  setEEPROMString(750, 1000, "003 Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book$");
//  setEEPROMString(1000, 1250, "004 Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book$");
//  setEEPROMString(1250, 1500, "005 Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book$");
//  setEEPROMString(1500, 1750, "006 Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book$");
//  setEEPROMString(1750, 2000, "007 Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book$");

//String custome = getEEPROMString(0, 250) + getEEPROMString(250, 500) + getEEPROMString(500, 750) + getEEPROMString(750, 1000) + getEEPROMString(1000, 1250) + getEEPROMString(1250, 1500) + getEEPROMString(1500, 1750) + getEEPROMString(1750, 2000);
  
//  client_status();
    return ""
                      "<!DOCTYPE html><html><head><title>CaptivePortal</title></head><body>"
                      "<h1>Hello World!</h1><p>This is a captive portal example. All requests will "
                      "be redirected here.</p>"
                      "<p>" 
//                      getEEPROMString(0, 250) + getEEPROMString(250, 500) + getEEPROMString(500, 750) + getEEPROMString(750, 1000) + getEEPROMString(1000, 1250) + getEEPROMString(1250, 1500) + getEEPROMString(1500, 1750) + getEEPROMString(1750, 2000) +
                      "</p>"
                      "<h1>ip: " +
                      webServer.client().remoteIP().toString() +
                      "</h1></body></html>";
}




void setup() {
  EEPROM.begin(4096);
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);

  String _ssid = getEEPROMString(0, 32);
  
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("DNSServer CaptivePortal example");

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


