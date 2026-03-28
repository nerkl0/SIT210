#include "LocalWifi.h"
#include "Broker.h"
#include "arduino_secrets.h"

const char EMQX_HOST[] = "broker.emqx.io";
const int EMQX_PORT=1883;

typedef struct {
  const char* wave;
  const char* touch;
} Topics;

const Topics topics = {
  .wave="sensor/wave",
  .touch="sensor/touch"
};

void setup() {
  connectLocalWifi();
  connectToBroker();
}

void loop() {
  mqttClient.loop();
}