// =====================================
//                 DEBUG
// =====================================
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
//----------------------------------------------------------------------------


// =====================================
//               LIBRARIES
// =====================================
#include <ESP8266WiFi.h>
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
//----------------------------------------------------------------------------


// =====================================
//            GLOBAL VARIABLES
// =====================================
unsigned int channel = 1;
String serialMsg = "";
//----------------------------------------------------------------------------


// =====================================
//                 SETUP
// =====================================
void setup() {

  Serial.begin(115200);
  //pinMode(LED_PIN, OUTPUT);

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

    if (Serial.available()){
      serialMsg = Serial.readString();
      wifi_promiscuous_enable(disable);
      manageMsg(serialMsg);
      wifi_promiscuous_enable(enable);
    }
  }
}
//----------------------------------------------------------------------------