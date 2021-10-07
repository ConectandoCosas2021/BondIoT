#include "reading_sensors_lib.h"

// ---------- CO2 ----------

unsigned int read_co2(unsigned int analogPin)
{
	delay(2000); // wait 2s for next reading
	return analogRead(analogPin);
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
		long reading = scale.get_units(10);
		msg = (String) reading;
	}else{
		msg = "HX711 not found.";
	}
	
	return msg;
}

void calibrateLoadCell(HX711 scale)
{
	scale.set_scale();
	scale.tare();
	Serial.println("Place known weight on the scale");
	int result = scale.get_units(10);
	Serial.print("Measure = ");
	Serial.print(result);
	Serial.println("Devide the measure by the known weight and type it:");
  	if (Serial.available() > 0) {
    	scale.set_scale(Serial.read());
    }
    Serial.println("Adjust the last result until getting an accurate reading."); 
}

//-

