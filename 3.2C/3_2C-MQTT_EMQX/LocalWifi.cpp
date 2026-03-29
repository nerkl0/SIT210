#include <WiFiNINA.h>
#include "arduino_secrets.h"

WiFiClient wifiClient;                 // initialise secure wifi client

// Wifi & Broker credentials
const char SSID[] = SECRET_SSID;
const char PSWD[] = SECRET_PWD;

void connectLocalWifi(){
  Serial.print("Attempting connection to SSID: ");
  Serial.println(SSID);
  while (WiFi.begin(SSID, PSWD) != WL_CONNECTED) {
    delay(5000);
  }
  Serial.println("Wifi connection established");
}