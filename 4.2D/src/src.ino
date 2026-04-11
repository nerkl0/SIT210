#include <ArduinoIoTCloud.h>
#include "thingsProperties.h"

struct PinState  {
  int pin;
  const char* name; 
};

PinState  pins[] = {
  {7, "Living"},
  {8, "Bathroom"},
  {9, "Closet"}
};

void setup() {
  Serial.begin(9600);
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  for(auto &p : pins){
    pinMode(p.pin, OUTPUT);
  }
}

void loop() {
  ArduinoCloud.update(); 
}

void setPin(const char* name, bool power){
  for (auto &p : pins) {
    if (strcmp(p.name, name) == 0) {
      digitalWrite(p.pin, power);
    }
  }
}

void updateAll(bool power) {
  for (auto &p : pins) {
    digitalWrite(p.pin, power);
  }
   
  LivingRoom = power;
  Bathroom = power;
  Closet = power;
}

void onLivingRoomChange(){
  setPin("Living", LivingRoom);
}

void onBathroomChange(){
  setPin("Bathroom",  Bathroom);
}

void onClosetChange(){
  setPin("Closet", Closet);
}
void onAllOnChange() {
  if (AllOn){
    updateAll(true);
    AllOn = false;
  }
}
void onAllOffChange(){
  if (AllOff){
    updateAll(false);
    AllOff = false;
  }
}