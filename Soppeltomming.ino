/*Programmet har tre funksjoner som måler avstanden på tre forskjellige
   avstandsmålere. Deretter skjekker den om avstanden er større eller 
   mindre en en satt grenseverdi. Hvis avstander er mindre betyr det at
   søppelnivået er høyt. Hvis den registrerer avstand under grenseverdien
   10 ganger på rad på en av sensorene, sendes det beskjed (MQTT) om at det
   må tømmes søppel der. Når beskjeden er sendt slutter den å måle avstand
   på den sensoren til den for beskjed (MQTT) om at søppelet er tømt.
*/

//inkluderer og navngir biblioteker
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "Prosjektnett";      //ssid og pw
const char* password = "14159265";

const char* id = "trash";               //ID til mqtt

const char* mqtt_server =  "10.0.0.6";     //mqtt serveren på RPi


//tid
unsigned long time_now = 0;
//angir pins
const byte echoPin1 = 18;
const byte trigPin1 = 19;
const byte echoPin2 = 25;
const byte trigPin2 = 26;
const byte echoPin3 = 32;
const byte trigPin3 = 33;

//boolske start-verdier angis 
bool previous1 = 0;
bool previous2 = 0;
bool previous3 = 0;

//tellere settes til 0
byte counter1 = 0;
byte counter2 = 0;
byte counter3 = 0;

//boolske "påvei" variabler
bool onWay1 = 0;
bool onWay2 = 0;
bool onWay3 = 0;


//avstander til ultrasoniske sensorer
float distance1;
float distance2;
float distance3;
float threshold = 10;

//tid
unsigned long duration1;
unsigned long duration2;
unsigned long duration3;

void setupWifi() {                    //samme som før
  //Kobler til WiFi
  Serial.print("Kobler til ");
  Serial.println(ssid);
  WiFi.disconnect();
  delay(2000);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {    
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi tilkoblet");
  Serial.println("IP Adresse: ");
  Serial.println(WiFi.localIP());
  //Ferdig med å koble til WiFi :)
}


void setupMQTT() {                        
  client.setServer(mqtt_server, 1883);       //samme som før
  client.setClient(espClient);
  client.connect(id);
  while (!client.connected()) {
    if ((time_now + 1000) < millis()) {
      time_now = millis();
      Serial.print(client.state());
      Serial.print(".");
    }
  }
  if (client.connected()) {
    Serial.println("mqtt funker");
  }
}



float getDistance1() {
  digitalWrite(trigPin1, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);

  duration1 = pulseIn(echoPin1, HIGH);
  distance1 = duration1 * 0.0343 / 2;

  return distance1;
}


float getDistance2() {
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);

  duration2 = pulseIn(echoPin2, HIGH);
  distance2 = duration2 * 0.0343 / 2;

  return distance2;
}

float getDistance3() {
  digitalWrite(trigPin3, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin3, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin3, LOW);

  duration3 = pulseIn(echoPin3, HIGH);
  distance3 = duration3 * 0.0343 / 2;

  return distance3;
}


void binStatus() {
  //Sensor 1
  if (distance1 < threshold) {    //hvis avstanden er mindre enn toleransen så teller den opp
    counter1 += 1;
  } else if (distance1 > threshold) {
    counter1 = 0;
  }
  if ((counter1 >= 10) && (!previous1)) {   //hvis avstanden er over 10 for første gang,
    const char* a = "trash/1";              //publiser status til MQTT
    const char* b = "a2|";
    client.publish(a, b);
    previous1 = !previous1;                 
  } else if ((counter1 == 0) && (previous1)){  //hvis den hopper ned igjen, publiser oppdatert status til MQTT
    const char* a = "trash/1";
    const char* b = "b2|";
    client.publish(a, b);
    previous1 = !previous1;

  }

  //Sensor 2                                    //det samme repeteres for sensor 2 og 3
if (distance2 < threshold) {
    counter2 += 1;
  } else if (distance2 > threshold) {
    counter2 = 0;
  }
  if ((counter2 >= 10) && (!previous2)) {
    const char* a = "trash/2";
    const char* b = "a3|";
    client.publish(a, b);
    previous2 = !previous2;
  } else if ((counter2 == 0) && (previous2)){
    const char* a = "trash/2";
    const char* b = "b3|";
    client.publish(a, b);
    previous2 = !previous2;
  }
/*
 * 
 * Søpplebøtte nr 4 så vi oss nødt til å fjerne i praksis da ESP ble ekstremt ustabil mtp. MQTT. 
 * Vi tror det er på grunn av at det dro for mye strøm, og derfor begynte den å
 * glitche litt. På de andre modulene som bruker 5V logikk har vi derfor brukt 
 * Arduinoer til å styre logikken, og en ESP ved siden av til å kommunisere med resten! 
 * Den løsningen har gitt mye mer stabile resulteter. 
 */
  //Sensor 3
  if (distance3 < threshold) {
    counter3 += 1;
  } else if (distance3 > threshold) {
    counter3 = 0;
  }
  if ((counter3 >= 10) && (!previous3)) {
    const char* a = "trash/3";
    const char* b = "a4|";
    client.publish(a, b);
    previous3 = !previous3;
  } else if ((counter3 == 0) && (previous3)){
    const char* a = "trash/3";
    const char* b = "b4|";
    client.publish(a, b);
    previous3 = !previous3;
  }
  time_now = millis(); 
  
}




void setup() {                        
  setupWifi();
  setupMQTT();
  pinMode(trigPin1, OUTPUT);      //angir bruk av pins
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  pinMode(trigPin3, OUTPUT);
  pinMode(echoPin3, INPUT);
}

void loop() {
  if(!client.connected()){    //if mqtt dead {mqtt = !dead}
    client.connect(id);
  }
  getDistance1();             //sjekker avstandene
  getDistance2();
  getDistance3();

  if ((time_now + 1000) < millis()){      //Oppdater teller og evt status i sek intervaller
    binStatus(); 
  }
}
