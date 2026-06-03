#include "TempSensor.h"
#define TEMP_SENSOR_PIN 2

static OneWire oneWire(TEMP_SENSOR_PIN);
static DallasTemperature sensors(&oneWire);
static float valid_temp_read = 0; 
static unsigned long bad_read_start_ms = 0;
static const unsigned long change_state_timeout = 5000; 

TempSensorState temp_sensor_state = OK; 

void tempSensor_begin(){
    sensors.setWaitForConversion(false);
    sensors.begin();
    sensors.requestTemperatures();
}

float tempSensor_read() {
    float temp = sensors.getTempCByIndex(0);
    sensors.requestTemperatures();

    if (temp == DEVICE_DISCONNECTED_C) {
        if (bad_read_start_ms == 0) 
            bad_read_start_ms = millis();
        else if ((millis() - bad_read_start_ms) >= change_state_timeout) 
            temp_sensor_state = BAD;
        Serial.println("TEMP: Bad read");
    } else {
        bad_read_start_ms = 0;
        temp_sensor_state = OK;
        valid_temp_read = temp;
    }
    return valid_temp_read;
}

TempSensorState tempSensor_state(){
    return temp_sensor_state;
}