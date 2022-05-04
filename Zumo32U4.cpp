//Inkluderer bibliotekene vi trenger

#include <Wire.h>
#include <Zumo32U4.h>
#include <EEPROM.h>

#include "steer.h"
#include "SpeedLib.h"

Steer steer;
Speedo speedo;

Zumo32U4LCD display;
Zumo32U4Motors motors;
Zumo32U4ButtonA button_A;
Zumo32U4ButtonB button_B;
Zumo32U4LineSensors lineSensors;
Zumo32U4Encoders encoders;
Zumo32U4ProximitySensors proxSensors;

Zumo32U4Buzzer buzzer;

// Variabler som berører zumoens bevegelser
int sensorValues[5];
int desLinSpeed = 150;
bool counting = false;
int adress = 0;
int linePos;
int mode = 0;

//Variabler som handler om kommunikasjon mot ESP32
String mailBox = "";
char message = ' ';
bool adress1 = false;
bool adress2 = false;
bool adress3 = false;
bool adress4 = false;
bool adress5 = false;

// Variabler som omhandler batterinivå/hastighet
int batHealth = EEPROM.read(0);
int batProsent = EEPROM.read(1);
unsigned long old_time = 0;
float maxBatCapacity = 10000;
float batCapacity = maxBatCapacity;
float batHurtFactor = 0.995;
float batLevel = batCapacity;
int batCount = 0;
String chargeOrService = "";
bool emCharged = false;


//Variabler som omhandler Joystick()
bool mode4 = false;
String joyVal = "";
int joySpeed = 0;

int turnFactor = 1;// faktor som sørger for at vi kjører ut av kryss samme retning som vi kjørte inn

int calibrated = 0;

// Funksjon som brukes til å kalibrere zumoens tolkning av linje-Sensorer
void calib() {
  lineSensors.initFiveSensors();
  proxSensors.initThreeSensors();
  encoders.init();
  unsigned long timer = millis();
  motors.setSpeeds(desLinSpeed, -desLinSpeed);// Gjør at bilen snurrer rundt seg selv
  while ((millis() < timer + 5000)) {// hvis kalibrert i 3,6 sek OG linjesensoren ser en strek under
    lineSensors.calibrate();                                                             // er kalibreringen ferdig
    delay(20);
  }
  motors.setSpeeds(0, 0);
}

//sjekker knappetrykk, returnerer true når knapp A trykkes
// Knapp A brukes bl.a. til å kalibrere bilen før bruk
bool buttonA() {
  if (button_A.isPressed()) {
    while (button_A.isPressed()) {}
    return true;
  } else {
    return false;
  }
}

// skjekker knappetrykk, returnerer true når knapp B trykkes
bool buttonB() {
  if (button_B.isPressed()) {
    while (button_B.isPressed()) {}
    return true;
  } else {
    return false;
  }
}

// Sjekkker om linePos tilsvarer at vi har linje under bilen
// hvis utenfor ønskede verdier returner "true"
bool white() {
  if (linePos <= 500 || linePos >= 3990) {
    return true;
  } else {
    return false;
  }
}


/*
  hvis vi har mistet veien, prøv å snurre rundt og finne veien igjen.
  Returnerer false hvis den ikke er på linja, returnerer først true
  når den er nesten midt på svart linje
*/
bool endOfRoad() {
  if (white()) {
    motors.setSpeeds(turnFactor * desLinSpeed, -turnFactor * desLinSpeed);
    return false;
  } else if (1800 < linePos || linePos < 2200) {
    return true;
  }
}

//Funksjon som kalles hvis vi ønsker at zumoen skal svinge til
//høyre ved neste mulighet
void goRight() {
  motors.setSpeeds(150, -100);
  delay(510);
}

//Denne funksjonen gjør det samme som goRight, bare til venstre
// høyre.
void goLeft() {
  motors.setSpeeds(-100, 150);
  delay(510);
}



// funksjonen for å kjøre under vanlig drift
int left = 0;
int right = 0;
void drive(int desLinSpeed) {
  left = steer.setLeft(linePos, desLinSpeed);// bruker steerLeft og steerRight i Steer-biblioteket for å finne hastigheter
  right = steer.setRight(linePos, desLinSpeed);
  intersection();
  motors.setSpeeds(left, right);// setter hastigheter til det biblioteket har regnet ut
}

