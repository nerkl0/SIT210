#include "BathController.h"
#include "PumpController.h"
#include "AppManager.h"

#define MAX_TEMPERATURE 42 // Max safe temperature
#define PUMP_FLOW_RATE 10.0 // litres per min on each pump, used to estimate fill time
static uint32_t max_fill_ms = 12000; // fill duration 
static uint32_t fill_start_ms = 0; // timer for when filling began

static BathState bath_state = IDLE;
static const float MAX_TEMP_DIFF = 5.0;
static float target_temp = 0;
static const int TEMP_BAND = 1;

// Written from MQTT callback, read in evaluate_state()
static bool volatile start_requested = false;
static bool volatile stop_requested  = false;
static bool volatile buzzer_is_silenced = false;

// Bath state machine holds all pump controller logic. Setup pins for the pumps
void bathController_begin(){
  pumpController_begin();
}

// Application has requested start  
void request_start(float target) {
  target_temp = target;
  start_requested = true;
  buzzer_is_silenced = false;  // new bath starts un-silenced
}

void request_stop() {
  stop_requested = true;
}

void request_silence() {
  buzzer_is_silenced = true;
}

bool buzzer_silenced() {
  return buzzer_is_silenced;
}

static BathState temperature_state(float temp) {
  float diff = temp - target_temp;
  if (abs(diff) > MAX_TEMP_DIFF || temp > MAX_TEMPERATURE)
    return diff > 0 ? HARD_WARNING : SOFT_WARNING;

  if (abs(diff) >= TEMP_BAND)
    return SOFT_WARNING;
  
  return FILLING;
}

BathState evaluate_state(float temp, uint32_t now, bool sensor_ok) {
  if (stop_requested) {
    stop_requested = false;
    fill_start_ms = 0;
    return temp >= target_temp + MAX_TEMP_DIFF ? HARD_WARNING : IDLE; ;
  }

  if (start_requested && bath_state == IDLE) {
    start_requested = false;
    fill_start_ms = now;
    return FILLING;
  }
  start_requested = false;

  if (bath_state == IDLE) 
    return IDLE;

  if (!sensor_ok) {
    fill_start_ms = 0;
    return SENSOR_FAULT;
  }

  if (connection_timeout(now)) 
    return LOST_CONNECTION;

  if ((now - fill_start_ms) >= max_fill_ms) {
    fill_start_ms = 0;
    return IDLE;
  }

  return temperature_state(temp);
}

// Actuator: drive pumps for a given state. No state writes.
void drive_pumps(BathState st, float curr_temp) {
  float err = curr_temp - target_temp;
  switch (st) {
    case HARD_WARNING:
    case SOFT_WARNING:
      if (fill_start_ms == 0) { 
        stop_both_pumps(); 
        break; 
      } 
      stop_pump(err > 0 ? HOT : COLD);
      run_pump(err > 0 ? COLD : HOT);
      break;
    case FILLING:
      run_both_pumps();
      break;
    case IDLE:
    case LOST_CONNECTION:
    case SENSOR_FAULT:
      stop_both_pumps();
      break;
  }
}

// Getters
BathState get_bath_state(){ 
  return bath_state; 
}
int get_fill_progress() {
  uint32_t start = fill_start_ms;
  uint32_t span  = max_fill_ms;
  if (start == 0 || span == 0) return 0;

  uint32_t elapsed = millis() - start;
  int pct = (int)((float)elapsed / span * 100.0);
  
  return pct > 100 ? 100 : pct;
}
const char* stringify_bathState(BathState st){
  switch (st) {
    case IDLE: return "IDLE";
    case FILLING: return "FILLING";
    case SOFT_WARNING:return "SOFT_WARNING";
    case HARD_WARNING: return "HARD_WARNING";
    case LOST_CONNECTION: return "LOST_CONNECTION";
    case SENSOR_FAULT: return "SENSOR_FAULT";
    default: 
      return "UNKNOWN STATE";
  }
}

// setters
void set_bath_size(float litres) {
  max_fill_ms = (uint32_t)((litres / PUMP_FLOW_RATE) * 60.0 * 1000.0);
}
void set_target_temperature(float t){
  target_temp = t;
}
void set_bath_state(BathState st){ 
  bath_state = st; 
}