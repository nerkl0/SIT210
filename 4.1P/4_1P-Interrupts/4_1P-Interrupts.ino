const uint8_t SWITCH_PIN = 2;
const uint8_t SENSOR_PIN = 3;
const uint8_t LEDB_PIN = 7;
const uint8_t LEDR_PIN = 8;

uint8_t ledState = LOW;
uint8_t sensorState = LOW;
uint8_t sensorValue = 0;

void setup() {
  Serial.begin(9600);
  while(!Serial);
  
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(LEDR_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);

  //attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), toggle, FALLING);
}

void loop() {
  sensorValue = digitalRead(SENSOR_PIN);
  if (sensorValue == HIGH) {
    digitalWrite(LEDR_PIN, HIGH);
    digitalWrite(LEDB_PIN, HIGH);
    delay(100);
    
    if (sensorState == LOW) {
      Serial.println("Motion detected!");
      sensorState = HIGH;
    }
  } 
  else {
      digitalWrite(LEDR_PIN, LOW);
      digitalWrite(LEDB_PIN, HIGH);
      delay(200);
      
      if (sensorState == HIGH){
        Serial.println("Motion stopped!");
        sensorState = LOW;
    }
  }
}
