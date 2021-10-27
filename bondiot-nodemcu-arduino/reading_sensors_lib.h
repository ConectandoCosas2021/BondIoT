#ifndef READING_SENSORS_LIB.H
#define READING_SENSORS_LIB.H

#include <Arduino.h>
#include <HX711.h>
#include <Servo.h>

/* Parameters: MQ2 pin
   Returns: CO2 measure */
unsigned int read_co2(unsigned int analogPin);

/* Parameters: D out pin, SCK pin
   Returns: HX711 initialized object */
HX711 setUpLoadCell(unsigned int loadcell_dout_pin, unsigned int loadcell_sck_pin);

/* Parameters: HX711 object and a timeout for non-blocking mode
   Returns: Loadcell measure */
String read_weight(HX711 scale, unsigned int loadcell_timeout);

/* Parameters: HX711 object pointer
   Returns: Value to set_scale
   This function is used to calibrate the load cell */
float calibrateLoadCell(HX711 &scale, float weight_for_calibration);

/* Parameters: servoStatus (open or close), openedPos and closedPos are angles
 * Returns: -
 */
void moveServo(Servo &myServo, String servoState, int openedPos, int closedPos);

#endif
