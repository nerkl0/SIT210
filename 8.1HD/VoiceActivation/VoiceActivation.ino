#include <Wire.h>
#include <BH1750.h>
#include <WiFiNINA.h> 
#include "Bluetooth.h"

BH1750 lightSensor;

const int FAN_PIN = 6; 

struct Device {
  const char* name;
  const int pin;
  int pwm; 
};

Device lights[]={
  {"living room", 9, 0},
  {"bathroom", 10, 0},
  {"closet", 11, 0}
};
const int lightCount = sizeof(lights) / sizeof(lights[0]);

const int MAX_PWM = 255; // max pwm for LEDs
// Scaler for adjusting to room light level. 300 is an arbitrary max room value (could be lowered)
const float SCALE = 300 / MAX_PWM;

// Called in setup(), initialises LED and fan pins
void setupDevices(){
  for (int i = 0; i < lightCount; i++){
    pinMode(lights[i].pin, OUTPUT);
    digitalWrite(lights[i].pin, LOW);
  }

  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);
}

/*
  Searches for room within room array. 
  If found:
    value = 0: turn off light
    value = 255: turn on light and adjust for room light level based on lightSensor reading.
    value > 0 and < 255: set light level based on value
  If the room is bathroom, also toggle fan
*/
void powerLight(const char* room, int value){
  for (int i = 0; i < lightCount; i++){
    if (strcmp(lights[i].name, room) == 0){
      switch (value) {
        case 0: 
          lights[i].pwm = 0; break;
        case 255:
          lights[i].pwm = adjustLedPwm(lightSensor.readLightLevel());
          break; 
        default: 
          lights[i].pwm = value; break; 
      }

      if (strcmp(room, "bathroom") == 0)
        powerFan(lights[i].pwm > 0);
      
      analogWrite(lights[i].pin, lights[i].pwm);
      return;
    }
  }
}

// Handles powering on the fan
void powerFan(bool power)
  digitalWrite(FAN_PIN, power);


/* 
  Adjusts level of LED by taking the light in the room level and scaling
  Uses constrain to keep LED between a minimum of 30 and MAX_PWM constant 
*/
int adjustLedPwm(float lux)
  return constrain((int)(MAX_PWM - (lux*SCALE)), 20, MAX_PWM);


/*
  Splits argument string at colon. configures device name to a lowercase char* and converts pwm value to int
  If device is the fan, power fan and return immediately (do not need to adjust pwm)
  Else call powerLight
*/
void processSignal(String str){
  int split = str.indexOf(':');
  String deviceStr = str.substring(0, split);
  String value = str.substring(split + 1);
  deviceStr.toLowerCase();

  const char* deviceName = deviceStr.c_str();
  int intValue = value.toInt();

  if (strcmp(deviceName, "fan") == 0){
    powerFan(intValue);
    return;
  }
  powerLight(deviceName, intValue);
}

void setup() {
  Serial.begin(9600);

  Wire.begin();         // Initialize the I2C bus for light connection
  lightSensor.begin();
  bleBegin();
  
  setupBLE();
  setupDevices();
}

void loop() {
  BLEDevice bleConnect = BLE.central(); 
  if(bleConnect){
    // While bluetooth is connected, if a signal is received, convert to string and call processSignal()
    while (BLE.connected()) {
      if (ble_request.written()) {
        String req = ble_request.value();
        processSignal(req);
      }
    }
  }
}