/*
  Sjekker om bilen har en vei til høyre eller venstre ift hovedlinja den kjører på. Returnerer
  en verdi tilsvarende hvordan krysset ser ut. Brukes blandt annet når bilen skal inn til an adresse
  og gjøre en oppgave, men også når den skal ut av en blindvei i race-modus
*/
char intersection() {
  if (sensorValues[0] > 500 && sensorValues[4] > 500 && (1500 < linePos < 2500)) {
    return 'b';
  } else if (sensorValues[0] > 500 && (1500 < linePos < 2500)) {
    turnFactor = -1;
    return 'l';
  } else if (sensorValues[4] > 500 && (1500 < linePos < 2500)) {
    turnFactor = 1;
    return 'r';
  }
}

/*
  race()-funksjonen brukes når bilen kjører løp. Den bruker en egen funksjon til dette
  fordi den skal ignorere hull i veien(som til vanlig kan tilsvare en adresse), og
  fordi den må være i stand til å detektere blindveier og snu i neste kryss etterpå
*/
unsigned long timeOff;            //Må ha disse utenfor, blir påvirket av å ligge inne i race()
char inter;
bool blank;
bool deadEnd;
void race() {
  if (inter == 'r' && deadEnd == true) {
    goRight();
    deadEnd = false;
  } else if (inter == 'l' && deadEnd == true) {
    goLeft();
    deadEnd = false;
  }

  if (blank && (timeOff + 290 - desLinSpeed) < millis()) {
    if (endOfRoad()) {
      blank = false;
    } else if (timeOff + (750 - desLinSpeed) < millis()) {
      deadEnd = true;
    }
  } else if (white()) {
    blank = true;
    motors.setSpeeds(desLinSpeed, desLinSpeed);
  } else {
    drive(desLinSpeed);
    timeOff = millis();
    inter = intersection();
  }

}

/*
  Funksjonen sjekker for hindringer foran seg med proximity-sensorene. Disse er veldig følsomme,
  dermed brukes de kun i vanlig "linjefølger"-modus, fordi bl.a. garasjeveggene på ladestasjonen gjør
  at den ikke vil kjøre hvis checkForObst brukes.
*/
bool checkForObst() {
  proxSensors.read();
  float threshold = 6;
  float leftFrontVal = proxSensors.countsFrontWithLeftLeds();
  float rightFrontVal = proxSensors.countsFrontWithRightLeds();
  float leftVal = proxSensors.countsLeftWithLeftLeds();
  float rightVal = proxSensors.countsRightWithRightLeds();
  bool obstruction = leftVal >= threshold || rightVal >= threshold || leftFrontVal >= threshold || rightFrontVal >= threshold;

  if (obstruction) {
    return true;
  } else {
    return false;
  }

}


/*
  countAdress er funksjonen som er i stand til å registrere hvor i byen bilen befinner seg.
  Det ligger "hull" i veien, i form av vekselvis svart og ingen tape. countAdress er i stand
  til å telle antall endringer, og dermed komme opp med en adresse. Hvis zumoen ikke teller en
  tilstandsendring  140 ms etter første tilstandsendring, betyr dette at den er på en blindvei eller
  utenfor veien, og dermed settes adressen til -1. Dette vil igjen føre til at bilen begynner
  å lete etter en vei
*/
bool counter = true;
unsigned long adressTime;
int countAdress() {
  bool blank = white();//hvis mangler linje er blank "true"

  if ((blank) && (counting == false) && adress == 0) {    // når vi mister linja første gang aktiveres denne
    adress = 0;
    adressTime = millis();
    counting = true;                                      // counting settes til true, neste gang funksjonen kjøres
    counter = false;                                      // kommer vi til neste sløyfe
  } else if (counting) {                                  // Neste sløyfe, nå er tellingen i gang
    if ((adressTime + 140 < millis()) && adress == 0) {   //hvis vi ikke finner igjen svart teip, settes adressen til -1
      counting = false;                                   // kun hvis adress = 0, dvs. den kom aldri tilbake på svart linje
      adress = -1;
      return adress;
    } else if ((adressTime + 140) < (millis())) {          //hvis vi har en adresse, men ikke har endret tilstand(teip / ikke teip)
      counting = false;                                   // på 150 ms, stopper vi å telle, setter mode til 1, og returnerer adresse
      mode = 1;                                           // mode vil endres til 100 + adresse i en annen funksjon, mode = 1 kun for å
      // unngå at vi setter oss fast i mode 100
      return adress;
    } else if ((blank == false) && (counter == false)) {  // hvis vi har linje og ikke hadde det i stad(counter = false -> forrige var hvit)
      adressTime = millis();
      drive(desLinSpeed);
      adress++;                                           // adresse økes
      counter = true;
    } else if ((blank) && (counter)) {                    //hvis vi har linje og ikke hadde det i stad(motsatt av forrige else if)
      counter = false;
      adressTime = millis();
    }
  }
}

