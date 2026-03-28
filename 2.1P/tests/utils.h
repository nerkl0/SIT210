#ifndef UTILS_H
#define UTILS_H

int LED_Status = LOW;

void toggleLED(int LED_PIN){
  LED_Status = !LED_Status;
  digitalWrite(LED_PIN, LED_Status);
}

template <typename T>
void formatPrint(T val, bool success){
  Serial.print(val);
  Serial.print(": ");
  Serial.print(success ? "SUCCESS" : "FAILED");

  Serial.println();
  Serial.println();
  Serial.println("---------------------------------");
  Serial.println();
}


#endif