#include "TempSensor.h"
#define TEMP_SENSOR_PIN 2

static OneWire oneWire(TEMP_SENSOR_PIN);
static DallasTemperature sensors(&oneWire);
static float valid_temp_read = 0; 
static unsigned long bad_read_start = 0;
static const unsigned long change_state_timeout = 5000; 

TempSensorState temp_sensor_state = OK; 

void tempSensor_begin(){
    // Converts sensor read into non-blocking function. By default library will block waiting for conversion (750ms)
    sensors.setWaitForConversion(false);
    sensors.begin();
    sensors.requestTemperatures();
}

// Returns value read by temperature sensor. On first detection of a bad read (DEVICE_DISCONNECTED_C), 
// start bad read timer (bad_read_start). Gives the sensor time to correct single bad reads. Only updates 
// sensor state if bad reads exceed set timeout (change_state_timeout).
// During this period the last valid temperature reading is sent back to the main program to maintain state
float tempSensor_read() {
    float temp = sensors.getTempCByIndex(0); // reads the previous conversion
    sensors.requestTemperatures(); // kicks off next conversion

    if (temp == DEVICE_DISCONNECTED_C) {
        if (bad_read_start == 0) 
            bad_read_start = millis();
        else if ((millis() - bad_read_start) >= change_state_timeout) 
            temp_sensor_state = BAD;
    } else {
        bad_read_start = 0;
        temp_sensor_state = OK;
        valid_temp_read = temp;
    }
    return valid_temp_read;
}

// Getter
TempSensorState tempSensor_state(){
    return temp_sensor_state;
}