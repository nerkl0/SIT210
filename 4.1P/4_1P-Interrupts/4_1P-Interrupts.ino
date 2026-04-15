#include <Wire.h>
#include <BH1750.h>

BH1750 lightSensor;

const uint8_t SENSOR_PIN = 2;
const uint8_t SWITCH_PIN = 3;
const uint8_t LEDB_PIN = 7;
const uint8_t LEDR_PIN = 8;

const unsigned int DEBOUNCE_MS = 300;
const unsigned int MOTION_TIMEOUT = 10000; 
volatile bool switchActive = false;
volatile unsigned long lastMotionTrigger = 0; 
volatile unsigned long lastSwitchTrigger = 0;
volatile byte ledState = LOW;

const float LIGHT_THRESHOLD = 20;

/* Variables for logging */
unsigned long lastLog = 0;
const unsigned int LOG_INTERVAL = 2000;
float lastLux = -1;
bool lastMotionState = false;
bool lastLedState = LOW;

void triggerLights(bool power){
  ledState = power; 
  digitalWrite(LEDR_PIN, power);
  digitalWrite(LEDB_PIN, power);
  
  log(power ? "== Lights ON ==" : "== Lights OFF ==");
}

// Interrupt handler for switch. Interupt set to CHANGE. Will update switchActive variable to inverse boolean when triggereds.
// Debounce logic ignores a retrigger with DEBOUNCE_MS to prevent any potential flickering of the leds
void ISRtriggerLights(){
  unsigned long now = millis();
  if (now - lastSwitchTrigger < DEBOUNCE_MS) return;
  lastSwitchTrigger = now;

  switchActive = !switchActive;
  log(switchActive ? "== Switch ON ==" : "== Switch OFF ==");
}

// ISR set to RISING. When motion detected, sets lastMotionTrigger variable to current board uptime.
// Compare millis in loop to have motion sensor trigger on a countdown for automatic shut off 
void ISRmotionDetected(){
  lastMotionTrigger = millis();  
}

void log(const char* message){
  Serial.println(message);
  Serial.println();
}
void logLux(float lux){    
  Serial.print("Lux: ");    
  Serial.println(lux);
  Serial.print("LED: ");    
  Serial.println(ledState);
  Serial.println();
}
void logMotion(bool motionSensor){
  Serial.print("Motion Detected: ");
  Serial.println(motionSensor);
  if(motionSensor) {
    Serial.print("Time Remaining: ");
    Serial.println(MOTION_TIMEOUT - (millis() - lastMotionTrigger));
  }
  Serial.println();
}

void setup() {
  Serial.begin(9600);
  while(!Serial);
  
  Wire.begin();
  lightSensor.begin();

  pinMode(SWITCH_PIN, INPUT);
  pinMode(SENSOR_PIN, INPUT);
  pinMode(LEDR_PIN, OUTPUT);
  pinMode(LEDB_PIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), ISRtriggerLights, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), ISRmotionDetected, RISING);

  // warm up delay to give the PIR sensor time to adjust to the room conditions
  Serial.println("Sensor warming up...");
  delay(60000);
  Serial.println("Warmup complete");
}

void loop() {
  // Switch trumps all logic. If the switch is turned on, the lights will always be on.
  // else continue -> let other environment variables handle LEDs 
  if (switchActive) {
    if (ledState == LOW) 
      triggerLights(true);
    return;
  }
  
  float lux = lightSensor.readLightLevel(); 
  // holds state for motion sensor based on the sensor timeout. Compares current board uptime against potential ISR trigger board uptime
  bool motionTimeout = (millis() - lastMotionTrigger) < MOTION_TIMEOUT;

  // logging will only update serial if state change detected
  unsigned long now = millis();
  if (lux != lastLux || motionTimeout != lastMotionState || ledState != lastLedState || now - lastLog > LOG_INTERVAL) {
    lastLux = lux;
    lastMotionState = motionTimeout;
    lastLedState = ledState;
    lastLog = now;
    logLux(lux);
    logMotion(motionTimeout);
  }

  // If light levels are high, make sure lights are off then exit loop early
  if (lux > LIGHT_THRESHOLD){
    if (ledState == HIGH)
      triggerLights(false);
    return;
  }

  // Motion Sensor logic, if motionTimeout returns true, timer still running make sure lights on
  // else make LEDs are off. 
  if (!motionTimeout && ledState == HIGH)
    triggerLights(false);
  else if (motionTimeout && ledState == LOW)
    triggerLights(true);
}
