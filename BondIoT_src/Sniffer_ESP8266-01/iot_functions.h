#define MAC_LEN 6
#define JBUFFER 1024

String MACs[] = {"00:f4:8d","80:fd:7a","c0:9f:e1","2c:d9:74","3e:84:6a","11:22:33","aa:bb:cc"};
int totalMACs = 7;

//String MACs[MAX_CLIENTS_TRACKED];
//int totalMACs = 0;


WiFiClient wifiClient;
PubSubClient client(wifiClient);


bool connectToWiFi(String AP_SSID, String AP_PASSWORD, int MAX_ATTEMPTS){
  int attempts = 1;

  debugln();
  debug("Connecting to ");
  debug(AP_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(AP_SSID, AP_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED && attempts < MAX_ATTEMPTS) {
    attempts++;
    delay(1000);
    debug(".");
  }

  if (WiFi.status() == WL_CONNECTED){
    debugln();
    debug("WiFi connected in ");
    debug(attempts);
    debugln(" attempts");
    debugln("IP address: ");
    debugln(WiFi.localIP());
    return true;
  }else{
    debugln();
    debugln("Error connecting to WiFi - Connection attempt timeout");
    return false;
  }
}


bool connectToThingsBoard(char* TB_SERVER, char* NODE_NAME, char* NODE_TOKEN, char* NODE_PW, int MAX_ATTEMPTS){
  int attempts = 1;

  client.setServer(TB_SERVER, 1883);

  while (!client.connected() && attempts < MAX_ATTEMPTS) {
    debug("Connecting to ThingsBoard...");
    if (!client.connect(NODE_NAME, NODE_TOKEN, NODE_PW)) {     // Attempt to connect (clientId, username, password)
      debug("Connection attempt failed - rc = ");
      debug(client.state());
      debugln(" : retrying in 1 second" );
      delay(1000);    // Wait 1 seconds before retrying
      attempts++;
    }
  }

  if (client.connected()){
    debugln();
    debug("Connected successfully in ");
    debug(attempts);
    debugln(" attempts");
    return true;
  }else{
    debugln();
    debugln("Error connecting to ThingsBoard - Connection attempt timeout");
    return false;
  }
}


void sendValues(char* topic, String name, String value){
  bool publishOK = false;

  DynamicJsonDocument data(JBUFFER);
  data[name] = value;"['00:f4:8d','80:fd:7a','c0:9f:e1','2c:d9:74','3e:84:6a','11:22:33','aa:bb:cc']"; 

  char payload[1024];
  serializeJson(data, payload);

  publishOK = client.publish(topic, payload);
  
  if (publishOK){ 
    debug("Successfully published on topic: ");
    debugln(topic);
    debug("Payload: ");
    debugln(payload);
  }else {
    debugln();
    debugln("!!!!! Not published. Please add #define MQTT_MAX_PACKET_SIZE 2048 at the beginning of PubSubClient.h file");
    debugln();
  }
}


void purgeDevice(int purgetime){
  for (int u = 0; u < clients_known_count; u++) {
    if ((millis() - clients_known[u].lastDiscoveredTime) > purgetime) {
      //debug("purge Client" );
      //debugln(u);
      for (int i = u; i < clients_known_count; i++) memcpy(&clients_known[i], &clients_known[i + 1], sizeof(clients_known[i]));
      clients_known_count--;
      break;
    }
  }
  for (int u = 0; u < aps_known_count; u++) {
    if ((millis() - aps_known[u].lastDiscoveredTime) > purgetime) {
      //debug("purge Bacon" );
      //debugln(u);
      for (int i = u; i < aps_known_count; i++) memcpy(&aps_known[i], &aps_known[i + 1], sizeof(aps_known[i]));
      aps_known_count--;
      break;
    }
  }
}


char *signalQuality(signed rssi){
  if (rssi < -90)                return "■▬▬▬▬▬▬▬";
  if (rssi < -80 && rssi >= -90) return "■■▬▬▬▬▬▬";
  if (rssi < -70 && rssi >= -80) return "■■■▬▬▬▬▬";
  if (rssi < -60 && rssi >= -70) return "■■■■▬▬▬▬";
  if (rssi < -50 && rssi >= -60) return "■■■■■▬▬▬";
  if (rssi < -40 && rssi >= -50) return "■■■■■■▬▬";
  if (rssi < -30 && rssi >= -40) return "■■■■■■■▬";
  if (rssi >= -30)               return "■■■■■■■■";
  else                           return "";
}


void showDevices(){
  debug("\n\n");
  debugln("-------------------Device DB-------------------");
  debugf("%4d Devices + Clients.\n",aps_known_count + clients_known_count); // show count

  // show Beacons
  for (int u = 0; u < aps_known_count; u++) {
    if (aps_known[u].isNew){
      debug("#");
      aps_known[u].isNew = false;
    }else{
      debug(" ");
    }
    debugf("%3d ",u); // Show beacon number
    debug("BEACON --> ");
    debug(formatMac1(aps_known[u].bssid));
    debug("| RSSI(");
    debug(aps_known[u].rssi);
    debugf(") %8s", signalQuality(aps_known[u].rssi));
    debug("| channel ");
    debugf("%2d", aps_known[u].channel);
    //debugf(" -- %32s\n", aps_known[u].ssid);
    debug(" -- ");
    for(int i=0;i<aps_known[u].ssid_len;i++){debugf("%c", aps_known[u].ssid[i]);}
    debugln("");
  }
  
  debugln("");
  
  // show Clients
  for (int u = 0; u < clients_known_count; u++) {
    if (clients_known[u].isNew){
      debug("#");
      clients_known[u].isNew = false;
    }else{
      debug(" ");
    }
    debugf("%3d ",u); // Show client number
    debug("CLIENT <-- ");
    debug(formatMac1(clients_known[u].station));
    debug("| RSSI(");
    debug(clients_known[u].rssi);
    debugf(") %8s", signalQuality(clients_known[u].rssi));
    debug("| channel ");
    //debug("| RSSI ");
    //debug(clients_known[u].rssi);
    //debug(" channel ");
    debugln(clients_known[u].channel);
  }
}


void getClients(clientinfo clients_known[], int len, unsigned int digits){
  String newMAC = "";
  uint8_t aux;

  if (digits > MAC_LEN) digits = MAC_LEN;

  for(int i = 0; i < len; i++){
    for(int j = 0; j < digits; j++){
      aux = clients_known[i].station[j];
      if (aux < 16) newMAC += "0";
      newMAC += String(aux, HEX);
      if (j < digits-1) newMAC += ":";
    }
    newMAC += "'";
    MACs[i] = newMAC;
  }
  totalMACs = len;
}

// String getClients(clientinfo clients_known[], int len, unsigned int digits){
//   String out;
//   uint8_t aux;
//   if (digits > MAC_LEN) digits = MAC_LEN;
//   //out = "{'total_clients':" + String(len) + ", ";
//   //out += "'MACs':[";
//   out += "[";
//   for(int i = 0; i < len; i++){
//     out += "'";
//     for(int j = 0; j < digits; j++){
//       aux = clients_known[i].station[j];
//       if (aux < 16) out += "0";
//       out += String(aux, HEX);
//       if (j < digits-1) out += ":";
//     }
//     out += "'";
//     if (i < len-1) out += ",";
//   }
//   //out += "]}";
//   out += "]";

//   debugln("-------------- DEBUG --------------");
//   debugln("Redacted MACs to be sent:");
//   debugln(out);
//   debugln("-----------------------------------");
  
//   return out;
// }

// String showHelp(){
//   String man = "";
//   man += ">>>>> SNIFFER MANUAL <<<<<\n";
//   man += "> HELP/MAN/?   - Shows this manual\n";
//   man += "> GET_STATUS   - Returns \"OK\" if there is active communication\n";
//   man += "> GET_CLIENTS  - Returns redacted MACs of client devices in json format\n";
//   man += "> SHOW_DEVICES - Shows a full list of client and beacon devices with details\n";
//   man += "\n\n";
//   return man;
// }


// void manageMsg(String msg){

//   msg.toUpperCase();

//   if (msg == "HELP" || msg == "MAN" || msg == "?") Serial.print(showHelp());

//   if (msg == "GET_STATUS") Serial.println("OK");

//   if (msg == "GET_CLIENTS") Serial.println(getClients(clients_known, clients_known_count, 3));

//   //if (msg == "SHOW_DEVICES") showDevices();
// }