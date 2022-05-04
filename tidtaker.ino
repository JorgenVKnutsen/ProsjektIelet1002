/*
 * Denne koden sitter på en ESP32 som er plassert ved racerbanen. Denne sjekker og publiserer både
 * løpende og beste tid + evt penger tjent ved å sette nye rekorder. Den skal kobles til en PIR sensor.
 */

//inkluderer og navngir biblioteker
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>



WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "Prosjektnett";
const char* password = "14159265";           //ssid og passord

const char* id = "racetimer";                // ID til MQTT

const char* mqtt_server =  "10.0.0.6";       //mqtt serveren på RPi


//angir variabler og sensorpin
unsigned long time_now = 0;
unsigned long currentTime = 0;
unsigned long timeTaker = 0;
float roundTime = 0;
float bestTime = 30;
int previousState = 0;
const int timeSensor = 5;
float timePub;




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


void setupMQTT() {                          //samme som før
  client.setServer(mqtt_server, 1883);     
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

void runTimeMQTT(float t) {                 //denne gjør koden mer kompakt senere, da den tar inn 
  char timeToPublish[6];                    // flyttallet den skal publisere, og publiserer det.
  dtostrf(t, 1, 2, timeToPublish);          //input er float, som konverteres til char array vha dtostrf(),  
  const char* a = "time/current";           //og publiserer det til relevant MQTT tema
  const char* b = timeToPublish;
  client.publish(a, b);
}

void bestTimeMQTT(float timeNew, float moneySend) {   //Denne gjør akkurat samme som funksjonen over
  char bestTimePub[10];                               // men den tar inn ny beste tid, og penger tjent
  dtostrf(timeNew, 1, 2, bestTimePub);                //konverterer vha dtosrf() og publiserer til
  const char* a = "time/best";                        //riktig tema. 
  const char* b = bestTimePub;
  client.publish(a, b);

  char moneyMadeRace[10];                             //buffer char array
  dtostrf(moneySend, 1, 2, moneyMadeRace);
  const char* c = "money/made";                       //sendes til forskjellige temeaer for å holde styr på det
  const char* d = moneyMadeRace; 
  client.publish(c, d); 
}


void timeTrial() {
  time_now = millis();
  if ((digitalRead(timeSensor)) && (!previousState)) { // trigger er hvis previous state er 0, og nåværende state er 1
    time_now = millis();                               // to forskjellige tider fordi vi trenger flere intervaller inni intervallene
    previousState = digitalRead(timeSensor);           // endrer previous state    
    currentTime = millis();                            //CurrentTime brukes som tellende variabel i tidtakningen
    while ((time_now + 10000) > millis()) {            //sensoren gir 1 i 10 sek etter aktivert. Dette gjør at den ikke registrerer-
      if ((currentTime + 25) < millis()) {
        timePub = ((millis() - time_now) / 1000.00);
        runTimeMQTT(timePub);                          //hvert 25ms sendes publiseres løpende tid. Input er som nevnt tidligere flyttallet tid 
        currentTime = millis();                        // nullstiller klokken
      }     
    }
    while ((!digitalRead(timeSensor)) && (previousState)) { //koden hopper hit og gjør det samme etter 10 sek. Den holde seg her til neste gang
      timePub = ((millis() - time_now) / 1000.00);          //PIR sensoren blir aktivert
      if ((currentTime + 25) < millis()) {          
        runTimeMQTT(timePub);                               //publiserer løpende tid hvert 25 ms
        currentTime = millis();                                   
      }
    }                                             //frem til neste gang noe går forran sensoren holder den seg her
    time_now = millis();
    if (bestTime > timePub) {                    //sjekker om der er ny rekord
      float difference = bestTime - timePub;     //hvis ny rekord, tar den forskjellen
      float cashPrice = difference * 300;        //bestemmer cashprize
      bestTime = timePub;             
      bestTimeMQTT(bestTime, cashPrice);         //og publiserer disse via forhåndsbestemt funksjon
    }
    previousState = 0;                          //resetter 
    while ((time_now + 12000) > millis()) {}    //lar PIR bli lav igjen
  }
}

void setup() {
  time_now = millis();
  Serial.begin(115200);
  setupWifi();                            
  setupMQTT();
  pinMode(timeSensor, INPUT);
  while (time_now + 8000 > millis()) {}
  const char* a = "time/best";            //nullstiller beste tid
  const char* b = "0";
  client.publish(a, b);         
  if ((time_now + 10000) < millis()) {    //started loopen etter 10 sek, for å la PIR sensor kalibere
  }
}

void loop() {

  if (!client.connected()) {  // passer  på at vi forblir tilkoblet mqtt
    client.connect(id);
  }
            
  if (time_now + 50 < millis()) {  //Sjekker om den skal begynne hvert 50ms, og hvis aktivert begynner det.
    timeTrial();                   // prøvde en annen type metode her hvor den alltid går inn i funksjonen, men kommer ut igjen
  }                                // hvis ikke noe skjer. Hvis den aktiveres faller den inn i funksjonen og blir der frem til
}                                  // løpet er avsluttet.  
