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

// Below three functions handle the app publishing start/stop and silence
// Updates flags for evaluate_state() logic
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

/*
  Calculates whether the temperature is in a healthy state. If yes, returns FILLING state. 
  If the difference between the current temperature and the target temperature is >= MAX_TEMP_DIFF band
  or if current temperature is greate than the maximum allowed temp (MAX_TEMPERATURE) state is set to HARD_WARNING 
  If the temperature is <= TEMP_BAND state is set to SOFT_WARNING
*/
static BathState temperature_state(float temp) {
  float diff = temp - target_temp;
  if (abs(diff) > MAX_TEMP_DIFF || temp > MAX_TEMPERATURE)
    return diff > 0 ? HARD_WARNING : SOFT_WARNING; // SOFT_WARNING only for colder than temperature 

  if (abs(diff) >= TEMP_BAND)
    return SOFT_WARNING;
  
  return FILLING;
}

// The main function called for transiting through states
// Updates state flags using current temperature, current time and current sensor state 
BathState evaluate_state(float temp, uint32_t now, bool sensor_ok) {
  // If stop button has been pressed, reset the fill_start counter to 0. return 
  if (stop_requested) {
    stop_requested = false;
    fill_start_ms = 0;
    return temp >= target_temp + MAX_TEMP_DIFF ? HARD_WARNING : IDLE;
  }
  
  // Only update bath_state on start request if state is currently IDLE
  if (start_requested && bath_state == IDLE) {
    start_requested = false;
    fill_start_ms = now;
    return FILLING;
  }
  start_requested = false;  // reset start_requested to default

  // Early return. If bath state is IDLE, nothing after this point needs actioning
  if (bath_state == IDLE) 
    return IDLE;

  // Sensor Health check
  if (!sensor_ok) {
    fill_start_ms = 0;
    return SENSOR_FAULT;
  }
  
  // If connection to app has dropped longer than the set timer, update state to LOST_CONNECTION
  if (connection_timeout(now)) 
    return LOST_CONNECTION;

  // Bath has finished filling. Reset fill timer, set state to IDLE
  if ((now - fill_start_ms) >= max_fill_ms) {
    fill_start_ms = 0;
    return IDLE;
  }

  // Bath still filling, normal temp monitor check 
  return temperature_state(temp);
}

// Actuator: Drives the pumps related to set state
void drive_pumps(BathState st, float curr_temp) {
  float err = curr_temp - target_temp;
  switch (st) {
    case HARD_WARNING:
    case SOFT_WARNING:
      if (fill_start_ms == 0) { 
        stop_both_pumps(); 
        break; 
      } 
      // If too hot: stop hot pump, keep cold pump running
      // If too cold: keep hot pump running, stop cold pump
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

// Getter Functions
bool buzzer_silenced() {
  return buzzer_is_silenced;
}
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