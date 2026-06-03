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
  // publish topics
  const char* status;  // state, temp, progress
  const char* connected;
  // subscribe topics
  const char* start;
  const char* stop;
  const char* adjust_temp;
  const char* size;
  const char* silence;
} Topic;

const Topic topics = {
  .status = "smartbath/status",
  .connected = "smartbath/status/connected",
  .start = "smartbath/command/start",
  .stop = "smartbath/command/stop",
  .adjust_temp = "smartbath/command/temp",
  .size = "smartbath/command/size",
  .silence = "smartbath/command/silence"
};

// Is called on connection of broker 
void subscribe_all(){
  mqtt.publish(topics.connected, "True", true); // "True": Payload, true: retainer (broker keeps topic so newly-connected subscriber gets last known publish)
  mqtt.subscribe(topics.start);
  mqtt.subscribe(topics.stop);
  mqtt.subscribe(topics.adjust_temp);
  mqtt.subscribe(topics.size);
  mqtt.subscribe(topics.silence);
}

/*
  MQTT message callback function. Copies the payload into a buffer publishes via respective topics
  In order: 
    - start: assign target temperature and start filling 
    - stop: stop filling
    - adjust_temp: bath is filling and user has adjusted temp
    - size: size of bath literage has changed
    - silence: HARD_WARNING or SENSOR_FAULT state, user has requested buzzer to be silent  
*/
void onMqttMsgReceived(char* topic, byte* payload, unsigned int length){
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
  else if (strcmp(topic, topics.silence) == 0){ 
    request_silence(); // Buzzer silenced
  }
}

// Non-blocking reconnect check. Tracks how long the system has been down (disconnected_since_ms)
// Retries attempt once per RECONNECT_INTERVAL_MS. WiFi is recovered before MQTT.
void check_connection(){
  uint32_t now = millis();

  if (WiFi.status() == WL_CONNECTED && mqtt.connected()){
    disconnected_since_ms = 0;
    return;
  }

  // If just disconnected set start disconnection period
  if (disconnected_since_ms == 0) 
    disconnected_since_ms = now;

  // Avoids spamming reconnection attempts, returns while within reconnect interval
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

// Tracks connection timeout for bath. If true and bath is filling, the bath pumps stop
bool connection_timeout(uint32_t now){
  if (disconnected_since_ms == 0) 
    return false;
  return (now - disconnected_since_ms) >= MAX_DISCONNECT_MS;
}

// Publishes the current bath status as JSON object ({state, temp, progress}) via topics.status.
// Called from normal-priority task; takes mqttMutex so it doesn't collide with mqtt_loop()'s broker access
void publish_status(BathState state, float temp, int progress){
  if (mqttMutex == NULL) return;
  if (xSemaphoreTake(mqttMutex, portMAX_DELAY) != pdTRUE) return;

  char payload[96];
  snprintf(payload, sizeof(payload), "{\"state\":\"%s\",\"temp\":%.1f,\"progress\":%d}",
           stringify_bathState(state), temp, progress);

  if (!mqtt.publish(topics.status, payload))
    Serial.println("Error publishing status");

  xSemaphoreGive(mqttMutex);
}

//  ===== Connect functions =======
void wifi_connect(){
  while (WiFi.begin(WIFI_SSID, WIFI_PASS) != WL_CONNECTED){
    delay(1000);
  }
}

/* 
  Connect to the broker (blocking function). 
  mqtt.connect args: 
  - topics.connected: registers the LWT 
  - 1: will QoS
  - true: will retained 
  - "False": will payload, broker publishes this if system drops
*/
void mqtt_connect(){
  while(!mqtt.connected()){
    if (mqtt.connect(MQTT_CLIENT, MQTT_USER, MQTT_PASS, topics.connected, 1, true, "False")){
      subscribe_all();
    } else {
      delay(1500);
    }
  }
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