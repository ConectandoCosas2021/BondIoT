#include "automation_lib.h"			    // library header


void setLCD(LiquidCrystal_I2C lcd)
{
    lcd.init();	                        // initializes LCD
    lcd.backlight();		            // makes LCB screen backlit
    lcd.clear();			            // erases screen
    lcd.home();                         // brings cursor to home position
}


void printLCD(LiquidCrystal_I2C lcd, bool hasCustomMessage, char* customMessage, int qPas,int maxPas)
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

        if(qPas<maxPas)
        {
            lcd.home();                             // brings cursor to home position		
            lcd.print("Nro DE PASAJEROS:");         // prints text       
            lcd.setCursor(0, 1);		            // sets cursor to column 0 row 1
            lcd.print(qPas,DEC);                    // prints number of passengers in decimal
        }
        else
        {
            lcd.setCursor(0, 0);		        // sets cursor to column 0 row 0		
            lcd.print("OMNIBUS");	            // prints text
            lcd.setCursor(0, 1);                // sets cursor to column 0 row 1
            lcd.print("LLENO");	                // prints text      
        }
    }
}