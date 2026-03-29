#define MAX_SUBS 10

#include <PubSubClient.h>
#include "LocalWifi.h" 
#include "arduino_secrets.h"

PubSubClient mqttClient(wifiClient);

// defines a pointer function used to store and call the onReceive Subscriber member
typedef void (*TopicFunction)(String message);

const char MQTT_SERVER[] = SECRET_MQTT_SERVER;
const int MQTT_PORT = SECRET_MQTT_PORT;
const char MQTT_CLIENT[] = "ArduinoNanoIoT";

typedef struct {
  const char* topic; 
  TopicFunction onReceive;
} Subscriber;

Subscriber subscriptions[MAX_SUBS];
int topicSubs = 0;

// handles connection to the broker, will attempt to connect 10 times before halting
void connectToBroker(){
  int attempts= 0; 
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  Serial.println("Attempting connection to the MQTT broker");
  if (!mqttClient.connect(MQTT_CLIENT)) {
    attempts++; 
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.state());

    if (attempts >= 10) {
      Serial.println("Max attempts reached, halting.");
      while(1);
    }
    Serial.print("Reattempting connection: ");
    Serial.println(attempts);
    delay(2000);
  }
  Serial.println("Broker connection established");
}

// parseMessage is used as the callback function for the broker. 
// on receipt of the payload, if the topic is found in subscriptions, the onReceive function attributed
// to the topic is called.
// If no topic is found, logs to serial monitor. 
void parseMessage(char* topic, byte* payload, unsigned int length){
  String message = String((char*)payload).substring(0, length);   // convert to arduino String

  for (int i = 0; i < topicSubs; i++) {
    if (strcmp(topic, subscriptions[i].topic) == 0){
      subscriptions[i].onReceive(message);
      return;
    }
  }
  Serial.print("No handler found for topic: ");
  Serial.println(topic);
}

// handles publishing data to the broker. 
// Reconnects to broker if connection has been terminated. 
void publishToBroker(const char* topic, const char* message) {
  Serial.println("Publishing Data...");
  if (!mqttClient.connected())
    connectToBroker();

  mqttClient.publish(topic, message);
}

// sends subscription request MQTT broker.
// If max subscriptions reached, logs error and returns
// Otherwise, adds topic and onReceive handler to the subscriptions array
void subscribeToTopic(const char* topic, TopicFunction onReceive){
  if (topicSubs >= MAX_SUBS) {
    Serial.print("Max subscriptions reached, cannot subscribe to: ");
    Serial.println(topic);
    return;
  }
  mqttClient.subscribe(topic);
  subscriptions[topicSubs++] = {topic, onReceive};
}

