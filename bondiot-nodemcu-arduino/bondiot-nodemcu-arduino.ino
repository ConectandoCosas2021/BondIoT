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
#include <Servo.h>

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
#define MQ2_PIN A0              //co2 analog
#define LOADCELL_DOUT_PIN D5    //loadcell data
#define LOADCELL_SCK_PIN D6     //loadcell clock
#define FRONTDOOR_OUT_PIN 7     //IR frontdoor
#define BACKDOOR_OUT_PIN 8      //IR backdoor
#define SERVO D4                //servo

// =====================================
//            GLOBAL VARIABLES
// =====================================
unsigned int channel = 1;
int clients_known_count_old, aps_known_count_old;
unsigned long sendEntry, deleteEntry;
char jsonStringOut[JBUFFER];
char* jsonStringIn;
//StaticJsonBuffer<JBUFFER>  jsonBuffer;

String device[MAXDEVICES];
int nbrDevices = 0;
int usedChannels[15];

char* topic = telemetryTopic;
String wifi_SSID = mySSID;
String wifi_PASSWORD = myPASSWORD;
bool WiFi_OK = false;
bool TB_OK = false;

HX711 scale;
Servo myServo;
int openedPos = 90;
int closedPos = 0;
String calibrationMode = "ON";		 // !!!!!!!! ESTO TIENE QUE TRAERSE DE THINGSBOARD
String servoState = "OPEN";	// !!!!!!!! ESTO TIENE QUE TRAERSE DE THINGSBOARD
unsigned int loadcell_timeout = 1000; // !!!!!!!! ESTO TIENE QUE TRAERSE DE THINGSBOARD
unsigned int weight_variation = 10;  // !!!!!!!! ESTO TIENE QUE TRAERSE DE THINGSBOARD
unsigned int weight_for_calibration = 1;  //Kg !!!!!!!! ESTO TIENE QUE TRAERSE DE THINGSBOARD
unsigned int last_weight = 0;
unsigned int passengers = 0;
float calibration_constant = 1.f;

// =====================================
//            INTERRUPT FUNCTIONS
// =====================================
void read_frontdoor()
{
	passengers++;
	Serial.println("Front door interrupting");
}

void read_backdoor()
{
	passengers--;
	Serial.println("Back door interrupting");
}


// =====================================
//                 SETUP
// =====================================
void setup() {
  Serial.begin(115200);

  // ---------- debug mode ----------
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
  //-
  
  // ---------- wifi ----------
    wifi_set_opmode(STATION_MODE);  // Promiscuous works only with station mode
    wifi_set_channel(channel);
    wifi_promiscuous_enable(disable);
    wifi_set_promiscuous_rx_cb(promisc_cb); // Set up promiscuous callback
    wifi_promiscuous_enable(enable);
  //-
  
  // ---------- interruptions ----------
  	pinMode(FRONTDOOR_OUT_PIN, INPUT_PULLUP);
  	pinMode(BACKDOOR_OUT_PIN, INPUT_PULLUP);
  	attachInterrupt(FRONTDOOR_OUT_PIN, read_frontdoor, RISING);
  	attachInterrupt(BACKDOOR_OUT_PIN, read_backdoor, RISING);
  //-	 

  // ---------- servo, scale, co2 ----------
    myServo.attach(SERVO);
    scale = setUpLoadCell(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);  
  	if (calibrationMode.equals("ON")){
  		calibration_constant = calibrateLoadCell(scale, weight_for_calibration);
      scale.set_scale(calibration_constant);
  	}
  	delay(2000); //co2 warm-up
  //-

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

  //read co2 and loadcell sensors
	unsigned int readingCO2 = read_co2(MQ2_PIN);
  String readingLoadCellStr = read_weight(scale, loadcell_timeout);

  if (millis() - sendEntry > SENDTIME) {
    sendEntry = millis();
    //showDevices();  //Prints MAC addressses to the serial monitor    
    //jsonString = generateJson();  

    // Disable promiscuous mode in order to connect to the access point 
    wifi_promiscuous_enable(disable);             
    WiFi_OK = connectToWiFi(wifi_SSID, wifi_PASSWORD, WiFi_connect_attempts);   // Connect to WiFi access point    
    if (WiFi_OK) 
      TB_OK = connectToThingsBoard(TB_SERVER, NODE_NAME, NODE_TOKEN, NODE_PW, TB_connect_attempts);    // If WiFi connected successfully, connect to ThingsBoard 

    //create json to send to thingsboard
    String test = "{'co2': "; 
    test += String(readingCO2);
    test += ", 'loadcell': ";
    test += readingLoadCellStr;
    test += "}";
    debugln(test);
  
    char payload[100];
    test.toCharArray(jsonStringOut, JBUFFER);
    topic = telemetryTopic;              
    if (WiFi_OK && TB_OK) 
      sendValues(topic, jsonStringOut);    // If connected, send data to ThingsBoard
  
    //jsonStringIn = receiveData(topic, RECEVIE_TIMEOUT);
  
    client.disconnect ();   // Disconnect from ThingsBoard
    WiFi.disconnect();    // Disconnect from WiFi
    wifi_promiscuous_enable(enable);    // Re-enable promiscuous mode
  
  }//end if sendtime

  //move servo
  if (servoStatus.equals("OPEN"))
    if (servo.read() == closedPos) myServo.write(openedPos);
  else
    if (servo.read() == openedPos) myServo.write(closedPos);
      
}
