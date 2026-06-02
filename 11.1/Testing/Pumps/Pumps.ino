// ── Pin definitions ──────────────────────────────────────
const int COLD_ENB = 6,  COLD_IN3 = 5,  COLD_IN4 = 4;
const int HOT_ENA  = 9,  HOT_IN1  = 8,  HOT_IN2  = 7;

// ── Safety ───────────────────────────────────────────────
const unsigned long MAX_PUMP_RUN_MS = 300000;
unsigned long coldPumpStartTime = 0;
unsigned long hotPumpStartTime  = 0;
bool coldRunning = false;
bool hotRunning  = false;

void setup() {
  Serial.begin(9600);
  while (!Serial);  // wait for Serial Monitor to open
  Serial.println("[BOOT] System starting...");

  pinMode(COLD_ENB, OUTPUT); pinMode(COLD_IN3, OUTPUT); pinMode(COLD_IN4, OUTPUT);
  pinMode(HOT_ENA,  OUTPUT); pinMode(HOT_IN1,  OUTPUT); pinMode(HOT_IN2,  OUTPUT);
  Serial.println("[BOOT] Pins configured");

  stopAll();
  Serial.println("[BOOT] Ready");
}

// ── Pump controls ─────────────────────────────────────────
void runColdPump(int speed = 200) {
  Serial.print("[COLD] Starting — speed: "); Serial.println(speed);
  digitalWrite(COLD_IN3, HIGH);
  digitalWrite(COLD_IN4, LOW);
  analogWrite(COLD_ENB, speed);
  coldPumpStartTime = millis();
  coldRunning = true;
  Serial.print("[COLD] Running. Timeout in "); Serial.print(MAX_PUMP_RUN_MS / 1000); Serial.println("s");
}

void stopColdPump() {
  analogWrite(COLD_ENB, 0);
  digitalWrite(COLD_IN3, LOW);
  digitalWrite(COLD_IN4, LOW);
  coldRunning = false;
  unsigned long ranFor = millis() - coldPumpStartTime;
  Serial.print("[COLD] Stopped. Ran for "); Serial.print(ranFor / 1000); Serial.println("s");
}

void stopHotPump() {
  analogWrite(HOT_ENA, 0);
  digitalWrite(HOT_IN1, LOW);
  digitalWrite(HOT_IN2, LOW);
  hotRunning = false;
  unsigned long ranFor = millis() - hotPumpStartTime;
  Serial.print("[HOT] Stopped. Ran for "); Serial.print(ranFor / 1000); Serial.println("s");
}

void stopAll() {
  Serial.println("[SYS] stopAll() called");
  stopColdPump();
  stopHotPump();
}

// ── Safety watchdog ───────────────────────────────────────
void checkPumpTimeouts() {
  unsigned long now = millis();

  if (coldRunning) {
    unsigned long elapsed = now - coldPumpStartTime;
    if (elapsed > MAX_PUMP_RUN_MS) {
      Serial.print("[COLD] TIMEOUT after "); Serial.print(elapsed / 1000); Serial.println("s");
      stopColdPump();
      triggerFault("COLD_TIMEOUT");
    }
  }

  if (hotRunning) {
    unsigned long elapsed = now - hotPumpStartTime;
    if (elapsed > MAX_PUMP_RUN_MS) {
      Serial.print("[HOT] TIMEOUT after "); Serial.print(elapsed / 1000); Serial.println("s");
      stopHotPump();
      triggerFault("HOT_TIMEOUT");
    }
  }
}

void triggerFault(const char* reason) {
  Serial.print("[FAULT] "); Serial.println(reason);
  Serial.println("[FAULT] Killing all pumps");
  stopAll();
  // TODO: write to LCD, set cloud variable, sound buzzer
}

void loop() {

  checkPumpTimeouts();
  // rest of your logic...

}