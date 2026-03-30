#include <Wire.h>
#include <BH1750.h>

BH1750 lightSensor;

const uint8_t SENSOR_PIN = 2;
const uint8_t SWITCH_PIN = 3;
const uint8_t LEDB_PIN = 7;
const uint8_t LEDR_PIN = 8;

const unsigned long DEBOUNCE_MS = 200;
volatile bool isrActive = false;
volatile unsigned long lastTrigger = 0;
volatile byte ledState = LOW;

const float LIGHT_THRESHOLD = 20;

void triggerLights(bool power){
  ledState = power; 
  digitalWrite(LEDR_PIN, power);
  digitalWrite(LEDB_PIN, power);
  
  log(power ? "== Lights ON ==" : "== Lights OFF ==");
}

// Interrupt handler for switch motion. Toggles lights on/off if interrupt registered.
// Debounce logic ignores a retrigger with DEBOUNCE_MS to prevent any potential flickering of the leds
void ISRtriggerLights(){
  unsigned long now = millis();
  if (now - lastTrigger < DEBOUNCE_MS) return;
  lastTrigger = now;
  
  if (isrActive) {
    isrActive = false;
    triggerLights(LOW); 
  } else {
    isrActive = true;
    triggerLights(HIGH);
  }
}

void log(const char* message){
  Serial.println(message);
  Serial.println();
}
void log(float lux, bool motionDetected){
  Serial.print("Lux: ");    
  Serial.println(lux);
  Serial.print("Motion: "); 
  Serial.println(motionDetected);
  Serial.print("LED: ");    
  Serial.println(ledState);
  Serial.println();
}

void setup() {
  Serial.begin(9600);
  while(!Serial);
  
  Wire.begin();
  lightSensor.begin();

  pinMode(SWITCH_PIN, INPUT);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(LEDR_PIN, OUTPUT);
  pinMode(LEDB_PIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), ISRtriggerLights, CHANGE);

  // warm up delay to give the PIR sensor time to adjust to the room conditions
  Serial.println("Sensor warming up...");
  delay(60000);
  Serial.println("Warmup complete");
}

void loop() {
  // doesn't let sensors override if the switch has been turrned on 
  if (isrActive) {
    return;
  }

  float lux = lightSensor.readLightLevel(); 
  uint8_t motion = digitalRead(SENSOR_PIN);
  bool lowLight = lux < LIGHT_THRESHOLD;

  log(lux, motion);

  // if no motion, irrespective of light levels. Lights are turned off.
  // otherwise if low level light and motion detected, lights are on. 
  if (!motion && ledState == HIGH) triggerLights(LOW);
  else if (ledState == LOW && (lowLight && motion)) triggerLights(HIGH);
}
