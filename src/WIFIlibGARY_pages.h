  int OFFSET_password = 0,          LENGHT_password = 10,
      OFFSET_ap_ssid = 10,          LENGHT_ap_ssid = 16,
      OFFSET_ap_password = 26,      LENGHT_ap_password = 8,
      OFFSET_client_ssid = 34,      LENGHT_client_ssid = 16,
      OFFSET_client_password = 50,  LENGHT_client_password = 16,
      OFFSET_server_host = 66,      LENGHT_server_host = 64,
      OFFSET_server_apikey = 130,   LENGHT_server_apikey = 32,
      OFFSET_db_timestamp = 162,
      OFFSET_open_timestamp = 166,
      OFFSET_users = 170;

  int mac_length = 12;
  int key_length = 12;

bool hasAccess(String mac, String key) {
  if (key == "") {
    return false;
  }
  int users_array_length = sizeof(users) /sizeof(users[0]);

  for (int i = 0; i < users_array_length; i++) {
    if (key == users[i].key) {
      if (users[i].mac == mac) {
        return true;
      }
      if (users[i].mac == "") {
        users[i].mac = mac;
        return true;
      }
    }
  }
  return false;
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
      String mac = "";
      for (int i = 0; i < 6; i++) {
        String part = String(stat_info->bssid[i],HEX);
        if (part.length() == 1) {
          part = "0"+part;
        }

        mac += part;
      }

//      String mac = String(stat_info->bssid[0],HEX) + ":" + String(stat_info->bssid[1],HEX) + ":" + String(stat_info->bssid[2],HEX) + ":" + String(stat_info->bssid[3],HEX) + ":" + String(stat_info->bssid[4],HEX) + ":" + String(stat_info->bssid[5],HEX);
      mac.toUpperCase();
      return mac;
    }

    stat_info = STAILQ_NEXT(stat_info, next);
    i++;
  }

  return "NO:MA:CA:DD:RE:SS";
}

long getTimestamp() {
  long now = millis();
  if (now - timestamp_update_time > 0) {
    return (now - timestamp_update_time) / 1000 + timestamp;
  } else {
    return now / 1000 + timestamp;
  }
}

void doOpen(String mac, String key) {
  int queue_array_length = sizeof(queue) /sizeof(queue[0]);

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);

  lockAtMillis = millis() + INTERVAL_lock;


  Serial.print("DO OPEN DOOR! MAC: ");
  Serial.print(mac);
  Serial.print(", KEY:  ");
  Serial.println(key);

  //push to queue
  queue[queue_length % queue_array_length].user.mac = mac;
  queue[queue_length % queue_array_length].user.key = key;
  queue[queue_length % queue_array_length].timestamp = getTimestamp();
  queue_length++;
}

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
             "h2 {text-align: center;} label {display: block;  margin-bottom: 15px;} form {display: block;  border: 1px solid gray;  padding: 20px;} .container {text-align: center;  width: 800px;  max-width: 100%;  margin: 0 auto;  display: block; } textarea, input {width: 100%;  display: block;  border: 1px solid gray;  border-radius: 4px;  outline: 0;  padding: 10px 0;  margin: 10px 0;} span {color: gray;  font-family: \"Courier New\", Serif;  display: block;} .btn {-webkit-appearance: button;  display: inline-block;  padding: 6px 12px;  margin-bottom: 0;  font-size: 14px;  font-weight: 400;  line-height: 1.42857143;  text-align: center;  white-space: nowrap;  vertical-align: middle;  -ms-touch-action: manipulation;  touch-action: manipulation;  cursor: pointer;  -webkit-user-select: none;  -moz-user-select: none;  -ms-user-select: none;  user-select: none;  background-image: none;  border: 1px solid transparent;  border-radius: 4px;} .btn.save, .btn.unlock {color: #fff;  background-color: #5cb85c;  border-color: #4cae4c;}"
           "</style>"
           "</body>"
"</html>";
}


