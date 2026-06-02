#include "BathController.h"
#include "PumpController.h"
#include "AppManager.h"

#define MAX_TEMPERATURE 42
#define LEVEL_HARD_CUTOFF 95
#define SENSOR_GRACE_MS 5000
#define PUMP_FLOW_RATE_LPM 10.0

static uint32_t max_fill_ms = 12000;

static BathState bath_state = IDLE;
static const float MAX_TEMP_DIFF = 5.0;
static float target_temp = 0;
static const int TEMP_BAND = 1;

static bool volatile start_requested = false;
static bool volatile stop_requested  = false;
static uint32_t fill_start_ms = 0;

void bathController_begin(){
  pumpController_begin();
}

void request_start(float target) {
  target_temp = target;
  start_requested = true;
}

void request_stop() {
  stop_requested = true;
}

static BathState temperature_state(float temp) {
  float t = temp - target_temp;
  if (abs(t) > MAX_TEMP_DIFF || temp > MAX_TEMPERATURE)
    return t > 0 ? HARD_WARNING : SOFT_WARNING;
  if (abs(t) >= TEMP_BAND)
    return SOFT_WARNING;
  return FILLING;
}

BathState evaluate_state(float temp, uint32_t now, bool sensor_ok) {
  if (stop_requested) {
    stop_requested = false;
    fill_start_ms = 0;
    return IDLE;
  }

  if (start_requested && bath_state == IDLE) {
    start_requested = false;
    fill_start_ms = now;
    return FILLING;
  }
  start_requested = false;

  if (bath_state == IDLE) 
    return IDLE;

  if (!sensor_ok) 
    return SENSOR_FAULT;

  if (connection_timeout(now)) 
    return LOST_CONNECTION;

  if ((now - fill_start_ms) >= max_fill_ms) {
    Serial.println("Evaluate -> IDLE Bath full");
    fill_start_ms = 0;
    return IDLE;
  }

  return temperature_state(temp);
}

// Actuator: drive pumps for a given state. No state writes.
void drive_pumps(BathState st, float curr_temp) {
  float err = curr_temp - target_temp;
  Serial.println();
  Serial.println("======= In drive_pumps =======");
  Serial.print("Bath state: "); Serial.println(stringify_bathState(bath_state));
  Serial.print("Target temp: "); Serial.println(target_temp);
  Serial.println(); 
  Serial.print("Current temp: "); Serial.println(curr_temp);
  Serial.print("In drive_pumps. current-target=");Serial.println(err);
  switch (st) {
    case HARD_WARNING:
    case SOFT_WARNING:
      if (err > 0) { 
        stop_pump(HOT);
        run_pump(COLD);
        Serial.println("Stopping hot pump");
      }
      else { 
        stop_pump(COLD); 
        run_pump(HOT);  
        Serial.println("Stopping cold pump");
      }
      break;
    case FILLING:
      run_both_pumps();
      Serial.println("Running both pumps");
      break;
    case IDLE:
    case LOST_CONNECTION:
    case SENSOR_FAULT:
      stop_both_pumps();
      Serial.println("Stopping both pumps");
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
    int pct = elapsed / span * 100.0;
    
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
  max_fill_ms = (uint32_t)((litres / PUMP_FLOW_RATE_LPM) * 60.0 * 1000.0);
}
void set_target_temperature(float t){
  target_temp = t;
  Serial.print("BATH target temp: "); Serial.println(target_temp);
}
void set_bath_state(BathState st){ 
  bath_state = st; 
}