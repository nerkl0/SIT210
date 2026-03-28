#include <PubSubClient.h>
#include "LocalWifi.h" 
#include "arduino_secrets.h"

PubSubClient mqttClient(wifiClient);      // connect the wifi client to MQTT

const char MQTT_USER[] = SECRET_MQTT_USER;
const char MQTT_PASSWORD[] = SECRET_MQTT_PASSWORD;
const char MQTT_SERVER[] = SECRET_MQTT_SERVER;
const int MQTT_PORT = SECRET_MQTT_PORT;

void connectToBroker(){
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  Serial.println("Attempting connection to the MQTT broker");
  if (!mqttClient.connect("ArduinoNanoIoT", MQTT_USER, MQTT_PASSWORD)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.state());

    while (1);
  }
  Serial.println("Broker connection established");
}


void publishToBroker(char* topic, char* message) {
  Serial.println("Publishing Data...");

  if (!mqttClient.connected()) {
    connectToBroker();
  }

  mqttClient.publish(topic, message);
}

void brokerParseMessage(){
  int messageSize = mqttClient.parseMessage();
  if (messageSize) {

    Serial.print("Received a message with topic '");
    Serial.print(mqttClient.messageTopic());
    Serial.print("', length ");
    Serial.print(messageSize);
    Serial.println(" bytes:");

    while (mqttClient.available()) {
      Serial.print((char)mqttClient.read());
    }
    Serial.println();
}