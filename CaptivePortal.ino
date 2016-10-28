#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif

/*
   Captive Portal by: M. Ray Burnette 20150831
   See Notes tab for original code references and compile requirements
   Sketch uses 300,640 bytes (69%) of program storage space. Maximum is 434,160 bytes.
   Global variables use 50,732 bytes (61%) of dynamic memory, leaving 31,336 bytes for local variables. Maximum is 81,920 bytes.
*/

#include <ESP8266WiFi.h>
#include "./DNSServer.h"                  // Patched lib
#include <ESP8266WebServer.h>

const byte        DNS_PORT = 53;          // Capture DNS requests on port 53
IPAddress         apIP(10, 10, 10, 1);    // Private network for server
DNSServer         dnsServer;              // Create the DNS object
ESP8266WebServer  webServer(80);          // HTTP server

//String responseHTML = ""
//                      "<!DOCTYPE html><html><head><title>CaptivePortal</title></head><body>"
//                      "<h1>Hello World!</h1><p>This is a captive portal example. All requests will "
//                      "be redirected here.</p>" 
//                      //webServer->client().remoteIP().toString() +
//                      "</body></html>";



String responseHTML() {
  client_status();
  return ""
                      "<!DOCTYPE html><html><head><title>CaptivePortal</title></head><body>"
                      "<h1>Hello World!</h1><p>This is a captive portal example. All requests will "
                      "be redirected here.</p><h1>ip: " +
                      webServer.client().remoteIP().toString() +
                      "</h1></body></html>";

  }

void wifi_event_handler_cb(System_Event_t * event)
{
  Serial.print(event->event);
  Serial.println(" evented");
//    ehConsolePort.print(EVENT_NAMES[event->event]);
//    ehConsolePort.print(" (");
//    

        char mac[32] = {0};
        snprintf(mac, 32, MACSTR ", aid: %d" , MAC2STR(event->event_info.sta_connected.mac), event->event_info.sta_connected.aid);
        Serial.println(mac);

        Serial.print("ip ");
        char ipp[32] = {0};
        snprintf(ipp, 32, IPSTR , IP2STR(event->event_info.got_ip.ip.addr));
        Serial.println(ipp);

    switch (event->event) {
      case 5555: {
        //connected
        char mac[32] = {0};
        snprintf(mac, 32, MACSTR ", aid: %d" , MAC2STR(event->event_info.sta_connected.mac), event->event_info.sta_connected.aid);
        Serial.println(mac);

        Serial.print("ip ");
        char ipp[32] = {0};
        snprintf(ipp, 32, IPSTR , IP2STR(event->event_info.got_ip.ip.addr));
        Serial.println(ipp);
        
       

        
//        Serial.println(IP2STR(event->event_info.got_ip.ip.addr));
        //Serial.println(ipToString(event->event_info.got_ip.ip));
      } break;
      case 6: {
        //disconnect
        } break;
//        case EVENT_STAMODE_CONNECTED:
//            break;
//        case EVENT_STAMODE_DISCONNECTED:
//            break;
//        case EVENT_STAMODE_AUTHMODE_CHANGE:
//            break;
//        case EVENT_STAMODE_GOT_IP:
//            break;
//        case EVENT_SOFTAPMODE_STACONNECTED:
//        case EVENT_SOFTAPMODE_STADISCONNECTED:
//            {
//                char mac[32] = {0};
//                snprintf(mac, 32, MACSTR ", aid: %d" , MAC2STR(event->event_info.sta_connected.mac), event->event_info.sta_connected.aid);
//                
//                ehConsolePort.print(mac);
//            }
//            break;
    }
//
//    ehConsolePort.println(")");
}



void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("DNSServer CaptivePortal example");

  wifi_station_dhcpc_start();
  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);

  // replay to all requests with same HTML
  webServer.onNotFound([]() {
    webServer.send(200, "text/html", responseHTML());
  });
  webServer.begin();

//  wifi_set_event_handler_cb(wifi_event_handler_cb);
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
}


void client_status() {
  unsigned char number_client;
  struct station_info *stat_info;

  struct ip_addr *IPaddress;
  IPAddress address;
  int i=1;

  number_client= wifi_softap_get_station_num(); // Count of stations which are connected to ESP8266 soft-AP
  stat_info = wifi_softap_get_station_info();

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
