#pragma once

#include <Wire.h>
#include <Zumo32U4.h>
#include <avr/pgmspace.h>
#include <arduino.h>
/*
Definerer alle funksjoner og variabler som brukes i biblioteket.
*/
class Speedo {
  public: 
  // We are using pointers to point to the object. Otherwise, the function doesn't know what it is grabbing. 
    getSpeed(Zumo32U4Encoders &encoders);     
    getBat(float spd, float batLevel); 
    displaySpeed(Zumo32U4LCD &display, float spd);
    displayBat(Zumo32U4LCD &display, float bat);          // pointing, then saying what variables it takes in. 
    unsigned long old_time;                               // doing this makes it so that it doesn't throw away the variables-
    int16_t batLevel;                                     // when it has been used.
    
};
