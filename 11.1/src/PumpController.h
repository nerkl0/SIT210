#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <Arduino.h>

enum PUMP { HOT, COLD };

void pumpController_begin();
void run_pump(int pump);
void stop_pump(int pump);
void run_both_pumps();
void stop_both_pumps(); 

#endif