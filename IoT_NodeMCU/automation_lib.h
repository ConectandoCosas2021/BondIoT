#ifndef LIB_LCD_H
#define LIB_LCD_H

#include <LiquidCrystal_I2C.h>		// library to use LCD with I2C
#include <string.h>


/*
Default LCD declaration:
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

/*
Parameters: LiquidCrystal_I2C display.
This function is used to setup previously declared lcd screen form communication.
*/
void setLCD(LiquidCrystal_I2C lcd);

/*
Parameters: LiquidCrystal_I2C display.
            hasCustomMessage: indicates if custom message is displayed .
            customMessage: string to be displayed.
            qPas: Amount of passengers on board.
            maxPas: Passenger limit.
This function is used to print information on display.
*/
void printLCD(LiquidCrystal_I2C lcd, bool hasCustomMessage, char* customMessage, int qPas,int maxPas);


#endif