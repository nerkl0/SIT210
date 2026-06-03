#ifndef BATH_CONTROLLER_H
#define BATH_CONTROLLER_H
#include <Arduino.h>

enum BathState { IDLE, FILLING, SOFT_WARNING, HARD_WARNING, LOST_CONNECTION, SENSOR_FAULT };

BathState evaluate_state(float temp, uint32_t now, bool sensor_ok);
void drive_pumps(BathState st, float curr_temp);
void request_start(float target);
void request_stop();
void request_silence();
void bathController_begin();

// Getters
BathState get_bath_state();
int get_fill_progress();
bool buzzer_silenced();
const char* stringify_bathState(BathState st);

// Setters
void set_bath_state(BathState st);
void set_target_temperature(float t);
void set_bath_size(float litres);

#endif