/*
  Funksjon for å sette fart på zumoen hvis den står i modus 4, altså gamepad-styring. Verdier hentes i
  checkMail og videresendes som variablene joyVal og joySpeed. Hvis joySpeed er over 300, vil det si at
  den er over maxfart(som er satt til 255). Dette betyr at bilen har mottatt fart + 1000, som vil si at
  den skal lage tutelyd. Koden trekker fra 1000, og joySpeed kan bestemme hastigheten, mens joyVal bestemmer
  retningen
*/
void joystick() {

  //Motorfart + 1000 betyr tut
  if (joySpeed > 300) {
    joySpeed -= 1000;
    buzzer.playNote(NOTE_A(4), 500, 10);
  }

  //Dobbelsikring mot motorkollaps
  if (joySpeed > 300 || joySpeed < -100) {
    joySpeed = 0;
  }
  if (joyVal == "jw") {
    motors.setSpeeds(joySpeed, joySpeed);
  } else if (joyVal == "ja") {
    motors.setSpeeds(0, joySpeed);
  } else if (joyVal == "jd") {
    motors.setSpeeds(joySpeed, 0);
  } else if (joyVal == "wa") {
    motors.setSpeeds(joySpeed * 0.5, joySpeed);
  } else if (joyVal == "wd") {
    motors.setSpeeds(joySpeed, joySpeed * 0.5);
  } else if (joyVal == "js") {
    motors.setSpeeds(0, 0);
  }
}

/*
  Denne funksjonen brukes til å sette modus på bilen, i form av at "mode" endres. Denne funskjonen
  jobber tett med execute, siden execute gjennomfører det en spesifikk modus krever, mens mye av
  endringen i modus skjer her. Modus-endringer skjer også hvis visse beskjeder mottas i checkMail()-funksjonen.
*/
void checkMode() {                                          // her sjekker vi for hendelser, og hvis en bestemt ting har skjedd, bør vi
  countAdress();                                            // sjekker om counting har blitt satt til true
  //Klar til å kjøre
  if (buttonA() && mode != 1) {                             //Bruker både denne og execute sin switch..case for å holde det "ryddig"
    adress = 0;                                             // da disse funksjonene er mer overordnet og "overstyrer" execute
    mode = 1;
  } else if (buttonB() && mode == 3) {
    mode = 1;                                               //velger en tilfeldig hastighet for å få variasjon i løpstid
  } else if (buttonB()) {
    mode = 0;
  } else if (mode4) {
    mode = 4;
  } else if (mode == 3) {
    mode = 3;
  } else if (adress == -1) {
    mode = 2;
  } else if (counting) {                                    // hvis counting er true, går vi til modus 100, som er "adressering"
    mode = 100;
  } else if (adress > 0) {
    mode = adress + 100;
  }
}

// Hovedkoden(skjelettet) ligger under her

