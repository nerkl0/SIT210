#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <OneWire.h>
#include <DallasTemperature.h>

enum TempSensorState { OK, BAD };

void tempSensor_begin();
float tempSensor_read();
TempSensorState tempSensor_state();

#endif