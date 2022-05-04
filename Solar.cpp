/*Denne koden simulerer et batteri, som lades via input fra et solcellepanel.
   Strømmen i batteriet brukes til å lade en "Zumo bil", og den selges til et
   tenkt strømnett for å tjene penger. Prisen på strømmen får vi fra en
   funksjon som skal etterligne en varierende strømpris.  
  På denne koden drives også økonomien. Det er lagret her fordi det var mest praktisk.
   
*/

//inkluderer biblioteker, definerer og angir navn
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <EEPROM.h>


#define EEPROM_SIZE 1

WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "Prosjektnett";
const char* password = "14159265";           //ssid og passord

const char* id = "solar";                    //navngir som Solar i MQTT

const char* mqtt_server =  "10.0.0.6";       //mqtt serveren på RPi


const int sensorPin = 32;                    //Sensor pin = 32. 25 fungerer ikke med wifi

//angir globale variabler
int moneyMade;
int lightLevel;
float solarBatLevel = 0;
float i;


//start balanse på konto, KUN første gang kode kjøres 
int accountBalance = 4000;

//globale tidvariabler. Bruker forskjellige fordi vi teller forskjellige steder
unsigned long time_now = 0;
unsigned long startTime;
unsigned long sellTime = 0;
unsigned long eepromTime = 0;
unsigned long chargeTime = 0;



void callback(char* topic, byte* message, unsigned int length) {     //angir hva callback skal gjøre her
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String moneyIn;

  for (int i = 0; i < length; i++) {                      //henter ut beskjeden fra MQTT temaene vi har
    moneyIn += (char)message[i];                          // abonnert til            
  }
  if (moneyIn == "sb|") {                                 //hvis beskjeden er "sb|" har en service blitt bestillt
    accountBalance -= 4000;                               // så da må vi trekke fra 4000kr
  } else if (moneyIn.length() < 4) {                      // hvis lengden er under 4 så er det inntekt, så vi legger det til
    accountBalance += moneyIn.toFloat();                  // vi tjener aldri mer enn 999kr fra en transasksjon, så det fungerer sånn.
  } else {                          
    solarBatLevel -= moneyIn.toFloat();                   //Ellers er det energi hentet fra solcellebatteriet, så da trekker vi ifra
  }                                                       //ladning fra batteriet
}

void setupWifi() {                                        //kommentert tidligere, er nøyaktig den samme koden
  //Kobler til WiFi                                     
  Serial.print("Kobler til ");
  Serial.println(ssid);
  WiFi.disconnect();
  delay(2000);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {     // samme som før
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi tilkoblet");
  Serial.println("IP Adresse: ");
  Serial.println(WiFi.localIP());
  //Ferdig med å koble til WiFi :)
}

void setupMQTT() {                                //Nøyaktig samme kode som kommentert tidligere                      
  client.setServer(mqtt_server, 1883);     
  client.setClient(espClient);
  client.setCallback(callback);
  client.connect(id);
  while (!client.connected()) {
    if ((time_now + 1000) < millis()) {
      time_now = millis();
      Serial.print(client.state());
      Serial.print(".");
    }
  }
  if (client.connected()) {                 //abonnerer på relevante temaer
    client.subscribe("solar/consumed");
    client.subscribe("money/made");
    client.subscribe("money/cost");

    Serial.println("mqtt funker");          //sier ifra hvis ok
  }
}


void getLightLevel() {                        
  lightLevel = analogRead(sensorPin);             //sjekker lysnivået
  lightLevel = map(lightLevel, 0, 4095, 0, 100);  //Maper min- og maxverdien fra solcellepanelet fra 0-100 for enkelhets skyld
}

void charge() {                          //hvis lysnivået er høyt nok, så lader den hvis den ikke er fullladet.
  if (solarBatLevel < 20000) {           //Skjekker om batteriet er fullt
    solarBatLevel += (lightLevel);       //Øker batteriprosenten ut ifra lysmengden
    if (solarBatLevel > 20000) {         //Passer på at batteriprosenten ikke blir over 100
      solarBatLevel = 20000;             
    }
  }
}

void publishBatAndAccount() {                  
  char batteryPublish[10];                        //angir buffer char array
  dtostrf(solarBatLevel, 1, 2, batteryPublish);   //omgjør fra float til char array
  const char* a = "solar/Bat";                    //angir tema
  const char* b = batteryPublish;                 //angir beskjed, her er det den konverterte floaten
  client.publish(a, b);                           //publiserer

  char accountPublish[10];                          // -||-
  dtostrf(accountBalance, 1, 2, accountPublish);
  const char* c = "money";
  const char* d = accountPublish;
  client.publish(c, d);
}

void sell() {                                       
  float t = (millis() - startTime) / 1000.00;                            //sjekker sekunder siden start
  i = 30 + 15 * cos(0.018 * t) + 6 * cos(0.05 * t) + 2 * cos(0.60 * t);  //regner ut pris basert på tid
  float sellAmount = solarBatLevel - 10000;                              //bestemmer hvor mye vi kan selge for å ha 10000 igjen.
  accountBalance  += (sellAmount * i) / 350;                             //ganger pris med mengde (deler på 350 for å gi normale tall. fikk 400k en gang) 
  solarBatLevel = 10000;                                                 //angir nytt nivå på batteriet
}



void setup() {                        
  Serial.begin(115200);                     
  EEPROM.begin(EEPROM_SIZE);              //starter EEPROM 
  startTime = millis();                   //setter tid til bruk for å finne ut sekunder siden start
  setupWifi();                            
  setupMQTT();
  time_now = millis();
  pinMode(sensorPin, INPUT);             
  accountBalance = EEPROM.read(0);        //EEPROM for å finne kontobalanse. Dette gjør at kontobalansen vår beholdes uavhengig av
  accountBalance *= 1000;                 //om strømmen tapes. Vi ganger opp igjen, fordi vi kan bare lagre bytes (0-255), og vi skalerer
}                                         //ned før vi lagrer.

void loop() {

  if (!client.connected()) {              //if broken, fix           
    client.connect(id);
  }
  client.loop();                          //kjører callbackfunksjonen når det finnes beskjeder i abbonerte temaer
       


  getLightLevel();                                                //sjekker lysnivået på solcellepanelet
  if ((lightLevel > 50) && ((chargeTime + 50) < millis()))  {     // hvis høyt nok, og tidsintervall ok så lader den
    charge();
    chargeTime = millis();
  }

  if ((time_now + 500) < millis()) {        //publiserer balanse og batterinivå hvert 0.5 sekund
    publishBatAndAccount();
    time_now = millis();
  }
  if ((sellTime + 15000) < millis()) {
    float t = (millis() - startTime) / 1000.00;                           //sjekker tid og pris
    i = 30 + 15 * cos(0.018 * t) + 6 * cos(0.05 * t) + 2 * cos(0.60 * t);
    if ((solarBatLevel > 16000) && (i > 40)) {                            //Selger strøm hvis det er mye strøm i batteriet og strømprisen er høy
      sell();
      sellTime = millis();
    }
  }

  if ((eepromTime + 60000) < millis()) {           //backer opp accountBalance i EEPROM hvert 60 sekund
    byte eepromAccount  = accountBalance / 1000;
    EEPROM.write(0, eepromAccount);
    EEPROM.commit();
  }
}
