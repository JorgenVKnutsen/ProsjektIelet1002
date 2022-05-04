/* Denne koden er ment til å kjøre på en Arduino UNO. Den sjekker om det blir detektert bevegelse.
    Vi valgte denne løsningen fordi PIR sensoren vi bruker kjører på 5V logikk, noe Arduino UNO også gjør.
    Samtidig tenkte vi at det kunne være en spennende måte å lære hvordan vi kan kommunisere mellom disse.
   Denne koden styrer både lys og garasjeporten, og sender til MQTT når porten åpner og
   lysene skrus på, og samme når det skrus av.
*/

//inkluderer og navngir servo biblioteket
#include <Servo.h>

Servo servo;


//angir globale variabler
unsigned long time_now = 0;
const int motionSensor = 4;
const int LED = 11;
const int servoPin = 9;
int previousState = 0;
const int e = 2.71828;
int port = 0;



void motionDetected() {                //Dette er hva som skjer om bevegelse blir detektert
  port = !port;                        //toggler port slik at motsatt ting skjer neste gang
  if (port) {
    Serial.print("BilAnk");             //printer til serial monitoren ESP lytter til via UART
    for (int a = 0; a < 500; a++) {     //denne koden er nevnt i rapporten. Den åpner garasjeporten på en naturlig måte
      float b = 90 + 90 * pow(e, -0.01 * a);
      servo.write(b);
      delay(1);
    }
    servo.write(90);                    //passer på at servo er i riktig posisjon
    analogWrite(LED, 0);               
    for (int i = 0; i < 255; i++) {     //skrur på lyset gradvis 
      analogWrite(LED, i);
      delay(3);

    }
    analogWrite(LED, 255);

    delay(13000);                       //delay så PIR sensor kan bli lav

  } else if (!port) {                   //neste gang funksjonen aktiveres skrus lyset av og porten lukkes. 
    analogWrite(LED, 0);
    for (int i = 255; i > 0; i--) {
      analogWrite(LED, i);              //lys av
      delay(3);
    }
    analogWrite(LED, 0);                // port lukkes
    for (int a = 0; a < 500; a++) {
      float b = 180 - 90 * pow(e, -0.01 * a);
      servo.write(b);
      delay(1);
    }
    servo.write(180);
    Serial.print("BilFor");             //sender melding til ESP for å oppdatere status i Node-Red
    delay(13000);                       //delay så PIR sensor kan bli lav igjen
  }
}



void setup() {
  Serial.begin(9600);                 //Starter serial. Serial2 på ESP må matche denne.  
  servo.attach(servoPin);             //Angir servopin
  servo.write(180);                   //Setter garasjeporten til lukket.
  analogWrite(LED, 0);                //setter lys til 0
  pinMode(LED, OUTPUT);               //angir pinmode         
  pinMode(motionSensor, INPUT);       
  delay(10000);                       //lar PIR sensoner kalibrere

}


void loop() {
  if ((millis() - time_now) > 69) {    //sjekker hvert 69. millisekund om det har skjedd noe
    time_now = millis();
    if (digitalRead(motionSensor)) {  //hvis det leses av en høy verdi på motionSensor, så startes motionDetected.
      motionDetected();
    }
  }
}
