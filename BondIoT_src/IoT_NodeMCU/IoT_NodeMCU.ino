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

  #include "iot_functions.h"
  #include "reading_sensors_lib.h"
  #include "automation_lib.h"
//----------------------------------------------------------------------------



// =====================================
//              THINGSBOARD
// =====================================
  #define NODE_NAME "a2457060-1fee-11ec-b4a5-cfb289af38d9" //MAJO
  #define NODE_TOKEN "NSo9BArnHowXfhTi9Xku" //MAJO

  //#define NODE_NAME "aa0a47f0-3b6c-11ec-a8d7-7db293f1afb9" //SEBA
  //#define NODE_TOKEN "ynQ3BwFFN1dfS8aYuXMa" //SEBA

  #define NODE_PW NULL

  #define TB_SERVER "demo.thingsboard.io"

  #define telemetryTopic "v1/devices/me/telemetry"
  #define requestTopic "v1/devices/me/rpc/request/+"
  #define attributesTopic "v1/devices/me/attributes"

  #define JBUFFER 1024

// =====================================
//            COMMUNICATIONS
// =====================================
  #define WiFi_connect_attempts 10
  #define TB_connect_attempts 5
  #define TELEMETRY_UPDATE_INTERVAL 1000    // Time in milliseconds

// =====================================
//               I/O PINS
// =====================================
  #define MQ2_PIN A0              //co2 analog
  #define WINDOW_SIGN_LEDS D0     //LEDs from open windows sign
//Pin D1, D2 -> i2C               //display
  #define SERVO D3                //servo
  #define FULL_BUS_LED D4         //LED for bus driver
  #define LOADCELL_DOUT_PIN D5    //loadcell data
  #define LOADCELL_SCK_PIN D6     //loadcell clock
  #define FRONTDOOR_OUT_PIN D7    //IR frontdoor
  #define BACKDOOR_OUT_PIN D8     //IR backdoor
//----------------------------------------------------------------------------
 


// =====================================
//            GLOBAL VARIABLES
// =====================================

// ----------- communications ------------
  String wifi_SSID = mySSID;
  String wifi_PASSWORD = myPASSWORD;
  bool WiFi_OK = false;
  bool TB_OK = false;
  unsigned long lastTelemetryUpdate = 0;
//-

// --------------- control ---------------
  HX711 scale;
  Servo myServo;

  LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
  String busMessage = "";            //initialize only. Value comes from thingsboard.
  String busNameNum = "192 MANGA";   //initialize only. Value comes from thingsboard.

  String calibrationMode = "ON";        //initialize only. Value comes from thingsboard.
  unsigned int loadcell_timeout = 1000; //initialize only. Value comes from thingsboard.
  int weightVariation = 30;             //initialize only. Value comes from thingsboard.
  float weight_for_calibration = 500;   //initialize only. Value comes from thingsboard.
  float calibration_constant = 1;    
  float last_weight = 0;

  unsigned int passengers = 0;

  const int openedPos = 90;
  const int closedPos = 0; 

  bool alarmCO2 = false;           //initialize only. Value comes from thingsboard.

  bool reachedMaxPass = false;     //initialize only. Value comes from thingsboard.
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
DynamicJsonDocument generateJsonPayload();

void thingsBoard_cb(const char* topic, byte* payload, unsigned int length);

void wifiDebugSetup();
//----------------------------------------------------------------------------



