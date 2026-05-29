#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include "BathController.h"
#include "TempSensor.h"

void mqtt_begin();
void mqtt_loop();
void publish_status(BathState state, float temp, int progress);
void publish_notification(const char* msg);
bool connection_timed_out(uint32_t now);
void publish_notification(const char* msg);

#endif