void toggleLED(void);

#define TS_ENABLE_SSL     // enable sending data over HTTPS 

#include <DHT.h>
#include <WiFiNINA.h>
#include <ThingSpeak.h>
#include "arduino_secrets.h"

// Pin and Status variables
#define DHT11_PIN 2
DHT dht11(DHT11_PIN, DHT11);
const int LED_PIN = 4;
const int LIGHT_PIN = A0;
int LED_Status = LOW;

// Wifi credentials
const char SSID[] = SECRET_SSID;
const char PSWD[] = SECRET_PWD;

// ThingSpeak credentials
unsigned long tsChannelNum = SECRET_CH_NUM;
const char API_KEY[] = SECRET_API_WRITE;

WiFiSSLClient client;        // initialise secure wifi client

void setup() {
  pinMode(LED_PIN, OUTPUT);

  while (WiFi.begin(SSID, PSWD) != WL_CONNECTED) {
    delay(1000);
  }
  toggleLED();                            // Set LED on once wifi connects
  
  dht11.begin();                          // connect to DHT
  ThingSpeak.begin(client);               // connect to ThingSpeak
}

void loop() {
  float tempC = dht11.readTemperature();
  int lightSensor = analogRead(LIGHT_PIN);

  // set ThingSpeak fields to write readings to first before writing to channel
  ThingSpeak.setField(1, tempC);
  ThingSpeak.setField(2, lightSensor);
  ThingSpeak.writeFields(tsChannelNum, API_KEY);

  delay(30000); // wait 30 seconds before processing further readings
}


// helper function to toggle LED when wifi connects
void toggleLED(){
  LED_Status = !LED_Status;
  digitalWrite(LED_PIN, LED_Status);
}
