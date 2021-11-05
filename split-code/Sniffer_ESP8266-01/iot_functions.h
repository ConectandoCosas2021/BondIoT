
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
  debugf("Redacted MACs to be sent:\n%s\n",out);
  debugln("-----------------------------------");
  
  return out;
}