// =====================================
//                 SETUP
// =====================================
void setup() {
  Serial.begin(115200);

  // ---------- wifi debug mode ------------
    if (WIFI_DEBUG == 1) wifiDebugSetup();
  //-

  // -------------- pin mode ---------------
    pinMode(FULL_BUS_LED, OUTPUT);
    pinMode(WINDOW_SIGN_LEDS, OUTPUT);
  //-
  
  // ------------ interruptions ------------
  	pinMode(FRONTDOOR_OUT_PIN, INPUT_PULLUP);
  	pinMode(BACKDOOR_OUT_PIN, INPUT_PULLUP);
  	attachInterrupt(digitalPinToInterrupt(FRONTDOOR_OUT_PIN), read_frontdoor, FALLING);
  	attachInterrupt(digitalPinToInterrupt(BACKDOOR_OUT_PIN), read_backdoor, FALLING);
  //-	 

  // ---------- servo, scale, co2 ----------
    myServo.attach(SERVO);
    myServo.write(closedPos);

    scale = setUpLoadCell(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  	
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
    
  if (WiFi.status() != WL_CONNECTED){
    WiFi_OK = connectToWiFi(wifi_SSID, wifi_PASSWORD, WiFi_connect_attempts);   // Connect to WiFi access point
    if (WiFi_OK) 
      TB_OK = connectToThingsBoard(TB_SERVER, NODE_NAME, NODE_TOKEN, NODE_PW, TB_connect_attempts);    // If WiFi connected successfully, connect to ThingsBoard 
      // Subscribe to topics and setup callback function
      if (TB_OK){
        client.subscribe(requestTopic);
        client.subscribe(attributesTopic);
        client.setCallback(thingsBoard_cb);
      }
  }

  if (WiFi_OK && TB_OK){
    client.loop();

    if (millis() - lastTelemetryUpdate > TELEMETRY_UPDATE_INTERVAL){
      sendValues(telemetryTopic, generateJsonPayload());   // If connected, send data to ThingsBoard
      lastTelemetryUpdate = millis();
    }
  }

}
//----------------------------------------------------------------------------



// =====================================
//            IMPLEMENTATIONS
// =====================================
void wifiDebugSetup(){
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


DynamicJsonDocument generateJsonPayload(){
  DynamicJsonDocument out(JBUFFER);

  out["co2"] = read_co2(MQ2_PIN); //random(1024);
  out["doors"] = passengers;

  //update loadcell telemetry ONLY if weight varies enough
  String loadcell_reading = read_weight(scale, loadcell_timeout);
  if (!loadcell_reading.equals("HX711 not found.")){
    if (loadcell_reading.toInt() - last_weight >= weightVariation){
      out["loadcell"] = String(loadcell_reading);
      last_weight = loadcell_reading.toInt();
    }
  }  

  return out;
}
//----------------------------------------------------------------------------



// =====================================
//           CALLBACK FUNCTION
// =====================================

// This callback function is called by de client.loop routine every time
// a topic, to which this device is subscibed to, is updated with new data.
void thingsBoard_cb(const char* topic, byte* payload, unsigned int length){
  
  // DEBUG MSG
  debug("Message received on topic: ");
  debug(topic);
  debug(" ==> Payload: ");
  for (int i = 0; i < length; i++){
    debug((char)payload[i]);
  }
  debugln();

  // Convert topic to string to be parsed
  String cb_topic = String(topic);

  // ---------------------------------------
  //           MANAGE RPC REQUESTS 
  // ---------------------------------------

  // If the topic is of type RPC REQUEST do the following
  if (cb_topic.startsWith("v1/devices/me/rpc/request/")){

    //We are in a request, save request number
    String response_number = cb_topic.substring(26);

    //Read JSON Object
    DynamicJsonDocument in_message(256);
    deserializeJson(in_message, payload);

    // Extract method to be executed
    String method = in_message["method"];
    

    // --------- method "switchLED" ----------
    /*  if (method == "switchLED"){

        bool state = in_message["params"]; //read parameter

        if (state) {
          digitalWrite(LED_PIN, LOW); //turn on led
        } else {
          digitalWrite(LED_PIN, HIGH); //turn off led
        }

        // Update attribute "ledStatus" to let ThingsBoard know the new led state
        DynamicJsonDocument resp(256);
        resp["ledStatus"] = state;
        char buffer[256];
        serializeJson(resp, buffer);
        client.publish("v1/devices/me/attributes", buffer);

        // DEBUG MSG
        debug("Message sent on atribute: ledStatus");
        debug(" ==> Payload: ");
        debug(buffer);
      } */
    //-

    // --------- method "setHatch" ----------
      if (method == "setHatch"){ 
        bool state = in_message["params"];

        if (state){
          Serial.println("Abriendo escotilla");
          myServo.write(openedPos);        
        }
        else{
          Serial.println("Cerrando escotilla");
          myServo.write(closedPos);
        }

        //Attribute update
        DynamicJsonDocument resp(256);
        resp["hatchState"] = state;
        char buffer[256];
        serializeJson(resp, buffer);
        client.publish("v1/devices/me/attributes", buffer);      
      }        
    //-      
  }


  // ---------------------------------------
  //         MANAGE ATTRIBUTE UPDATES 
  // ---------------------------------------
  
  if (cb_topic.equals("v1/devices/me/attributes")){
    //String attribute_id = cb_topic.substring(24);  //We are in a request, check request number

    //Read JSON Object
    DynamicJsonDocument in_message(256);
    deserializeJson(in_message, payload);

    // --------- shared attribute " weightForCalibration " ----------
      String tb_weight_for_calibration = String(in_message["weightForCalibration"]); //read from thingsboard

      if (tb_weight_for_calibration != "null") //if new value then update esp variable
        weight_for_calibration = (float) tb_weight_for_calibration.toInt();
    //-

    // --------- shared attribute " calibrationModeLoadCell " ----------
      String tb_calibrationMode = in_message["calibrationModeLoadCell"];

      if (tb_calibrationMode != "null"){
        calibrationMode = String(tb_calibrationMode);
        calibrationMode.toUpperCase();

        if (calibrationMode.equals("ON")){
          calibration_constant = calibrateLoadCell(scale, weight_for_calibration);
          scale.set_scale(calibration_constant); 

          //Attribute update
            DynamicJsonDocument resp(256);
            resp["calibrationModeLoadCell"] = "OFF";
            char buffer[256];
            serializeJson(resp, buffer);
            client.publish("v1/devices/me/attributes", buffer);         
          //-            
        }       
      }

    // --------- shared attribute " loadCellTimeOut " ----------
      String tb_loadcell_timeout = in_message["loadCellTimeOut"];

      if (tb_loadcell_timeout != "null"){
        loadcell_timeout = tb_loadcell_timeout.toInt();
      } 
    //- 

    // --------- shared attribute " alarmStateCO2 " ----------  
      String tb_alarmCO2 = in_message["alarmCO2"];

      if (tb_alarmCO2 != "null"){
        if (tb_alarmCO2.equals("true")){
          manageLEDs(true, WINDOW_SIGN_LEDS);
          myServo.write(openedPos);   //open hatch

          DynamicJsonDocument resp(256);
          resp["hatchState"] = true;
          char buffer[256];
          serializeJson(resp, buffer);
          client.publish("v1/devices/me/attributes", buffer);

        }else{
          manageLEDs(false, WINDOW_SIGN_LEDS);        
        }
      }
    //-

    // --------- shared attribute " busMessage AND busNameNum " ----------  
      String tb_busMessage = in_message["busMessage"];
      String tb_busNameNum = in_message["busNameNum"];

      if (tb_busNameNum != "null") {
        busNameNum = tb_busNameNum;
      }
      if (tb_busMessage != "null"){ 
        busMessage = tb_busMessage;
      }

      printLCD(lcd, busNameNum, busMessage);
    //-

    // --------- shared attribute " weightVariation " ----------
      String tb_weightVariation = in_message["weightVariation"];

      if (tb_weightVariation != "null"){
        weightVariation = tb_weightVariation.toInt();
      } 
    //

    // --------- shared attribute " reachedMaxPass " ----------
      String tb_reachedMaxPass = in_message["reachedMaxPass"];

      if (tb_reachedMaxPass != "null"){
        reachedMaxPass = tb_reachedMaxPass.equals("true");
        manageLEDs(reachedMaxPass, FULL_BUS_LED);
      }       
    //
      
  }

}
