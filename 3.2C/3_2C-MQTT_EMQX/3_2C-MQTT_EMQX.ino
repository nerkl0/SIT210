#define LED_RED 2
#define LED_GREEN 3
#define TRIG_PIN 6
#define ECHO_PIN 7

#include "LocalWifi.h"
#include "Broker.h"
#include "arduino_secrets.h"

typedef struct {
  const char* wave;
  const char* pat;
} Topic;

const Topic topics = {
  .wave = "ES/s225617573/wave",
  .pat = "ES/s225617573/pat"
};

bool light_on = false; 
float duration_us, distance_cm;

void detectedTouch(String message){
  Serial.print("Touch detected: "); 
  Serial.println(message);
  triggerLights(false);
  delay(1000);
}

void detectedWave(String message){
  Serial.print("Wave detected: ");
  Serial.println(message);
  triggerLights(true);
}

float readSensor(){
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration_us = pulseIn(ECHO_PIN, HIGH);    // measures pulse duration of object distance
  distance_cm = 0.017 * duration_us;        // measures distance between sensor & obstacle
  return distance_cm;
}

void triggerLights(bool power){
  digitalWrite(LED_RED, power);
  digitalWrite(LED_GREEN, power);
  light_on = power;
}

void printToTerminal(int dist){
  Serial.println("======================");
  Serial.print("distance: ");
  Serial.print(dist);
  Serial.println(" cm");
}

void setup() {
  Serial.begin(9600);
  while(!Serial);

  Serial.println("Setup");
  connectLocalWifi();

  connectToBroker();
  subscribeToTopic(topics.wave, detectedWave);
  subscribeToTopic(topics.pat, detectedTouch);
  mqttClient.setCallback(parseMessage);       // sets the callback function to parse received messages

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  Serial.println("System active");
}

void loop() {
  mqttClient.loop();

  readSensor();

  // handles touch functionality. Slight delay added after the if the first statment is true
  // to ensure it isn't picking up motion within the room. If after 600ms the distance is still
  // below 5cm, will publish a light off request to the broker
  if(distance_cm <= 5 && light_on) {
    printToTerminal(distance_cm);
    Serial.println("Pause for pat confirmation");
    delay(600);
    if (readSensor() <= 5) {
      printToTerminal(distance_cm);
      publishToBroker(topics.pat, "Lights OFF");
    }
  }
  else if(distance_cm < 20 && !light_on) {
    printToTerminal(distance_cm);
    publishToBroker(topics.wave, "Lights ON");
  }

  delay(500);
}