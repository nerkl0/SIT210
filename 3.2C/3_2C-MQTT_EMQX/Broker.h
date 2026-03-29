#ifndef BROKER_H
#define BROKER_H
#include <WiFiNINA.h>
#include <PubSubClient.h> 

extern PubSubClient mqttClient;
typedef void (*TopicFunction)(String message);

void connectToBroker(void);
void parseMessage(char* topic, byte* payload, unsigned int length);
void publishToBroker(const char* topic, const char* message);
void subscribeToTopic(const char* topic, TopicFunction onReceive);

#endif