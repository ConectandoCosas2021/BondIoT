#ifndef LIB_LCD.H
#define LIB_LCD.H

#include <Arduino.h>
#include <Wire.h>			        // library for I2C comm
#include <LCD.h>			        // library for LCD comm
#include <LiquidCrystal_I2C.h>		// library to use LCD with I2C
#include <string.h>

/*
Default LCD declaration:
LiquidCrystal_I2C lcd (0x27, 2, 1, 0, 4, 5, 6, 7); // DIR, E, RW, RS, D4, D5, D6, D7 
*/

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