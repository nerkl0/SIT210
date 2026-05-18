#include "Bluetooth.h"

// Setup BT service, characteristic UUID must match the sender UUID
BLEService lightService("88d6aab0-34ef-44dc-87c9-a2b070969a46");
BLEStringCharacteristic ble_request("5fcc86b2-1b9a-4316-8845-98c6ec20a1bc", BLERead | BLEWrite, 20);

void bleBegin(){
  if(!BLE.begin()) {
    Serial.println("Error connecting to Bluetooth");
    while(1);
  }
}

void setupBLE(){
  BLE.setLocalName("Lindas Lighting Rig");
  BLE.setAdvertisedService(lightService);
  lightService.addCharacteristic(ble_request);
  BLE.addService(lightService);
  ble_request.writeValue("Ready");
  BLE.advertise(); 
  Serial.println("BLE setup. Ready for connection");
}

