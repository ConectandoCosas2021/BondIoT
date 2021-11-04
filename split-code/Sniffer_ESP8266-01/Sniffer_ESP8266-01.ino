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
#include <Servo.h>

#include "sniffer_functions.h"
#include "iot_functions.h"
#include "reading_sensors_lib.h"
#include "automation_lib.h"
//----------------------------------------------------------------------------


// =====================================
//                SNIFFER
// =====================================
#define disable 0
#define enable  1
#define SENDTIME 5000                   // Cloud report frequency
#define MAXDEVICES 60
#define JBUFFER 15+ (MAXDEVICES * 40)
#define PURGETIME 20000                 // Max time a device can be undetected before is considered to not be in range anymore
#define MINRSSI -70                     // Minimum acceptable signal intensity to consider a device as "inside the bus"

// =====================================
//              THINGSBOARD
// =====================================
//#define NODE_NAME "a2457060-1fee-11ec-b4a5-cfb289af38d9" //MAJO
//#define NODE_TOKEN "NSo9BArnHowXfhTi9Xku" //MAJO

#define NODE_NAME "aa0a47f0-3b6c-11ec-a8d7-7db293f1afb9" //SEBA
#define NODE_TOKEN "ynQ3BwFFN1dfS8aYuXMa" //SEBA

#define NODE_PW NULL

#define TB_SERVER "demo.thingsboard.io"

#define telemetryTopic "v1/devices/me/telemetry"
#define requestTopic "v1/devices/me/rpc/request/+"
#define attributesTopic "v1/devices/me/attributes"

// =====================================
//            COMMUNICATIONS
// =====================================
#define WiFi_connect_attempts 10
#define TB_connect_attempts 5
#define RECEVIE_TIMEOUT 10000    // Time in milliseconds

// =====================================
//               I/O PINS
// =====================================
#define MQ2_PIN A0              //co2 analog
#define LOADCELL_DOUT_PIN D5    //loadcell data
#define LOADCELL_SCK_PIN D6     //loadcell clock
#define FRONTDOOR_OUT_PIN D7     //IR frontdoor
#define BACKDOOR_OUT_PIN D8      //IR backdoor
#define LED_PIN 16               //NodeMCU built in LED
#define SERVO D1                //servo
//----------------------------------------------------------------------------


// =====================================
//            GLOBAL VARIABLES
// =====================================
unsigned int channel = 1;
unsigned long sendEntry;
char* jsonIn;
//StaticJsonBuffer<JBUFFER>  jsonBuffer;

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
float weight_for_calibration = 500;  //!!!!!!!! ESTO TIENE QUE TRAERSE DE THINGSBOARD
unsigned int last_weight = 0;
unsigned int passengers = 0;
float calibration_constant = 1;
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
//----------------------------------------------------------------------------


// =====================================
//            INTERRUPT FUNCTIONS
// =====================================
ICACHE_RAM_ATTR void read_frontdoor()
{
	passengers++;
	debugln("Front door interrupting");
}

ICACHE_RAM_ATTR void read_backdoor()
{
	passengers--;
	debugln("Back door interrupting");
}
//----------------------------------------------------------------------------


// =====================================
//               FUNCTIONS
// =====================================
DynamicJsonDocument generateJsonPayload(){
  DynamicJsonDocument out(JBUFFER);

  out["co2"] = random(1024);//read_co2(MQ2_PIN);
  out["loadcell"] = random(20000);//read_weight(scale, loadcell_timeout);
  out["doors"] = passengers;
  //out["MACs"] = getClients(clients_known, clients_known_count);

  return out;
}