/*
  funksjonen battery() vil hvert sekund bruke biblioteket SpeedLib til å sjekke hastighet, og
  deretter bruke hastigheten til å regne ut forbrukt energi. Dette er Zumoen sin hoveddel av
  SW-batteriet, men funksjonene under spiller også inn.
*/
void battery() {
  if (millis() > 1000 + old_time) {
    float spd = speedo.getSpeed(encoders);
    if (batProsent > 0 && !emCharged) {
      batLevel = speedo.getBat(spd, batLevel);
    } else if (emCharged) {
      speedo.getBat(spd, batLevel);
      emCharged = false;
    }
    batHealth = map(batCapacity, 0 , maxBatCapacity, 0, 100);
    batProsent = map(batLevel, 0, batCapacity, 0, 100);
    old_time = millis();
    if (lowBattery()) {                                         //Hvis batteri under 20% skades batteriet med 0,5% per sekund
      batCapacity = batCapacity * batHurtFactor;
    }
  }
  batLevel = constrain(batLevel, 0, batCapacity);

}

/*
  eeprom() vil lagre batterihelse og batteriprosent inne på EEPROM, dvs. det vil lagres selv
  om strømmen på zumoen blir borte. Dette er hensiktsmessig fordi det simulerer et batteri, og
  hvis det ikke lagres på EEPROM vil verdiene nullstilles hver gang man slår av Zumoen.
  Verdiene lagres hvert 30. sekund, årsakene til det er 1) EEPROM-minnet har et begrenset antall
  lagringer før den ødelegges og 2) å lagre på EEPROM tar relativt mye tid.
*/
unsigned long eepromTime = millis();
void eeprom() {
  if (eepromTime + 30000 < millis()) {
    EEPROM.write(0, batHealth);
    EEPROM.write(1, batProsent);
    eepromTime = millis();
  }
}

