#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <ArduinoBLE.h>

extern BLEStringCharacteristic ble_request; 

void bleBegin();
void setupBLE(); 

#endif