void thingsBoard_cb(const char* topic, byte* payload, unsigned int length){
  
  debug("Message received on topic: ");
  debug(topic);
  debug(" ==> payload: ");
  for (int i = 0; i < length; i++){
    debug((char)payload[i]);
  }
  debugln();

  String cb_topic = String(topic);  //convert topic to string to parse
  
  if (cb_topic.startsWith("v1/devices/me/rpc/request/")){
    String response_number = cb_topic.substring(26);  //We are in a request, check request number

    //Read JSON Object
    DynamicJsonDocument in_message(256);
    deserializeJson(in_message, payload);
    String method = in_message["method"];
    
    if (method == "switchLED"){

      bool state = in_message["params"];

      if (state) {
        digitalWrite(LED_PIN, LOW); //turn on led
      } else {
        digitalWrite(LED_PIN, HIGH); //turn off led
      }

      //Attribute update
      DynamicJsonDocument resp(256);
      resp["ledStatus"] = state;
      char buffer[256];
      serializeJson(resp, buffer);
      client.publish("v1/devices/me/attributes", buffer);

      debug("Message sent on atribute: ledStatus");
      debug(" ==> payload: ");
      debug(buffer);
    } 
  } 
}
//----------------------------------------------------------------------------


// =====================================
//                 SETUP
// =====================================
void setup() {

  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // ------------- debug mode --------------
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
  
  // ----------------- wifi ----------------
    wifi_set_opmode(STATION_MODE);  // Promiscuous works only with station mode
    wifi_set_channel(channel);
    wifi_promiscuous_enable(disable);
    wifi_set_promiscuous_rx_cb(promisc_cb); // Set up promiscuous callback
    wifi_promiscuous_enable(enable);
  //-
  
  // ------------ interruptions ------------
  	pinMode(FRONTDOOR_OUT_PIN, INPUT_PULLUP);
  	pinMode(BACKDOOR_OUT_PIN, INPUT_PULLUP);
  	attachInterrupt(digitalPinToInterrupt(FRONTDOOR_OUT_PIN), read_frontdoor, RISING);
  	attachInterrupt(digitalPinToInterrupt(BACKDOOR_OUT_PIN), read_backdoor, RISING);
  //-	 

  // ---------- servo, scale, co2 ----------
   /* myServo.attach(SERVO);
    scale = setUpLoadCell(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);  
  	if (calibrationMode.equals("ON")){
  		calibration_constant = calibrateLoadCell(scale, weight_for_calibration);
      scale.set_scale(calibration_constant);
      }*/
  	
  	delay(2000); //co2 warm-up
  //-

  //------------- lcd setup ----------------
  setLCD(lcd);
}
//----------------------------------------------------------------------------


// =====================================
//                 LOOP
// =====================================
void loop() {

  //for(channel = 1; channel < 15; channel++){  // Only scan channels 1 to 14
  //  for(int wait = 0; wait < 300; wait++);  // wait while monitor channel for 300 cicles
  //  wifi_set_channel(channel);
  //}
  /*
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
	//unsigned int readingCO2 = read_co2(MQ2_PIN);
  //String readingLoadCellStr = read_weight(scale, loadcell_timeout);

  if (millis() - sendEntry > SENDTIME) {
    sendEntry = millis();
    //showDevices();  //Prints MAC addressses to the serial monitor    
    //jsonString = generateJson();  
  */
    // Disable promiscuous mode in order to connect to the access point 
    wifi_promiscuous_enable(disable);
             
    
    if (!WiFi_OK)
    WiFi_OK = connectToWiFi(wifi_SSID, wifi_PASSWORD, WiFi_connect_attempts);   // Connect to WiFi access point
    

    if (WiFi_OK) 
      TB_OK = connectToThingsBoard(TB_SERVER, NODE_NAME, NODE_TOKEN, NODE_PW, TB_connect_attempts);    // If WiFi connected successfully, connect to ThingsBoard 
     

    if (WiFi_OK && TB_OK){
      sendValues(telemetryTopic, generateJsonPayload());   // If connected, send data to ThingsBoard
      receiveData(requestTopic, RECEVIE_TIMEOUT);
    }
  /*
    client.disconnect ();   // Disconnect from ThingsBoard
    WiFi.disconnect();    // Disconnect from WiFi
    wifi_promiscuous_enable(enable);    // Re-enable promiscuous mode
  
  }//end if sendtime
  */

  //move servo
  //moveServo(myServo, servoState, openedPos, closedPos);

  //printLCD(lcd, false, "HOLA JUAN CARLOS", 10, 20);
      
}
//----------------------------------------------------------------------------