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
           "<div class='container'>"
             "<h2>" + title + "</h2>" +
             body +
           "</div>"  
           "<style>" 
             "h2 {text-align: center;} label {display: block;  margin-bottom: 15px;} form {display: block;  border: 1px solid gray;  padding: 20px;} .container {text-align: center;  width: 800px;  max-width: 100%;  margin: 0 auto;  display: block; } textarea {width: 100%;  display: block;  border: 1px solid gray;  border-radius: 4px;  outline: 0;  padding: 10px 0;  margin: 10px 0;} span {color: gray;  font-family: \"Courier New\", Serif;  display: block;} .btn {-webkit-appearance: button;  display: inline-block;  padding: 6px 12px;  margin-bottom: 0;  font-size: 14px;  font-weight: 400;  line-height: 1.42857143;  text-align: center;  white-space: nowrap;  vertical-align: middle;  -ms-touch-action: manipulation;  touch-action: manipulation;  cursor: pointer;  -webkit-user-select: none;  -moz-user-select: none;  -ms-user-select: none;  user-select: none;  background-image: none;  border: 1px solid transparent;  border-radius: 4px;} .btn.save, .btn.unlock {color: #fff;  background-color: #5cb85c;  border-color: #4cae4c;}"
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
                  "<input class='btn save' type='submit' value='Update list'>"
                "</form>";
  return htmlPage("GeekLock: config", body);
}

String getMac(String myIP) {
  unsigned char number_client;
  struct station_info *stat_info;

  struct ip_addr *IPaddress;
  IPAddress address;
  int i=1;

  number_client= wifi_softap_get_station_num(); // Count of stations which are connected to ESP8266 soft-AP
  stat_info = wifi_softap_get_station_info();

  while (stat_info != NULL) {
    IPaddress = &stat_info->ip;
    address = IPaddress->addr;

    String ipp = address.toString();

    if (ipp == myIP) {
      String mac = String(stat_info->bssid[0],HEX) + ":" + String(stat_info->bssid[1],HEX) + ":" + String(stat_info->bssid[2],HEX) + ":" + String(stat_info->bssid[3],HEX) + ":" + String(stat_info->bssid[4],HEX) + ":" + String(stat_info->bssid[5],HEX);
      mac.toUpperCase();
      return mac;
    }

    stat_info = STAILQ_NEXT(stat_info, next);
    i++;
  }
  
  return "NO:MA:CA:DD:RE:SS";
}

bool hasAccess(String mac) {
  String macList = getEEPROMString(0, 4096);

  if (macList.indexOf(mac) >= 0) {
    return true;
  }

  return false;
}




String indexHtml() {
  String ip = webServer.client().remoteIP().toString();
  String mac = getMac(ip);
  bool access = hasAccess(mac);

  HTTPClient http;

  http.begin("http://garik.pp.ua/prj/esp8266/");
  int httpCode = http.GET();
  String payload = "[empty]";
  if(httpCode>0){ 
    payload = http.getString();
    Serial.println(payload);
    http.end();  
  } else {
    Serial.print("HTTP code ");
    Serial.println(httpCode);
  }
  






  

  String msg = "<h1 class='msg denied'>Access denied</h1>";
  if (access) {
    String unlock = webServer.arg("unlock");
    if(unlock != "") {
      msg = "<h1 class='msg granted'>Opened!</h1>";
      digitalWrite(2, LOW);   // turn the LED on (HIGH is the voltage level)
      delay(1000);              // wait for a second
      digitalWrite(2, HIGH);  
    } else {
      msg = "<h1 class='msg granted'>Access granted</h1>";  
    }
  }
  
  String body = "<p>ip: " + ip + "</p>"
                "<p>mac: " + mac + "</p>"
                 + msg;

  if (access) {
    body = body + "<form action='/'>"
                    "<input class='btn unlock' type='submit' name='unlock' value='unlock' value='Unlock'>"
                  "</form>";   
  } 
  
//  return htmlPage("GeekLock: hello", body);
  return htmlPage(payload, body);

//  payload
}




void setup() {
  EEPROM.begin(4096);
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  WiFi.mode(WIFI_AP_STA);

  String _ssid = getEEPROMString(0, 32);
  
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("GeekLock2");
  WiFi.begin("ekreative", "yabloka346");

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


