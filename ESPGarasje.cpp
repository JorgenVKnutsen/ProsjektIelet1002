/*
 * Denne koden skal kjøre på ESP koblet til Arduino på garasjen og ladestasjonen. 
 * Den tar imot kommandoer fra Arduino, og publiserer til MQTT for at det
 * skal displayes i Node-Red
 * 
 */

//inkluderer biblioteker
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

//angir porter for kommunikasjon
#define RXp2 16
#define TXp2 17


WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "Prosjektnett";
const char* password = "14159265";           //ssid og passord

const char* id = "Garasje";                  //navnt til MQTT

const char* mqtt_server =  "10.0.0.6";       //mqtt serveren på RPi



//tid
unsigned long time_now = 0;



void setupWifi() {                          //samme som før
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

void setupMQTT(){                                 //samme som før, uten callback fordi vi ikke tar inn noe
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


void publishMQTT() {                            //publiserer til MQTT basert på hvilke beskjeder som kommer inn. 
  String message = Serial2.readString();
  Serial.println("Message received");
  Serial.println(message);
  if (message == "BilAnk") {                      //sjekker beskjed
    const char* a = "zumo/garasje";               //angir relevant
    const char* b = "Bil har ankommet garasje.";  //printer beskjed som skal displayes i Node-Red                     
    client.publish(a, b);                         //publiserer
  } else if (message == "BilFor") {
    const char* a = "zumo/garasje";
    const char* b = "Bil har forlatt garasje.";
    Serial.println("lys av");  
    client.publish(a, b);  

  }
}



void setup() {
  time_now = millis();
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2);   //denne må matche Serial.begin på arduino

  setupWifi();   //kobler på WiFi
  setupMQTT();  
}

void loop() {
  
  if (!client.connected()) {  // passer  på at vi forblir tilkoblet mqtt
    client.connect(id);
  }
  client.loop();  
  if ((time_now + 50) < millis()) { //sjekker hvert 50ms
    time_now = millis();
    if (Serial2.available()) {  //hvis den får beskjed, så publiserer den til MQTT
      publishMQTT();           
    }
  }
}
