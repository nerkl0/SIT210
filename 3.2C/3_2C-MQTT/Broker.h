#ifndef BROKER_H
#define BROKER_H
#include <WiFiNINA.h>
#include <PubSubClient.h> 

extern PubSubClient mqttClient;

void connectToBroker(void);
void publishToBroker(char* topic, char* message);
void brokerParseMessage(void);

#endif