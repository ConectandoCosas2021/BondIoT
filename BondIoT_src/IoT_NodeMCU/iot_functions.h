// Taken from PubSubClient in the fuction "setCallback"
// Defines a generic function whith the needed parameters and the alias "callBackFunction" in this case
// The alias "callBackFunction" is used to set the cb function in setCallback through the input parameter "CALLBACK_FUNCTION" of the function "setupCallBack"
//#define CALLBACK_FUNCTION std::function<void(char*, uint8_t*, unsigned int)> callBackFunction


WiFiClient wifiClient;
PubSubClient client(wifiClient);

char* receivedData = "";


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


void sendValues(char* topic, DynamicJsonDocument jsonOut){
  bool publishOK = false;

  char payload[1024];
  serializeJson(jsonOut, payload);

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


 void thingsBoard_cb(const char* topic, byte* payload, unsigned int length);//{
  
//   debug("Message received on topic: ");
//   debug(topic);
//   debug(" ==> payload: ");
//   for (int i = 0; i < length; i++) {
//     debug((char)payload[i]);
//   }
//   debugln();
  
// }
/*
void setupCallBack(CALLBACK_FUNCTION, char* topic){
  client.subscribe(topic);
  client.setCallback(callBackFunction);
}


/*
char* receiveData(char* topic, int timeout){
  unsigned long t_0 = millis();
  unsigned long t_act = millis();

  client.subscribe(topic);
  client.setCallback(thingsBoard_cb);

  while(t_act - t_0 < timeout){
    client.loop();
    t_act = millis();
  }

  return receivedData;    // Global variable modified by the callback function "process_cb"
}

/*
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


String getClients(clientinfo clients_known[], int len){
  String out;
  uint8_t aux;
  out = "{'total_clients':" + String(len) + ", ";
  out += "'MACs':[";
  for(int i = 0; i < len; i++){
    aux = clients_known[i].station[0];
    out += "'";
    if (aux < 16) out += "0";
    out += String(aux, HEX) + ":";
    aux = clients_known[i].station[1];
    if (aux < 16) out += "0";
    out += String(aux, HEX) + "'";
    if (i < len-1) out += ","; 
  }
  out += "]}";

  debugln("-----------------------------------");
  debugf("Redacted MACs to be sent:\n%s",out);
  debugln("-----------------------------------");
  
  return out;
}*/