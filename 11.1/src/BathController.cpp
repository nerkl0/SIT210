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

static bool want_start = false;
static bool want_stop  = false;
static uint32_t fill_start_ms = 0;

void request_start(float target) {
  target_temp = target;
  want_start = true;
}

void request_stop() {
  want_stop = true;
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
  if (want_stop) {
    want_stop = false;
    fill_start_ms = 0;
    return IDLE;
  }

  if (want_start && bath_state == IDLE) {
    want_start = false;
    fill_start_ms = now;
    return FILLING;
  }
  want_start = false;

  if (bath_state == IDLE) 
    return IDLE;

  if (!sensor_ok) 
    return SENSOR_FAULT;

  if (connection_timed_out(now)) 
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
  switch (st) {
    case HARD_WARNING:
    case SOFT_WARNING:
      if (err > 0) { 
        stop_pump(HOT);
        run_pump(COLD);
        }
      else { 
        stop_pump(COLD); 
        run_pump(HOT);  
      }
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
  max_fill_ms = (uint32_t)((litres / PUMP_FLOW_RATE_LPM) * 60.0 * 1000.0);
  Serial.print("BATH bath size: "); Serial.print(litres);
  Serial.print("L -> max_fill_ms="); Serial.println(max_fill_ms);
}
void set_target_temperature(float t){
  target_temp = t;
  Serial.print("[BATH] target temp: "); Serial.println(target_temp);
}
void set_bath_state(BathState st){ 
  bath_state = st; 
}