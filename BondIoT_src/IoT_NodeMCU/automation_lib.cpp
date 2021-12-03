#include "automation_lib.h"			    // library header

// ---------- Display ----------
    void setLCD(LiquidCrystal_I2C &lcd)
    {
        lcd.init();	                        // initializes LCD
        lcd.backlight();		            // makes LCB screen backlit
        lcd.noCursor();                     // turns the cursor off
        lcd.clear();			            // erases screen
        lcd.home();                         // brings cursor to home position
    }


    void printLCD(LiquidCrystal_I2C &lcd, bool hasCustomMessage, char* customMessage, int qPas,int maxPas)
    {
        if(hasCustomMessage)
        {
            lcd.home();                         // brings cursor to home position		
            lcd.print(customMessage);	        // prints text  
        }
        else
        {
            lcd.home();                         // brings cursor to home position		
            lcd.print("192 MANGA");	            // prints text

            if(qPas < maxPas)
            {
                lcd.setCursor(0, 1);                    // sets cursor to column 0 row 1	
                lcd.print("PASAJEROS:");                // prints text       
                lcd.print(qPas);                    // prints number of passengers
            }
            else
            {
                lcd.setCursor(0, 1);		        // sets cursor to column 0 row 1		
                lcd.print("OMNIBUS LLENO");	            // prints text
            }
        }
    }
//-

// ---------- Open windows ----------

    void openWindowsSign(bool state, int pin)
    {
        if (state)
            digitalWrite(pin, HIGH);
        else
            digitalWrite(pin, LOW);
    }

//-