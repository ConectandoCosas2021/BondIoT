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


    void printLCD(LiquidCrystal_I2C &lcd, String firstLine, String secondLine)
    {
        lcd.clear();                        // erases screen
        lcd.home();                     // brings cursor to home position	
        lcd.print(firstLine);	        // prints text  
        lcd.setCursor(0, 1);
        lcd.print(secondLine);
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