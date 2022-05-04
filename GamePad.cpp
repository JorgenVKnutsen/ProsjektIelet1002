/* --- GAMEPAD --- 
  Her er koden for gamepaden for å manuelt kontrollere Zumoen.
  Denne koden vil motta input fra fire knapper og et potensiometer,
  som vil deretter bli prosessert og lagt inn i en streng.
  Strengen blir deretter sendt videre som en charArray til en mqtt server,
  hvor den videre vil bli sendt til Zumoen, som lar oss ta kontroll over 
  bilens styring og fart. 
  (Void Setup og -loop finner du nederst i koden)
  _____________________________________________________________________
*/

// BIBLIOTEKER
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

// ANNET
WiFiClient espClient;
PubSubClient client(espClient);

// TILKOBLING
const char* ssid = "Prosjektnett";
const char* password = "14159265";    //ssid and passord

const char* id = "gamepad";  

const char* mqtt_server =  "10.0.0.6";       //MQTT server på Rpi

// TID
unsigned long lastTime = 0;

byte activate = 0;

// D-PAD
const int X = 32;
const int Y = 26;
const int C = 33;
const int Z = 34;

// FARTSSTYRING
const int potPin = 35;
int potVal;

// KNAPPESTATUS
bool xState = 0;
bool yState = 0;
bool cState = 0;
bool zState = 0;

// CHARS
char msg;
char honk;

// STRINGS
String cmd;
String cmd1;
String cmd2;
String cmd3;
String but;

// FUNKSJONER

void setupWifi() {
  //Kobler til WiFi
  Serial.print("Kobler til ");
  Serial.println(ssid);
  WiFi.disconnect();
  delay(2000);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {     // kode hentet fra
    delay(500);
    Serial.print(".");
  }
}

void buttons() {
  // Gir knappestatus til D-Pad knapper
if(xState && yState){
  but = "wa"; // Hvis x og y trykkes, sendes wa + fart. Får bilen til å svinge smoothere
} else if (yState && zState) {
  but = "wd";

  
} else if (xState) {
    but = "ja";    // Venstre knapp tildelt "ja"
  }

  else if (yState) {
    but = "jw";    // Øverste knapp tildelt "jw"
  }

  else if (zState) {
    but = "jd";    // Høyre knapp tildelt "jd"
  }

  else {
    but = 'O';    // Når ingen knapp trykkes, sett til "O"
  }
}

void buttonStates() {
  // Leser knappestatus og setter verdi
  if (digitalRead(X)) {
    xState = 1;                   // Hvis en knapp trykkes, vil dens verdi være 1
  }

  else {
    xState = 0;                   // Hvis ikke, vil verdien være lik 0 (samme for alle knappene)
  }

  if (digitalRead(Y)) {
    yState = 1;
  }

  else {
    yState = 0;
  }

  if (digitalRead(C)) {
    cState = 1;
  }

  else {
    cState = 0;
  }

  if (digitalRead(Z)) {
    zState = 1;
  }

  else {
    zState = 0;
  }
}

void honkyPonky() {
  // Midterste knapp er satt til hornet
  if (cState == 1) {
    honk = 'H';       // Samme måte som andre knappene
  }
  else {
    honk = 'h';       // Samme måte som andre knappene
  }
}

void strings() {
  // Strenger lages her
  if (honk == 'H') {
    potVal = potVal + 1000;     // Honk er aktviv når potVal er over 255 (dette skjer i koden hos Zumoen)
  }
  cmd1 = String(potVal);        // Lager en invividuell streng for potVal
  cmd2 = but;                   // Lager en individuell streng for but
  cmd = cmd2 + cmd1 + "|";      // Kombinerer strengene til én
}

void mapping() {
  // Mapper avlesningen fra potmeteret til intervallet -100 < potVal < 250
  potVal = map(analogRead(potPin), 0, 4095, 250, -100);
}

void misc() {
  // Ting som skjer hvert 10ms
  if (millis() - lastTime >= 10) {      // Alt inne i if-løkken skjer hvert 10ms
    char cmdTemp[10];                   // Lager en char for cmd-en
    cmd.toCharArray(cmdTemp, 10);       // Legger cmd-strengen inn i en charArray
    const char* a = "gamepad/tx";
    const char* b = cmdTemp;            // Nå kan den sende dataen til MQTT
    client.publish(a, b);               // Sender data...
    lastTime = millis();                // Oppdaterer klokka
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  if (messageTemp == "m4|") {
    Serial.println(messageTemp);
    activate = !activate;
  }

}
void setup() {
  Serial.begin(115200);           // Starter serial
  pinMode(potPin, INPUT);         // Setter potmeter til INPUT
  pinMode(X, INPUT);              // Setter knapp til INPUT
  pinMode(Y, INPUT);              // Setter knapp til INPUT
  pinMode(C, INPUT);              // Setter knapp til INPUT
  pinMode(Z, INPUT);              // Setter knapp til INPUT
  lastTime = millis();

  //WiFi MQTT setup
  setupWifi();                            //kobler på WiFi
  client.setServer(mqtt_server, 1883);    //setter serveren
  client.setClient(espClient);
  client.connect(id);
  client.setCallback(callback); 
  while (!client.connected()) {
    if ((lastTime + 1000) < millis()) {
      lastTime = millis();
      Serial.print(client.state());
      Serial.print(".");
    }
  }
  if (client.connected()) {
    client.subscribe("zumo/command");
    Serial.println("mqtt funker");
  }
}



void loop() {
  if (!client.connected()) {  // Forsikrer om at vi forblir tilkoblet til MQTT
    client.connect(id);
  }
  client.loop();

  // Kaller funksjoner
  if(activate){
    mapping();
    misc();
    buttonStates();
    buttons();
    strings();
    honkyPonky();
  }
}
