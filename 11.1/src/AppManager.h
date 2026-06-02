#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include "BathController.h"
#include "TempSensor.h"

void mqtt_begin();
void mqtt_loop();
void publish_status(BathState state, float temp, int progress);
bool connection_timeout(uint32_t now);

#endif