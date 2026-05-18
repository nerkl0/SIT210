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

const int MAX_PWM = 255;
const float SCALE = 300 / MAX_PWM; 

const int lightCount = sizeof(lights) / sizeof(lights[0]);

void setupDevices(){
  for (int i = 0; i < lightCount; i++){
    pinMode(lights[i].pin, OUTPUT);
    digitalWrite(lights[i].pin, LOW);
  }

  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);
}

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

void powerFan(bool power)
  digitalWrite(FAN_PIN, power);


int adjustLedPwm(float lux)
  return constrain((int)(MAX_PWM - (lux*SCALE)), 30, MAX_PWM);


void processSignal(String str){
  int split = str.indexOf(':');
  String deviceStr = str.substring(0, split);
  String value = str.substring(split + 1);
  deviceStr.toLowerCase();

  const char* deviceName = deviceStr.c_str();
  int intValue = value.toInt();

  Serial.print("Device: "); Serial.println(deviceName);
  Serial.print("Value: "); Serial.println(intValue);

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
    Serial.print("Connected to: "); Serial.println(bleConnect.address());
    while (BLE.connected()) {
      if (ble_request.written()) {
        String req = ble_request.value();
        processSignal(req);
      }
    }
    Serial.println("Bluetooth Connection Disconnected");
  }
}