#define TS_ENABLE_SSL

#include <WiFiNINA.h>
#include <ThingSpeak.h>
#include "arduino_secrets.h"
#include "utils.h"

// Wifi credentials
const char SSID[] = SECRET_SSID;
const char PSWD[] = SECRET_PWD;

int WIFI_status = WL_IDLE_STATUS;

// ThingSpeak credentials
unsigned long tsChannelNum = SECRET_CH_NUM;
const char API_KEY[] = SECRET_API_WRITE;

WiFiSSLClient client;              // initialise wifi client library

// Handles connection to wifi
bool connectToWifi(){
  Serial.println("Attempting connection to network");
  
  int WIFI_status = WiFi.begin(SSID, PSWD);
  while (WIFI_status != WL_CONNECTED) {
    delay(1000);
    WIFI_status = WiFi.status();
  }

  bool success = WL_CONNECTED ? true : false;
  formatPrint("Wifi connection: ", success);
  return success;
}


void connectToThingSpeak(){
  Serial.println("Attempting cnnection to ThingSpeak");
  bool success = ThingSpeak.begin(client);
  formatPrint("ThingSpeak connection: ", success);
}


void sendDataThingSpeak(float temp, int light){
  if (isnan(temp)) {
    Serial.println("Could not read DHT sensor");
    return;
  }

  ThingSpeak.setField(1, temp);
  ThingSpeak.setField(2, light);

  int responseCode = ThingSpeak.writeFields(tsChannelNum, API_KEY);

  if (responseCode == 200) {
    formatPrint("ThingSpeak Data Sent: ", true);
  } else {
    formatPrint("ThingSpeak Data Sent: ", false);
    Serial.print("Error: ");
    Serial.println(responseCode);
  }
}