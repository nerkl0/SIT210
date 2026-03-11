#include <DHT.h>
#include "network_utils.h"
#include "utils.h"

#define DHT11_PIN 2
DHT dht11(DHT11_PIN, DHT11);

// Pin and Status variables
const int LED_PIN = 4;
const int LIGHT_PIN = A0;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(LED_PIN, OUTPUT);

  bool wifiSuccess = connectToWifi();     // connect to Wifi
  if(wifiSuccess) toggleLED(LED_PIN);     // Set LED on
  dht11.begin();                          // connect to DHT
  connectToThingSpeak();                  // connect to ThingSpeak
}

void loop() {
  float tempC = dht11.readTemperature();
  int lightSensor = analogRead(LIGHT_PIN);
  sendDataThingSpeak(tempC, lightSensor);
  delay(30000);
}