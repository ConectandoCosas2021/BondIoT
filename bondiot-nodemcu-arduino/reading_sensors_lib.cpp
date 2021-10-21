#include "reading_sensors_lib.h"

// ---------- CO2 ----------

unsigned int read_co2(unsigned int analogPin)
{
	delay(2000); // wait 2s for next reading
	return analogRead(analogPin);
}

//-

// ---------- SERVO ----------

void moveServo(Servo &myServo, String servoState, int openedPos, int closedPos)
{
  if (servoState.equals("OPEN"))
    if (myServo.read() == closedPos) myServo.write(openedPos);
  else
    if (myServo.read() == openedPos) myServo.write(closedPos);  
}
   
//-    

// ---------- LOADCELL ----------

HX711 setUpLoadCell(unsigned int loadcell_dout_pin, unsigned int loadcell_sck_pin)
{
	HX711 scale;
  
	scale.begin(loadcell_dout_pin, loadcell_sck_pin);	  
	scale.set_scale();
	scale.tare(); //Reset the scale to 0

  return scale;
}

String read_weight(HX711 scale, unsigned int loadcell_timeout)
{
	String msg = "";
		
	if (scale.wait_ready_timeout(loadcell_timeout)){
		long reading = scale.get_units(10); //10 measures
		msg = (String) reading;
	}else{
		msg = "HX711 not found.";
	}
	
	return msg;
}

float calibrateLoadCell(HX711 &scale, unsigned int weight_for_calibration)
{
	scale.set_scale();
	scale.tare();
  delay(3000); 
  
	int result = scale.get_units(10);
	Serial.print("Measure = ");
	Serial.println(result);
 
  float adjusted_value = result/weight_for_calibration;
  Serial.print("Adjusted value: ");
  Serial.println(adjusted_value);

  return adjusted_value;
}

//-
