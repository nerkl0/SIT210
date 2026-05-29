#include "PumpController.h"   

const int HOT_ENA = 9, HOT_IN1 = 8, HOT_IN2 = 7;
const int COLD_ENB = 6, COLD_IN3 = 5, COLD_IN4 = 4;

int speed = 225;

void pumpController_begin() {
    pinMode(HOT_ENA, OUTPUT);
    pinMode(HOT_IN1, OUTPUT);
    pinMode(HOT_IN2, OUTPUT);
    pinMode(COLD_ENB, OUTPUT);
    pinMode(COLD_IN3, OUTPUT);
    pinMode(COLD_IN4, OUTPUT);
}

void run_pump(int pump) {
    if (pump == HOT) {
        analogWrite(HOT_ENA, speed); 
        digitalWrite(HOT_IN1, HIGH);
        digitalWrite(HOT_IN2, LOW);
    } else if (pump == COLD) {
        analogWrite(COLD_ENB, speed); 
        digitalWrite(COLD_IN3, HIGH);
        digitalWrite(COLD_IN4, LOW);
    }
}

void stop_pump(int pump) {
    if (pump == HOT) {
        analogWrite(HOT_ENA, LOW);
        digitalWrite(HOT_IN1, LOW);
        digitalWrite(HOT_IN2, LOW);
    } else if (pump == COLD) {
        analogWrite(COLD_ENB, LOW);
        digitalWrite(COLD_IN3, LOW);
        digitalWrite(COLD_IN4, LOW);
    }
}

void stop_both_pumps(){
    stop_pump(HOT);
    stop_pump(COLD);
}

void run_both_pumps(){
    run_pump(HOT);
    run_pump(COLD);
}