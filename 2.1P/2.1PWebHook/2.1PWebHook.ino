#include <DHT.h>

#define DHTPIN 4;
#define DHTTYPE DHT11;

float currentTemp;

void setup()
{
  Serial.begin(9600);
  pinMode(TEMP_SENSOR, INPUT);
}

void loop()
{
  currentTemp = digitalRead(TEMP_SENSOR);
  Serial.println(currentTemp);
  delay(500);
}