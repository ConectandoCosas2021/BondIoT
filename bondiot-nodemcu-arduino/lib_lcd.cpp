#include "lib_lcd.h"			        // library header




void setLCD(LiquidCrystal_I2C lcd)
{
    lcd.setBacklightPin(3,POSITIVE);	//  P3 port of PCF8574 as VCC
    lcd.setBacklight(HIGH);		        // Makes LCB screen backlit
    lcd.begin(16, 2);			        // 16 x 2 size for LCD 1602A
    lcd.clear();			            // erases screen
}

void printLCD(LiquidCrystal_I2C lcd, bool hasCustomMessage, char* customMessage, int qPas,int maxPas)
{
    if(hascustomMessage)
    {
        if(strlen(customMessage)>16)
        {
            char*messageH[12];                        //Create buffer for first part of text
            char*messageL[12] = customMessage + 12;   //Pointer to second part of text
            strncpy(messageH, customMessage, 12);     //Copy first part on buffer.
            lcd.setCursor(0, 0);		              // sets cursor to column 0 row 0		
            lcd.print(messageH);	                  // prints text
            lcd.setCursor(0, 1);		              // sets cursor to column 0 row 1		
            lcd.print(messageL);	                  // prints text
            delay(2000);			                  // two seconds of delay
        }
        else
        {
            lcd.setCursor(0, 0);		        // sets cursor to column 0 row 0		
            lcd.print(customMessage);	        // prints text
            delay(2000);			            // two seconds of delay
        }    
    }
    else
    {
        lcd.setCursor(0, 0);		        // sets cursor to column 0 row 0		
        lcd.print("192 MANGA");	            // prints text
        delay(1000);			            // a second of delay
        if(qPas<maxPas)
        {
            lcd.setCursor(0, 0);		         // sets cursor to column 0 row 0		
            lcd.print("CANTIDAD DE PASAJEROS:"); // prints text       
            lcd.setCursor(0, 1);		        // sets cursor to column 0 row 1
            lcd.print(qPas,DEC);                // prints number of passengers in decimal
            delay(1000);			            // a second of delay
        }
        else
        {
            lcd.setCursor(0, 0);		        // sets cursor to column 0 row 0		
            lcd.print("OMNIBUS");	            // prints text
            lcd.setCursor(0, 1);                // sets cursor to column 0 row 1
            lcd.print("LLENO");	                // prints text
            delay(1000);			            // a second of delay        
        }
    }
}