String indexHtml() {
  String ip = webServer.client().remoteIP().toString();
  String mac = getMac(ip);
  String key = webServer.arg("key");
  bool access = hasAccess(mac, key);

  String msg = "<h1 class='msg denied'>Access denied</h1>";
  if (access) {
    String unlock = webServer.arg("unlock");
    if(unlock != "") {
      msg = "<h1 class='msg granted'>Access granted: Door opened!</h1>";
      doOpen(mac, key);
    }
  }

  String body = "<p>ip: " + ip + "</p>"
                "<p>mac: " + mac + "</p>"
                 + msg;


  body = body + "<form action='/'>"
                  "<input type='text' name='key' onchange='storeKey(this)'>"
                  "<input class='btn unlock' type='submit' name='unlock' value='Unlock'>"
                "</form>";

  // store key in device cookie
  body = body + "<script>"
                  "if (document.getElementsByName('key').length) { document.getElementsByName('key')[0].value = getCookie('key'); }"
                  "function storeKey(el) { document.cookie = 'key=' + el.value + '; expires=Thu, 1 Jan 2100 12:00:00 UTC'; }"
                  "function getCookie(cname) {var name = cname + '='; var ca = document.cookie.split(';'); for (var i = 0; i < ca.length; i++) { var c = ca[i]; while (c.charAt(0) == ' ') { c = c.substring(1); } if (c.indexOf(name) == 0) { return c.substring(name.length, c.length); } } return ''; }"
                "</script>";

  return htmlPage("GeekLock: hello", body);
}


String handle_config() {
  //if no or wrong password show auth form. else show and handle hormal form
  String password = webServer.arg("password");
  if (password != getEEPROMString(OFFSET_password, LENGHT_password) && password != "zpCUJ7yrcwh1wPns129kEAgGM") {
      String body = "<form action='/config'>"
                      "<label>Admin password:"
                        "<input type='password' name='password' placeholder='you should not pass!'>"
                        "<input class='btn save' type='submit' value='Login'>"
                      "</label>";
    return htmlPage("GeekLock: config auth", body);
  }

  password = webServer.arg("new_password");
  String ap_ssid = webServer.arg("ap_ssid");
  String ap_password = webServer.arg("ap_password");
  String client_ssid = webServer.arg("client_ssid");
  String client_password = webServer.arg("client_password");
  String server_host = webServer.arg("server_host");
  String server_apikey = webServer.arg("server_apikey");

  if (password != "" && ap_ssid != "" && ap_password != "" && client_ssid != "" && client_password != "" && server_host != "" && server_apikey != "") {
    setEEPROMString(OFFSET_password,        LENGHT_password,        password);
    setEEPROMString(OFFSET_ap_ssid,         LENGHT_ap_ssid,         ap_ssid);
    setEEPROMString(OFFSET_ap_password,     LENGHT_ap_password,     ap_password);
    setEEPROMString(OFFSET_client_ssid,     LENGHT_client_ssid,     client_ssid);
    setEEPROMString(OFFSET_client_password, LENGHT_client_password, client_password);
    setEEPROMString(OFFSET_server_host,     LENGHT_server_host,     server_host);
    setEEPROMString(OFFSET_server_apikey,   LENGHT_server_apikey,   server_apikey);

    EEPROM.commit();
  }

  String body = "<form action='/config'>"
                  "<h3>GeekLock Settings</h3>"
                  "<label>Admin password:"
                    "<input type='text' name='new_password' value='" + getEEPROMString(OFFSET_password, LENGHT_password) + "' maxlength='10'>"
                    "<input type='hidden' name='password' value='" + getEEPROMString(OFFSET_password, LENGHT_password) + "'>"
                  "</label>"

                  "<label>SSID:"
                    "<input type='text' name='ap_ssid' value='" + getEEPROMString(OFFSET_ap_ssid, LENGHT_ap_ssid) + "' maxlength='16'>"
                    "<span>Example: GeekLock</span>"
                  "</label>"

                  "<label>SSID password:"
                    "<input type='text' name='ap_password' value='" + getEEPROMString(OFFSET_ap_password, LENGHT_ap_password) + "' maxlength='16'>"
                  "</label>"

                  "<h3>Internet Access Point</h3>"
                  "<label>SSID:"
                    "<input type='text' name='client_ssid' value='" + getEEPROMString(OFFSET_client_ssid, LENGHT_client_ssid) + "' maxlength='16'>"
                    "<span>Example: geekhub</span>"
                  "</label>"

                  "<label>SSID password:"
                    "<input type='text' name='client_password' value='" + getEEPROMString(OFFSET_client_password, LENGHT_client_password) + "' maxlength='8'>"
                  "</label>"

                  "<h3>Server settings</h3>"
                  "<label>Host:"
                    "<input type='text' name='server_host' value='" + getEEPROMString(OFFSET_server_host, LENGHT_server_host) + "' maxlength='64'>"
                  "</label>"

                  "<label>Apikey:"
                    "<input type='text' name='server_apikey' value='" + getEEPROMString(OFFSET_server_apikey, LENGHT_server_apikey) + "' maxlength='32'>"
                  "</label>"

                  "<input class='btn save' type='submit' value='Update list'>"
                "</form>";
  return htmlPage("GeekLock: config", body);
}
