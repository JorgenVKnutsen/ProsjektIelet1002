#include "SpeedLib.h"


/*
Henter inn hastighet og batterinivå. Bruker disse verdiene til å regne ut
energiforbruk det siste sekundet, og trekker det fra batterinivå
*/
Speedo :: getBat(float spd, float batLevel) {
  float usage = 10 + 2 * spd;
  batLevel -= usage ;
  return batLevel;  
}

/*
Henter rotasjonen til hjulene på zumoen siden sist ved å bruke Zumo32U4Encoders-bibliotek. Henter
verdier på begge hjul og regner ut gjennomsnitt. returnerer hastighet i cm/s 
*/
// Input argument er en pointer (&)                              
Speedo :: getSpeed(Zumo32U4Encoders &encoders) {      //counts rotation, then converts it to meter. Checks every second for speed/second. 
  uint16_t left = encoders.getCountsAndResetLeft();
  uint16_t right = encoders.getCountsAndResetRight();
  uint64_t totalRight = totalRight + right;
  float avgLR = (left + right) / 2;
  float spd = (avgLR / 7750) * 100;
  old_time = millis();
  return spd; 
}
/*
Tar i mot en fart, og sender det videre til displayet man kan feste på topp av Zumo.
Benytter seg av Zumo32U4LCD-biblioteket via pointer
*/
Speedo :: displaySpeed(Zumo32U4LCD &display, float spd) {
  display.clear();
  display.gotoXY(0, 0);
  display.print(spd);
  display.gotoXY(0, 6);
  display.print("cs");
}
/*
Fungerer på samme måte som displaySpeed, men med batteri som input og output
*/
Speedo :: displayBat(Zumo32U4LCD &display, float bat) {
  display.gotoXY(0, 1);
  display.print(bat);
  display.gotoXY(6, 1);
  display.print("%");
}
