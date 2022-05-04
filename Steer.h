#pragma once

#include <Wire.h>
#include <Zumo32U4.h>
#include <Arduino.h>
#include <avr/pgmspace.h>


/*
Definerer variabler som trengs i biblioteket
*/
class Steer{
  public:
    int desLinSpeed = 100;
    int linePos;
    float rotSpeed;
    float rotGain;
    Steer();
    setLeft(int linePos, int desLinSpeed);
    setRight(int linePos, int desLinSpeed);
  private:
};
