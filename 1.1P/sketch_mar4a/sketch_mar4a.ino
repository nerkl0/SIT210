void handleTimers(void);
void handleSwitch(bool power);

// Pin allocation
const int SWITCH_PIN = 2;
const int LED_PORCH = 4;
const int LED_HALLWAY = 8;

// Storage for easy access to pins
const int PINS[] = { LED_PORCH, LED_HALLWAY };
const int PIN_COUNT = 2;

// Holds the current state of the LED program 
// is updated after the switch gets activated/deactivated
bool currentState = false; 

// Timers
struct TimerState {
  unsigned long start = 0;
  unsigned long current = 0;
  const unsigned long PORCH_MAX = 30000;
  const unsigned long HALLWAY_MAX = 60000;
} Timer;

void setup() {
  pinMode(SWITCH_PIN, INPUT);
  pinMode(LED_PORCH, OUTPUT);
  pinMode(LED_HALLWAY, OUTPUT);
}

void loop() {
  bool switchOn = digitalRead(SWITCH_PIN);

  // if the switch has been read as active and the current state of the program is off, 
  // steps into the first if block and switches on LEDs. If the inverse, the lights get turned off
  if(switchOn && !currentState) {
    handleSwitch(true);
    Timer.start = millis();
  } else if(!switchOn && currentState)
    handleSwitch(false);

  if(currentState) // if current state is on, the program updates the timers on each LED
    handleTimers();
}

// handleSwitch takes a boolean argument that triggers the LED lights on/off
// it then alters the currentState to match
void handleSwitch(bool power) {
  for(int i = 0; i < PIN_COUNT; i++){
    digitalWrite(PINS[i], power);
  }
  currentState = power;
}

// increments the Timer.current struct member
// if it exceeds the LED max time allowance, triggers the LED switch off
void handleTimers(){
  Timer.current = millis();
  unsigned long timerCalc = Timer.current - Timer.start;

  if (timerCalc >= Timer.PORCH_MAX) 
    digitalWrite(LED_PORCH, false);
  if (timerCalc >= Timer.HALLWAY_MAX)
    digitalWrite(LED_HALLWAY, false);
}