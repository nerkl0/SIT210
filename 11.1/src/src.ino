#include <FreeRTOS_SAMD21.h>
#include <Wire.h>
#include <Arduino.h>
#include "BathController.h"
#include "AppManager.h"
#include "Config.h"
#include "Display.h"
#include "TempSensor.h"

SemaphoreHandle_t pumpMutex;
SemaphoreHandle_t i2cMutex;
QueueHandle_t tempQueue;

#define HIGH_PERIOD_MS 250
#define NORMAL_PERIOD_MS 250
#define GENERAL_PERIOD_MS 100

// Stack sizes are in WORDS (4 bytes on SAMD21)
#define STACK_HIGH 256
#define STACK_NORMAL 256
#define STACK_GENERAL 512 

// Task priorities
#define PRIO_HIGH 3
#define PRIO_NORMAL 1
#define PRIO_GENERAL 1 // For MQTT connect

// Timers
#define PUBLISH_PERIOD_MS 2000
#define SENSOR_READ_MS 750

// Buzzer will tone on HARD_WARNING and SENSOR_FAULT if the user hasn't 
// silenced the tone from the frontend.
static void updateBuzzer(BathState s){
  if ((s == HARD_WARNING || s == SENSOR_FAULT) && !buzzer_silenced())
    tone(BUZZER_PIN, BEEP_FREQ, BEEP_TIME);
  else
    noTone(BUZZER_PIN);
}

// Configures the messaged displayed on the top line of the OLED screen dependent on state
// Base case is Bath: [State]
static const char* setOLEDMsg(){
  BathState s = get_bath_state();
  switch(s){
    case HARD_WARNING:  
      return "! WARNING !";
    case SOFT_WARNING:  
      return "Adjusting Temp";
    case SENSOR_FAULT: 
      return "SENSOR FAULT";
    default:
      static char str[32];
      snprintf(str, sizeof(str), "Bath: %s", stringify_bathState(s));
      return str;
  }
}

// Sets the LEDs ON/OFF dependent on state. 
// Yellow for LOST_CONNECTION and SOFT_WARNING. Red for HARD_WARNING and SENSOR_FAULT
// Any other state will keep the LEDs switched OFF
static void applyAlerts(BathState st){
  bool red = false, yellow = false;
  switch (st){
    case HARD_WARNING:
    case SENSOR_FAULT:
      red = true;
      break;
    case SOFT_WARNING:
    case LOST_CONNECTION:
      yellow = true;
      break;
    default: break;
  }

  digitalWrite(LED_RED, red ? HIGH : LOW);
  digitalWrite(LED_YELLOW, yellow ? HIGH : LOW);
}

/*
  Refresh the OLED. Builds and sets the message.
  I2C mutex waits for portMAX_DELAY, then runs the block only if the pdTRUE lock was acquired
  temp argument is the current temperature. Can be a stale read if the sensor returns a bad reading 
*/
static void refreshDisplay(float temp){
  display_setTemp(temp);
  display_setMessage(setOLEDMsg());
  if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE){
    update_display();
    xSemaphoreGive(i2cMutex);
  }
}

// Apply pump transition guarding them with under the pump mutex.
// st / temp is current state of st / temp
static void controlPumps(BathState st, float temp){
  if (xSemaphoreTake(pumpMutex, portMAX_DELAY) == pdTRUE){
    drive_pumps(st, temp);
    xSemaphoreGive(pumpMutex);
  }
}

// High priority task: reads the sensor, evaluates the bath state calling evaluate_state()
// Updates the alerts/buzzer/pumps. Runs every HIGH_PERIOD_MS.
static void highPriorityTask(void *pv){
  static float temp = tempSensor_read();
  uint32_t last_temp_read = millis();

  for (;;){
    uint32_t now = millis();
    // DS18B20 temp sensor polls every 750ms. This block avoids unecessarily polling for stale reads
    if (now - last_temp_read >= SENSOR_READ_MS) {
        last_temp_read = now;
        temp = tempSensor_read();
    }

    TempSensorState t_sensorState = tempSensor_state();
    xQueueOverwrite(tempQueue, &temp);  // add to xqueue latest temp for the normal-priority task

    // Run the state machine and apply to state
    BathState state = evaluate_state(temp, now, t_sensorState == OK);
    set_bath_state(state);
    
    // Action system based on the set state
    applyAlerts(state);
    updateBuzzer(state);
    controlPumps(state, temp);

    vTaskDelay(pdMS_TO_TICKS(HIGH_PERIOD_MS));
  }
}

/*
  Normal priority task: peeks the latest temp, publishes status over MQTT on a fixed interval,
  Refreshes the OLED every NORMAL_PERIOD_MS. 
  Uses vTaskDelayUntil for an absolute schedule so the high-prio task doesn't starve the display/publish tasks
*/
static void normalPriorityTask(void *pv){
  static uint32_t last_publish_ms = 0;
  float temp = 0; 

  TickType_t lastWake = xTaskGetTickCount();
  for (;;){
    // non-blocking read of latest temp (timeout 0). Leaves temp unchanged if queue empty
    xQueuePeek(tempQueue, &temp, 0);

    BathState st = get_bath_state();

    // Control MQTT publishes to PUBLISH_PERIOD_MS so it's not spamming the broker
    uint32_t now = millis();
    if (now - last_publish_ms >= PUBLISH_PERIOD_MS){
      last_publish_ms = now;
      publish_status(st, temp, get_fill_progress());
    }

    refreshDisplay(temp);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(NORMAL_PERIOD_MS)); // absolute schedule
  }
}

// General/background task: services the MQTT client (keepalives, incoming
// messages, reconnects) every GENERAL_PERIOD_MS.
static void generalTask(void *pv){
  for (;;){
    mqtt_loop();
    vTaskDelay(pdMS_TO_TICKS(GENERAL_PERIOD_MS));
  }
}

// Pin setup
static void pin_begin(){
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
}

void setup(){
  Serial.begin(9600);
  bathController_begin();
  tempSensor_begin();
  display_begin();
  pin_begin();
  mqtt_begin();

  //RTOS primitives 
  pumpMutex = xSemaphoreCreateMutex();
  i2cMutex = xSemaphoreCreateMutex();
  tempQueue = xQueueCreate(1, sizeof(float));

  // If any mutex isn't allocated sufficient space, system halts
  if (pumpMutex == NULL || i2cMutex == NULL || tempQueue == NULL){
    Serial.println("RTOS primitive allocation failed, halting");
    while (1) {}
  }
  // Task initialiser
  xTaskCreate(highPriorityTask, "high", STACK_HIGH, NULL, PRIO_HIGH, NULL);
  xTaskCreate(normalPriorityTask, "nrml", STACK_NORMAL, NULL, PRIO_NORMAL, NULL);
  xTaskCreate(generalTask, "gnrl", STACK_GENERAL, NULL, PRIO_GENERAL, NULL);

  vTaskStartScheduler(); // if successful, will not return from here

  // Reached only if the scheduler doesn't start (out of heap)
  Serial.println("Scheduler failed to start");
  while (1) {}
}

void loop(){
}