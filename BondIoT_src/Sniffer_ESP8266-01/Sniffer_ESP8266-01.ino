// =====================================
//                 DEBUG
// =====================================
  #define WIFI_DEBUG 1
  #define DEBUG 1

  #if DEBUG == 1
  #define debug(x) Serial.print(x)
  #define debugf(x,y) Serial.printf(x,y)
  #define debugln(x) Serial.println(x)
  #else
  #define debug(x)
  #define debugf(x,y)  
  #define debugln(x) 
  #endif

  #if WIFI_DEBUG == 1
  #define mySSID ""
  #define myPASSWORD ""
  #else
  #define mySSID "HUAWEI-IoT"
  #define myPASSWORD "ORTWiFiIoT"
  #endif
//----------------------------------------------------------------------------


// =====================================
//               LIBRARIES
// =====================================
  #include <ESP8266WiFi.h>
  #include <PubSubClient.h> 
  #include <ArduinoJson.h>
  #include <string.h>

  #include "sniffer_functions.h"
  #include "iot_functions.h"
//----------------------------------------------------------------------------


// =====================================
//                DEFINE
// =====================================
  #define disable 0
  #define enable  1
  #define MAXDEVICES 60
  #define PURGETIME 20000                 // Max time a device can be undetected before is considered to not be in range anymore
  #define MINRSSI -70                     // Minimum acceptable signal intensity to consider a device as "inside the bus"
  #define LED_PIN 1


// =====================================
//              THINGSBOARD
// =====================================
  #define NODE_NAME "380b62a0-4c8e-11ec-8c14-07468c6f7a3d"
  #define NODE_TOKEN "9T1exjdGQ7JTkFDtw0kY"

  //#define NODE_NAME "e5f19360-4cc7-11ec-bdee-5b1567981707" //SEBA
  //#define NODE_TOKEN "N0N2DiVEAtWOTp5LjEUa" //SEBA

  #define NODE_PW NULL

  #define TB_SERVER "demo.thingsboard.io"

  #define telemetryTopic "v1/devices/me/telemetry"
  #define attributesTopic "v1/devices/me/attributes"


// =====================================
//            COMMUNICATIONS
// =====================================
  #define WiFi_connect_attempts 10
  #define TB_connect_attempts 5
  #define TELEMETRY_UPDATE_INTERVAL 5000    // Time in milliseconds
//----------------------------------------------------------------------------


// =====================================
//            GLOBAL VARIABLES
// =====================================
  unsigned int channel = 1;
  String serialMsg = "";
  int lastTimeSent = 0;
  bool WiFi_OK = false;
  bool TB_OK = false;
  String wifi_SSID = mySSID;
  String wifi_PASSWORD = myPASSWORD;
//----------------------------------------------------------------------------


void wifiDebugSetup(){
  delay(5000);
  debugf("\n\n\n\n\n\nBondIoT - version: 1.1\n",NULL);
  debugf("SDK version: %s\n\n", system_get_sdk_version());
  debugln("========================");
  debugln("   DEBUG MODE ENABLED   ");
  debugln("========================");
  debugf("\n\n",NULL);
  debugln("Please input access point SSID");
  while(!Serial.available()) delay(1);
  wifi_SSID = Serial.readString();
  debugln("Please input access point PASSWORD");
  while(!Serial.available()) delay(1);
  wifi_PASSWORD = Serial.readString();
  debugf("Credentials saved successfully!\n\n\n\n\n",NULL);
}


// =====================================
//                 SETUP
// =====================================
void setup() {

  Serial.begin(115200);
  //pinMode(LED_PIN, OUTPUT);

  // ---------- wifi debug mode ------------
    if (WIFI_DEBUG == 1) wifiDebugSetup();
  //-

  wifi_set_opmode(STATION_MODE);  // Promiscuous works only with station mode
  wifi_set_channel(channel);
  wifi_promiscuous_enable(disable);
  wifi_set_promiscuous_rx_cb(promisc_cb); // Set up promiscuous callback
  wifi_promiscuous_enable(enable);
}
//----------------------------------------------------------------------------


// =====================================
//                 LOOP
// =====================================
void loop() {

  channel = 1;
  wifi_set_channel(channel);
  
  while (channel < 15) {                    // Only scan channels 1 to 14
    nothing_new++;                          // Array is not finite, check bounds and adjust if required
    if (nothing_new > 300) {       //200    // monitor channel for 200 ms
      nothing_new = 0;
      channel++;            
      wifi_set_channel(channel);
    }
    
    delay(1);                               // Critical processing timeslice for NONOS SDK! No delay(0) yield()

    purgeDevice(PURGETIME);                 // Delete old devices that are no longer in range

    // if (Serial.available()){
    //   serialMsg = Serial.readString();
    //   wifi_promiscuous_enable(disable);
    //   manageMsg(serialMsg);
    //   wifi_promiscuous_enable(enable);
    //}
  }

  if (millis() - lastTimeSent > TELEMETRY_UPDATE_INTERVAL){
    //digitalWrite(LED_PIN, LOW);
    wifi_promiscuous_enable(disable);

    getClients(clients_known, clients_known_count, 3);

    for (int i = 0; i < totalMACs; i++) debug(MACs[i]);
    debugln("");

    WiFi_OK = connectToWiFi(wifi_SSID, wifi_PASSWORD, WiFi_connect_attempts);   // Connect to WiFi access point

    if (WiFi_OK) 
      TB_OK = connectToThingsBoard(TB_SERVER, NODE_NAME, NODE_TOKEN, NODE_PW, TB_connect_attempts);    // If WiFi connected successfully, connect to ThingsBoard
    
    if (TB_OK && WiFi_OK){
      sendValues(telemetryTopic, "MACs", "New reading");
      delay(1000);
      //getClients(clients_known, clients_known_count, 3);

      for (int i = 0; i < totalMACs; i++){
        sendValues(telemetryTopic, "MACs", MACs[i]);
        delay(1000);
      }

      sendValues(telemetryTopic, "MACs", "End of reading");

      // DynamicJsonDocument data(JBUFFER);
      // data["MACs"] = "['00:f4:8d','80:fd:7a','c0:9f:e1','2c:d9:74','3e:84:6a','11:22:33','aa:bb:cc']"; //getClients(clients_known, clients_known_count, 3);
      // sendValues(telemetryTopic, data);
    }

    client.disconnect ();   // Disconnect from ThingsBoard
    WiFi.disconnect();    // Disconnect from WiFi
    lastTimeSent = millis();
    wifi_promiscuous_enable(enable);
    //digitalWrite(LED_PIN, HIGH);
  }
}
//----------------------------------------------------------------------------