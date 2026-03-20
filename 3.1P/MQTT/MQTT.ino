#include <Wire.h>
#include <PubSubClient.h>
#include <BH1750.h>
#include <DHT.h>
#include <WiFiNINA.h>

#include "arduino_secrets.h"

#define DHT11_PIN 2
DHT dht11(DHT11_PIN, DHT11);
BH1750 lightSensor;

WiFiSSLClient wifiSSLClient;                 // initialise secure wifi client
PubSubClient mqttClient(wifiSSLClient);      // connect the wifi client to MQTT

// Wifi & Broker credentials
const char SSID[] = SECRET_SSID;
const char PSWD[] = SECRET_PWD;
const char MQTT_USER[] = SECRET_MQTT_USER;
const char MQTT_PASSWORD[] = SECRET_MQTT_PASSWORD;
const char MQTT_SERVER[] = SECRET_MQTT_SERVER;
const int MQTT_PORT = SECRET_MQTT_PORT;

typedef struct {
  const char* light;
  const char* temp;
} Topics;

const Topics topics = {
  .light = "sensor/light",
  .temp  = "sensor/temp"
};


void connectBroker(){
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  Serial.println("Attempting connection to the MQTT broker");
  if (!mqttClient.connect("ArduinoNanoIoT", MQTT_USER, MQTT_PASSWORD)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.state());

    while (1);
  }
  Serial.println("Broker connection established");
}

void connectWifi(){
  Serial.print("Attempting connection to SSID: ");
  Serial.println(SSID);
  while (WiFi.begin(SSID, PSWD) != WL_CONNECTED) {
    delay(5000);
  }
  Serial.println("Wifi connection established");
}

void printToTerminal(float light, float temp){
  Serial.print("   ");
  Serial.print(light);
  Serial.print("     |       ");
  Serial.println(temp);
}

// variables for light notifications
bool light_notification_sent = false;
bool temp_notification_sent = false;

void sendNotification(const char* topic, const char* message) {
  Serial.print("Change in ");
  Serial.print(topic);
  Serial.println(" detected. Processing Alert...");

  if (!mqttClient.connected()) {
    connectBroker();
  }

  mqttClient.publish(topic, message);
}

void updateStatus(bool &topicStatus, bool val){
  topicStatus = val;
}

void setup() {
  Serial.begin(9600);
  while(!Serial);

  Wire.begin();         // Initialize the I2C bus for light connection
  lightSensor.begin();
  dht11.begin();

  connectWifi();
  connectBroker();
  
  Serial.println("   Light     |    Temperature");
}

void loop() {
  mqttClient.loop();

  float lux = lightSensor.readLightLevel();
  float temp = dht11.readTemperature();
  printToTerminal(lux, temp);

  // handles changes in light / temperature. Will only send one notification per threshold change
  if(lux < 25 && !light_notification_sent) {
    sendNotification(topics.light, "Light Threshold Hit");
    updateStatus(light_notification_sent, true);
  } else if (lux > 25 && light_notification_sent) {
    sendNotification(topics.light, "Light Stable");
    updateStatus(light_notification_sent, false);
  }

  if(temp < 20 && !temp_notification_sent) {
    sendNotification(topics.temp, "Temp Threshold Hit");
    updateStatus(temp_notification_sent, true);
  } else if (temp > 20 && temp_notification_sent) {
    sendNotification(topics.temp, "Temp Stable");
    updateStatus(temp_notification_sent, false);
  }
  
  delay(5000);
}

