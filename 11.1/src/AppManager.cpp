#include "AppManager.h"
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <FreeRTOS_SAMD21.h>
#include "arduino_secrets.h"

static WiFiSSLClient wifiClient;
static PubSubClient mqtt(wifiClient);
static SemaphoreHandle_t mqttMutex = NULL;

const uint32_t RECONNECT_INTERVAL_MS = 5000;
const uint32_t MAX_DISCONNECT_MS = 30000;
static uint32_t disconnected_since_ms = 0;
static uint32_t last_reconnect_ms = 0;

typedef struct {
  const char* status;     // consolidated JSON: state, temp, progress
  const char* connected;  // retained, driven by LWT
  const char* start;
  const char* stop;
  const char* adjust_temp;
  const char* size;
} Topic;

const Topic topics = {
  .status = "smartbath/status",
  .connected = "smartbath/status/connected",
  .start = "smartbath/command/start",
  .stop = "smartbath/command/stop",
  .adjust_temp = "smartbath/command/temp",
  .size = "smartbath/command/size"
};

void subscribe_all(){
  mqtt.publish(topics.connected, "True", true);
  mqtt.subscribe(topics.start);
  mqtt.subscribe(topics.stop);
  mqtt.subscribe(topics.adjust_temp);
  mqtt.subscribe(topics.size);
}

void onMqttMsgReceived(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strcmp(topic, topics.start) == 0){
    char buf[32];
    snprintf(buf, sizeof(buf), "%.*s", (int)length, payload);
    request_start(atof(buf));
  }
  else if (strcmp(topic, topics.stop) == 0) {
    request_stop();
  }
  else if (strcmp(topic, topics.adjust_temp) == 0){
    char buf[32];
    snprintf(buf, sizeof(buf), "%.*s", (int)length, payload);
    set_target_temperature(atof(buf));
  }
  else if (strcmp(topic, topics.size) == 0){
    char buf[32];
    snprintf(buf, sizeof(buf), "%.*s", (int)length, payload);
    set_bath_size(atof(buf));
  }
}

// Connect functions
void wifi_connect(){
  Serial.println("Attempting wifi connection..");
  while (WiFi.begin(WIFI_SSID, WIFI_PASS) != WL_CONNECTED){
    delay(1000);
  }
  Serial.println("Wifi connected");
}

void mqtt_connect(){
  while(!mqtt.connected()){
    Serial.println("Attempting to connect to broker..");
    if (mqtt.connect(MQTT_CLIENT, MQTT_USER, MQTT_PASS, topics.connected, 1, true, "False")){
      Serial.println("Broker connected");
      subscribe_all();
    } else {
      Serial.print("Broker connection could not be established: "); Serial.println(mqtt.state());
      delay(1500);
    }
  }
}

void check_connection(){
  uint32_t now = millis();

  if (WiFi.status() == WL_CONNECTED && mqtt.connected()){
    disconnected_since_ms = 0;
    return;
  }

  if (disconnected_since_ms == 0) 
    disconnected_since_ms = now;

  if (now - last_reconnect_ms < RECONNECT_INTERVAL_MS) return;
  
  last_reconnect_ms = now;
  if (WiFi.status() != WL_CONNECTED){
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    return;
  }

  // WiFi up, MQTT down
  if (mqtt.connect(MQTT_CLIENT, MQTT_USER, MQTT_PASS, topics.connected, 1, true, "False")){
    subscribe_all();
  }
}

bool connection_timeout(uint32_t now){
  if (disconnected_since_ms == 0) return false;
  return (now - disconnected_since_ms) >= MAX_DISCONNECT_MS;
}

void mqtt_begin(){
  mqttMutex = xSemaphoreCreateMutex();
  wifi_connect();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setKeepAlive(15);
  mqtt.setSocketTimeout(2);
  mqtt.setCallback(onMqttMsgReceived);
  mqtt_connect();
}

void mqtt_loop(){
  if (mqttMutex == NULL) return;
  if (xSemaphoreTake(mqttMutex, portMAX_DELAY) == pdTRUE){
    check_connection();
    mqtt.loop();
    xSemaphoreGive(mqttMutex);
  }
}

void publish_status(BathState state, float temp, int progress){
  if (mqttMutex == NULL) return;
  if (xSemaphoreTake(mqttMutex, portMAX_DELAY) != pdTRUE) return;

  char payload[96];
  snprintf(payload, sizeof(payload),
           "{\"state\":\"%s\",\"temp\":%.1f,\"progress\":%d}",
           stringify_bathState(state), temp, progress);

  if (!mqtt.publish(topics.status, payload))
    Serial.println("Error publishing status");

  xSemaphoreGive(mqttMutex);
}