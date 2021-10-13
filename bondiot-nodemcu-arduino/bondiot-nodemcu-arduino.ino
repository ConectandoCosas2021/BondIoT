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


// =====================================
//               LIBRARIES
// =====================================
#include <ESP8266WiFi.h> 
#include <PubSubClient.h> 
#include <ArduinoJson.h>
#include <string.h>

//#include "PubSubClient.h"
//#include "ArduinoJson.h"
#include "sniffer_functions.h"
#include "iot_functions.h"
#include "reading_sensors_lib.h"


// =====================================
//                 DEFINE
// =====================================
#define disable 0
#define enable  1
#define SENDTIME 5000                   // Cloud report frequency
#define MAXDEVICES 60
#define JBUFFER 15+ (MAXDEVICES * 40)
#define PURGETIME 20000                 // Max time a device can be undetected before is considered to not be in range anymore
#define MINRSSI -70                     // Minimum acceptable signal intensity to consider a device as "inside the bus"

// thingsboard
#define NODE_NAME "a2457060-1fee-11ec-b4a5-cfb289af38d9"
#define NODE_TOKEN "NSo9BArnHowXfhTi9Xku"
#define NODE_PW NULL

#define TB_SERVER "demo.thingsboard.io"

#define telemetryTopic "v1/devices/me/telemetry"
#define requestTopic "v1/devices/me/rpc/request/+"
#define attributesTopic "v1/devices/me/attributes"

#if WIFI_DEBUG != 1
#define mySSID "DEFAULT_SSID"
#define myPASSWORD "DEFAULT_PASSWORD"
#else
#define mySSID ""
#define myPASSWORD ""
#endif

#define WiFi_connect_attempts 10
#define TB_connect_attempts 5
#define RECEVIE_TIMEOUT 5000    // Time in milliseconds

// ---------- DEFINE PINS ----------
#define MQ2_PIN A0
#define LOADCELL_DOUT_PIN 2
#define LOADCELL_SCK_PIN 3
#define FRONTDOOR_OUT_PIN 7
#define BACKDOOR_OUT_PIN 8

// =====================================
//            GLOBAL VARIABLES
// =====================================
unsigned int channel = 1;
int clients_known_count_old, aps_known_count_old;
unsigned long sendEntry, deleteEntry;
char jsonStringOut[JBUFFER];
char* jsonStringIn;

String device[MAXDEVICES];
int nbrDevices = 0;
int usedChannels[15];

char* topic = telemetryTopic;
String wifi_SSID = mySSID;
String wifi_PASSWORD = myPASSWORD;
bool WiFi_OK = false;
bool TB_OK = false;

HX711 scale;
String calibrationMode = "OFF";		 // !!!!!!!! ESTO TIENE QUE TRAERSE DE THINGSBOARD
unsigned int loadcell_timeout = 1000; // !!!!!!!! ESTO TIENE QUE TRAERSE DE THINGSBOARD
unsigned int weight_variation = 10;  // !!!!!!!! ESTO TIENE QUE TRAERSE DE THINGSBOARD
unsigned int last_weight = 0;
unsigned int passengers = 0;

//StaticJsonBuffer<JBUFFER>  jsonBuffer;


// =====================================
//            INTERRUPT FUNCTIONS
// =====================================
void read_frontdoor()
{
	passengers++;
}

void read_backdoor()
{
	passengers--;
}


// =====================================
//                 SETUP
// =====================================
void setup() {
  Serial.begin(115200);
  if (WIFI_DEBUG == 1){
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

  wifi_set_opmode(STATION_MODE);            // Promiscuous works only with station mode
  wifi_set_channel(channel);
  wifi_promiscuous_enable(disable);
  wifi_set_promiscuous_rx_cb(promisc_cb);   // Set up promiscuous callback
  wifi_promiscuous_enable(enable);
  
	/*// ---------- interruptions ----------
	pinMode(FRONTDOOR_OUT_PIN, INPUT_PULLUP);
	pinMode(BACKDOOR_OUT_PIN, INPUT_PULLUP);
	attachInterrupt(FRONTDOOR_OUT_PIN, read_frontdoor, RISING);
	attachInterrupt(BACKDOOR_OUT_PIN, read_backdoor, RISING);
	//-	 
	*/
  
	/*if (calibrationMode.equals("ON")){
		calibrateLoadCell(scale);
	}*/
	//HX711 scale = setUpLoadCell(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  Serial.println("ANTES DEL BEGIN");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);   
  //pinMode(LOADCELL_SCK_PIN, OUTPUT);
  //pinMode(LOADCELL_DOUT_PIN, INPUT_PULLUP);  
  //scale.set_scale();
  //scale.tare(); //Reset the scale to 0 
  Serial.println("paso setup celda");
  
	//delay(2000); //loadcell warm-up
}


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
    
  }//endwhile
  
	//read CO2 level
	unsigned int readingCO2 = read_co2(MQ2_PIN);
  
	//read weight and send to thingsboard only when there is a significant variation
	/*String readingLoadCellStr = read_weight(scale, loadcell_timeout);
	int readingLoadCell = readingLoadCellStr.toInt();
	if (readingLoadCell != 0){		
		if (readingLoadCell-last_weight >= weight_variation){
			//add to json
		}			
		last_weight = readingLoadCell;
	}else{
		Serial.println(readingLoadCell);
	}*/

  if (scale.wait_ready_timeout(loadcell_timeout)) {
      long reading = scale.get_units(10);
      Serial.print("Weight: ");
      Serial.println(reading, 2);
  } else {
      Serial.println("HX711 not found.");
  }

	//create and send json
  if (millis() - sendEntry > SENDTIME) {
    sendEntry = millis();
    //showDevices();
    
    //jsonString = generateJson();
    
    wifi_promiscuous_enable(disable);   // Disable promiscuous mode in order to connect to the access point
           
    WiFi_OK = connectToWiFi(wifi_SSID, wifi_PASSWORD, WiFi_connect_attempts);   // Connect to WiFi access point
    
    if (WiFi_OK) TB_OK = connectToThingsBoard(TB_SERVER, NODE_NAME, NODE_TOKEN, NODE_PW, TB_connect_attempts);    // If WiFi connected successfully, connect to ThingsBoard
  
    topic = telemetryTopic;

    String test = "{'co2': ";
    long ran = random(200);
    //test += String(ran); 
    test += String(readingCO2);
    test += ", 'loadcell': ";
    //ran = random(100);
    test += String(100);
    test += "}";
    debugln(test);
  
    char payload[100];
    test.toCharArray(jsonStringOut, JBUFFER);
          
    if (WiFi_OK && TB_OK) sendValues(topic, jsonStringOut);    // If connected, send data to ThingsBoard
  
    //jsonStringIn = receiveData(topic, RECEVIE_TIMEOUT);
  
    client.disconnect ();   // Disconnect from ThingsBoard
    WiFi.disconnect();    // Disconnect from WiFi
    wifi_promiscuous_enable(enable);    // Re-enable promiscuous mode
  
  }  
  //thingsboard communication
  //si se recibieron instrucciones ejecutar accion, else vuelvo arriba a medir CO2  
}