/*
  Sjekker om det er lavt batteri, returnerer eventuelt "true" eller "false". Piper hvert 5. sekund, pluss
  ett ekstra pip hvis den er helt tom for strøm. Batteriet tar skade av å gå under 20%, og
  årsaken til at lowBattery er en bool er at lowBattery sjekkes i battery(), og hvis "true" skades batteriet
  litt hvert sekund
*/
bool lowBattery() {
  if (batProsent == 0 && batCount == 0) {
    buzzer.playNote(NOTE_E(2), 1000, 10);
    batCount++;
    return true;
  } else if (batProsent <= 20) {
    batCount++;
    if (batCount >= 5) {
      batCount = 0;
      buzzer.playNote(NOTE_E(3), 500, 10);
      return true;
    }
  } else {
    return false;
  }
}
/*
  Lader opp batteriet, og sender mengde energi som ble brukt opp til MQTT via ESP. På
  grunn av intern motstand i batteriet, vil forbrukt energi aldri være mindre enn 1000
*/
void charge() {                                                 
  buzzer.playNote(NOTE_A(3), 100, 10);
  delay(10000);
  buzzer.playNote(NOTE_A(4), 100, 10);
  int lowBat = batLevel;
  batLevel = batCapacity;
  int batSent = batLevel - lowBat;
  batSent = constrain(batSent, 1000, 10000);
  Serial1.print(batSent);
}
/*
  gjør service på bilen. Dette vil si at batterikapasiteten flyttes tilbake til max,
  dermed blir batterihelsen 100%
*/
void service() {
  buzzer.playNote(NOTE_A(4), 100, 10);
  delay(10000);
  buzzer.playNote(NOTE_A(5), 100, 10);
  batCapacity = maxBatCapacity;
}
/*
  Under nødlading vil bilen snurre rundt mens den "lader", og batteriet vil økes med 20%
  Dette koster 2000 strøm selv hvis batterikapasiteten er lav, og batLevel ikke nødvendigvis
  fylles opp med 2000. (f.eks. hvis batHealth er 70% vil bilen bruke 2000 Wh, men kun motta 1400)
*/
void emergencyCharge() {
  buzzer.playNote(NOTE_A(4), 100, 10);
  motors.setSpeeds(desLinSpeed, -desLinSpeed);
  delay(4700);
  if (!white()) {
    //Må finne batterilevel som brukes på snurring for å ikke ta med det etter utladning
    emCharged = true;
    buzzer.playNote(NOTE_A(3), 100, 10);
    batCapacity = batCapacity * 0.90;
    batLevel += batCapacity * 0.20;
    Serial1.print("2000");
  }
}
/*
  Execute-funksjonen er funksjonen der en modus blir oversatt i en handling. Modus 1-5 er "standardmoduser",
  modus 100 er når vi sjekker for adresse, og hver adresse har egen modus: 100 + adresse
  Denne måten å gjøre programmet på gir mye fleksibilitet, hvis vi f.eks. hadde måttet legge til en ny adresse,
  eller at 2 adresser bytter plass med hverandre. Funksjonen overlapper mye med CheckMode, men det er greit å dele
  opp i 2 funksjoner, siden dette gjør det litt mer oversiktlig. Funksjonen ble likevel veldig lang, men består
  kun av en switch-case, så det er ikke så enkelt å dele opp/ korte ned
*/
void execute() {
  //I execute har vi de forskjellige modusene til zumoen, og hva hver modus innebærer
  switch (mode) {
    case 0:                                         //Modus "stå rolig", f.eks i starten.
      motors.setSpeeds(0, 0);
      break;
    case 1:                                         //Vanlig kjøremodus, følger linje
      if (!checkForObst()) {
        drive(desLinSpeed);
      } else {
        motors.setSpeeds(0, 0);
      }
      break;
    case 2:                                         // Hvis linja er borte, går vi i mode 2
      if (endOfRoad()) {
        mode = 1;
        adress = 0;
      }
      break;
    case 3:
      race();
      if (intersection() == 'b') {
        adress1 = false;
        desLinSpeed = 150;
        turnFactor = 1;
      }
      break;
    case 4:
      joystick();
      break;
    case 100:                                     // Kjører rett fram, så det blir lettere å holde kontroll på
      motors.setSpeeds(desLinSpeed, desLinSpeed);
      // adressetelling
      break;
    case 101:                                    //Adresse 1, kjør til høyre, for så å gå tilbake til mode 1
      if (intersection() == 'r' && adress1) {
        goRight();
        adress = 0;
        mode = 3;
        desLinSpeed = random(130, 155);         // motta beskjed om at løpet er over/ finne ut selv
      } else if (!adress1) {
        mode = 1;
        adress = 0;
      } else {
        drive(desLinSpeed);
      }
      break;
    case 102:                 // Adresse 2, kjør til venstre, for så å gå tilbake til mode 1
      if (intersection() == 'l' && adress2) {
        goLeft();
      } else if (!adress2) {
        mode = 1;
        adress = 0;
        turnFactor = -1;
      } else if (white()) {
        motors.setSpeeds(0, 0);
      } else {
        drive(desLinSpeed);
      }
      break;
    case 103:                                             //Adresse 3, pipelyd
      if (intersection() == 'l' && adress3) {
        goLeft();
      } else if (!adress3) {
        mode = 1;
        adress = 0;
        turnFactor = -1;
      } else if (white()) {
        motors.setSpeeds(0, 0);
      } else {
        drive(desLinSpeed);
      }
      break;
    case 104:                                             // Måtte kvitte oss med adresse 4 siden 3 søppelkasser ble
      mode = 1;                                           // tilbake til mode 1
      adress = 0;
      break;
    case 105:                                             //Adresse 5, Lade hvis trengs
      if (intersection() == 'r' && adress5) {
        goRight();
      } else if (white()) {
        motors.setSpeeds(90, 90);
        delay(220);
        motors.setSpeeds(0, 0);
        if (chargeOrService == "cb") {
          charge();
          chargeOrService = "";
        } else if (chargeOrService == "sb") {
          service();
          chargeOrService = "";
        }
        adress5 = false;
        mode = 1;
        adress = 0;
        turnFactor = 1;
      } else if (!adress5) {
        mode = 1;
        adress = 0;
      } else {
        drive(desLinSpeed);
      }
      break;
    default:                                              // Hvis ukjent mode, stopp bilen(må endres til slutt)
      motors.setSpeeds(0, 0);
      mode = 0;
      break;
  }
}
/*
  checkMail sjekker om bilen mottar noe fra MQTT, via ESP32 på toppen. I noen tilfeller(joystick)
  ønsker vi å motta både en beskjed og en verdi, dette gjøres ved at bokstav 0 og 1(null-indeksert) i Stringen som
  sendes inn mottas og lagres som en beskjed, deretter vil verdiene fra 2 og utover regnes som hastighet-beskjed.
  Deretter sjekkes beskjedene opp mot forhåndsprogrammerte instruksjoner, og gjør en oppgave ift. beskjeden. Dette kan
  f.eks. bestå av at en søppelkasse er full("a1"-"a3") eller at brukeren ønsker å styre bilen selv("m4"). Hvis vi er i
  modus for selv-kjøring vil også hastigheten mottas og lagres i variablen joySpeed
*/
void checkMail() {
  if (Serial1.available() > 0) {
    mailBox = Serial1.readStringUntil('|');
    String message;
    for (int i = 0; i < 2; i++) {
      message += (char)mailBox[i];
    }
    String speedTemp;
    for (int i = 2; i < 10; i++) {
      speedTemp += (char)mailBox[i];
    }
    if (speedTemp.toInt() != 0) {
      joySpeed = speedTemp.toInt();
    }

    if (message == "a1") {
      adress1 = true;
    } else if (message == "a2") {             //Søppel 2 full 'b2' = tom
      adress2 = true;
    } else if (message == "b2") {             //Søppel 2 full 'b2' = tom
      adress2 = false;
    } else if (message == "a3") {             //Søppel 3
      adress3 = true;
    } else if (message == "b3") {             //Søppel 3
      adress3 = false;
    } else if (message == "cb" || message == "sb") {
      adress5 = true;
      chargeOrService = message;
    } else if (message == "eb") {
      emergencyCharge();
    } else if (message == "m1") {
      mode = 1;
      adress = 0;
    } else if (message == "m4") {
      mode4 = !mode4;
      joyVal = "js";
    } else if (message == "jw" || message == "ja" || message == "jd" || message == "js" || message == "wa" || message == "wd") {
      joyVal = message;
    }
  }
}

