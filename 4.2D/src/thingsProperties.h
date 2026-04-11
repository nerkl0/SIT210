#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include "arduino_secrets.h"

void onAllOffChange();
void onAllOnChange();
void onBathroomChange();
void onClosetChange();
void onLivingRoomChange();

CloudLight AllOff;
CloudLight AllOn;
CloudLight Bathroom;
CloudLight Closet;
CloudLight LivingRoom;

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PWD);

void initProperties() {
  ArduinoCloud.addProperty(AllOff, READWRITE, ON_CHANGE, onAllOffChange);
  ArduinoCloud.addProperty(AllOn, READWRITE, ON_CHANGE, onAllOnChange);
  ArduinoCloud.addProperty(Bathroom, READWRITE, ON_CHANGE, onBathroomChange);
  ArduinoCloud.addProperty(Closet, READWRITE, ON_CHANGE, onClosetChange);
  ArduinoCloud.addProperty(LivingRoom, READWRITE, ON_CHANGE, onLivingRoomChange);
}