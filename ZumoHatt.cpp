/*
 * Denne koden skal være på ESP32 som sitter på toppen av Zumoen. 
 * Den har fått kallenavnet ZumoHatt, siden den sitter på Zumo som en hatt.
 * Koden tar inn informasjon fra alle temaene som er relevante for Zumoen, 
 * og printer det videre til Zumo via UART kommunikasjon. 
 * Den leser av beskjeder fra Zumo via UART kommunikasjon, og fordeler
 * beskjedene videre til relevant tema ut ifra hva koden tolker det som.
 * I essensen er det Zumoens egne kommunikasjonsmodul.
 */



//inkluderer biblioteker

#include <WiFi.h>         
#include <PubSubClient.h>
#include <Wire.h>

WiFiClient espClient;                    //angir navn vi bruker for wifi biblioteket
PubSubClient client(espClient);

const char* ssid = "Prosjektnett";
const char* password = "14159265";       //ssid og passord

const char* id = "zumohatt";             // Navngir denne ESP på MQTT serveren

const char* mqtt_server =  "10.0.0.6";   //mqtt serveren på RPi


// angir variabler
unsigned long time_now = 0;
float money;
float accountBalance = 1000;

void setupWifi() {                //wifi setup
  Serial.print("Kobler til ");    
  Serial.println(ssid);
  WiFi.disconnect();              //passer på clean connection
  delay(2000);                      
  WiFi.begin(ssid, password);     //kobler til

  while (WiFi.status() != WL_CONNECTED) {     //print ". . . " til tilkoblet
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi tilkoblet");
  Serial.println("IP Adresse: ");
  Serial.println(WiFi.localIP());    // printer IP adresse når tilkoblet
  //Ferdig med å koble til WiFi :)
}


void callback(char* topic, byte* message, unsigned int length) {      //angir callback funksjonen
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String espIN;

  for (int i = 0; i < length; i++) {               //Denne henter ut meldingen som kommer i callback
    Serial.print((char)message[i]);                //Callback hentet fra RandomNerdsTutorials, referert i rapporten
    espIN += (char)message[i];
  }
  Serial.print(espIN);                             //ZumoHatt sender koden videre til Zumo via UART kom.
}

void setupMQTT() {                                // setup av MQTT
  client.setServer(mqtt_server, 1883);     //setter serveren og port på RPi
  client.setCallback(callback);            //angir at callback funksjonen jeg har definert over skal brukes
  client.setClient(espClient);             
  client.connect(id);                      //setter forhåndsbestemt ID på ESP. Dette blir ID på MQTT serveren
  while (!client.connected()) {
    if ((time_now + 1000) < millis()) {
      time_now = millis();
      Serial.print(client.state());
      Serial.print(".");                   //print "..." frem til tilkoblet
    }
  }
  if (client.connected()) {                   //hvis den er tilkoblet, abboner til alle temaene vi trenger
    client.subscribe("zumo/command");    
    client.subscribe("gamepad/tx");
    client.subscribe("trash/1");
    client.subscribe("trash/2");
    client.subscribe("trash/3");
    Serial.println("mqtt funker");            //gir oss beskjed om at mqtt funker. 
                                              //Å få denne beskjeden føltes som julaften på et tidspunkt

  }
}


void espOUT() {                                    //Denne koden publiserer beskjeder fra Zumo til MQTT.
  String temp = Serial.readStringUntil('|');       //Leser av Stringen fra Zumo
  char Buf[50];                                    //Lager en buffer char array, som brukes til å lagre beskjeden
  if ((temp == "Service anbefales") || (temp == "Batterihelse: god")) {   //alle disse sjekker om beskjeden hvilken beskjed som kommer
    temp.toCharArray(Buf, 50);                                            //gjør om beskjeden til char array. Buf er nå array med beskjeden
    const char* a = "battery/health";                                     //angir tema
    const char* b = Buf;                                                  //legger til beskjed
    client.publish(a, b);                                                 //publiserer til MQTT temaet
  } else if ((temp == "Batteri OK :)") || (temp == "Lad nå!")) {
    temp.toCharArray(Buf, 50);
    const char* a = "battery/charge";
    const char* b = Buf;
    client.publish(a, b);
  } else if (temp.length() < 4) {                 // sjekker om lengden er 3 eller mindre, for da er det batteriprosent             
    temp.toCharArray(Buf, 50);                     
    const char* a = "battery/level";
    const char* b = Buf;
    client.publish(a, b);                         // publiserer batteriprosent
  } else if (temp.length() < 6) {                 // hvis lengden er 3 < x < 6, så er det solcellebatteri forbrukt
    temp.toCharArray(Buf, 50);
    const char* a = "solar/consumed";             // da sendes det til relevant tema
    const char* b = Buf;
    client.publish(a, b);                             
  }
}



void setup() {                    //Setup
  Serial.begin(115200);           //Start serial. Den må ha samme baudrate som Zumo sin Serial1.
  setupWifi();                    //Setup wifi
  setupMQTT();                    //setup MQTT
}

void loop() {
  if (!client.connected()) {  // passer  på at vi forblir tilkoblet mqtt
    client.connect(id);       //hvis ikke connected, connect med ID
  }
  client.loop();              //her inne sjekkes alle temaene vi har abboner på. Hvis det er
                              //en ny beskjed her, så kjøres callback funksjonen, og beskjedene
                              //videresendes til Zumo

  if (Serial.available()) {   //sjekker om det er beskjed fra Zumo
    espOUT();                 //publiserer beskjed til riktig tema hvis det er beskjed
  } 
}