/*
  Funksjonen som sender beskjeder via Serial til ESP32. Bruker if-setninger hver for seg fordi
  det kan være flere ting som trigges samtidig.
*/
void sendMail() {

  Serial1.print(batProsent);
  Serial1.print("|");
  delayMicroseconds(1);
  if (batHealth <= 70) {
    Serial1.print("Service anbefales|");
  } else {
    Serial1.print("Batterihelse: god|");
  }
  if (batProsent <= 20) {
    Serial1.print("Lad nå!|");
  } else {
    Serial1.print("Batteri OK :)|");
  }

}

/*
  Setup klargjør bilen for kjøring, bl.a. gjennom calib()-funksjonen, og kommunikasjon. Den setter også energinivå,
  og dette må gjøres etter at battery() er kalt på. Årsaken til dette er at bilen roterer når den kalibreres, og dette
  skal ikke tappe bilen for batteri. Løsningen blir å først kalle battery(), som endrer batCapacity, for så å hente
  prosenten bilen hadde ved forrige stans. Til slutt regnes batterikapasiteten ut ift. verdien på EEPROM
*/

void setup() {
  Serial1.begin(115200);

  while (calibrated == 0) {     //Kalibrerer bilen hvis det ikke allerede er gjort
    if (buttonA()) {
      calib();
      calibrated = 1;
    }
  }
  randomSeed(lineSensors.readLine(sensorValues));         //Finner en seed for generering av random som vil variere fra gang til gang
  while (!buttonA()) {
    mode = 0;
  }
  //For å nullstille batteri før kjøring
  battery();
  batProsent = EEPROM.read(1);
  batCapacity = batHealth * maxBatCapacity;
  batLevel = batProsent * batCapacity;
  delay(200);

}

/*
  loop er koden som bilen automatisk kjører kontinuerlig. Her kalles alle funksjoner på i en
  rekkefølge som sørger for at funksjonaliteten bevares. For eksempel må battery ligge utenfor if(batProsent < 0)
  slik at batteriet faktisk får mulighet til å fylles opp igjen.
*/
void loop() {
  battery();
  eeprom();
  checkMail();
  sendMail();
  if (batProsent > 0) {
    linePos = lineSensors.readLine(sensorValues);               //Finn zumoens posisjon
    checkMode();                                                //sjekk modus
    execute();                                                  //gjennomfør oppgave
  } else {
    motors.setSpeeds(0, 0);
  }
}�������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������������
