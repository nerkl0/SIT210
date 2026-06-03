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

// Stack sizes are in WORDS (4 bytes on SAMD21). Tune against
// uxTaskGetStackHighWaterMark during bring-up before trusting these.
#define STACK_HIGH 256
#define STACK_NORMAL 256
#define STACK_GENERAL 512 

// Task priorities
#define PRIO_HIGH 3
#define PRIO_NORMAL 1
#define PRIO_GENERAL 1

// Timers
#define PUBLISH_PERIOD_MS 2000
#define SENSOR_READ_MS 750

typedef struct {
  BathState from;
  BathState to;
  float temp;
  int progress;
} NotifMsg;

static void updateBuzzer(BathState s){
  if ((s == HARD_WARNING || s == SENSOR_FAULT) && !buzzer_silenced())
    tone(BUZZER_PIN, BEEP_FREQ, BEEP_TIME);
  else
    noTone(BUZZER_PIN);
}

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

static void applyAlerts(BathState s){
  bool red = false, yellow = false;
  switch (s){
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

// Refresh the OLED. Guards the I2C bus.
static void refreshDisplay(float temp){
  display_setTemp(temp);
  display_setMessage(setOLEDMsg());
  if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE){
    update_display();
    xSemaphoreGive(i2cMutex);
  }
}

// Apply the state-driven pump transition under the pump mutex.
static void controlPumps(BathState next, float temp){
  if (xSemaphoreTake(pumpMutex, portMAX_DELAY) == pdTRUE){
    drive_pumps(next, temp);
    xSemaphoreGive(pumpMutex);
  }
}

// High priority task
static void highPriorityTask(void *pv){
  static float temp = tempSensor_read();
  uint32_t last_temp_read = millis();

  for (;;){
    uint32_t now = millis();
    // temp sensor polls every 750ms to avoid stale reads
    if (now - last_temp_read >= SENSOR_READ_MS) {
        last_temp_read = now;
        temp = tempSensor_read();
    }

    TempSensorState t_sensorState = tempSensor_state();
    xQueueOverwrite(tempQueue, &temp);

    BathState current = get_bath_state();
    BathState next = evaluate_state(temp, now, t_sensorState == OK);

    if (next != current){
      set_bath_state(next);
      Serial.print("high prio next != current: "); Serial.println(next);
      Serial.print("Next: "); Serial.println(next);
      Serial.print("Current: "); Serial.println(current);
      Serial.println();
    }
    
    applyAlerts(next);
    updateBuzzer(next);
    controlPumps(next, temp);

    vTaskDelay(pdMS_TO_TICKS(HIGH_PERIOD_MS));
  }
}

// retain an absolute schedule so that high prio doesn't starve updates (particularly for update display)
static void normalPriorityTask(void *pv){
  static uint32_t last_publish_ms = 0;
  float temp = 0; 

  TickType_t lastWake = xTaskGetTickCount();
  for (;;){
    float t;
    xQueuePeek(tempQueue, &temp, 0);

    BathState s = get_bath_state();
    uint32_t now = millis();
    if (now - last_publish_ms >= PUBLISH_PERIOD_MS){
      last_publish_ms = now;
      publish_status(s, temp, get_fill_progress());
    }

    refreshDisplay(temp);
    taskYIELD();
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(NORMAL_PERIOD_MS));
  }
}

//  General tasks are lower on the priority list
static void generalTask(void *pv){
  for (;;){
    mqtt_loop();
    vTaskDelay(pdMS_TO_TICKS(GENERAL_PERIOD_MS));
  }
}

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

  if (pumpMutex == NULL || i2cMutex == NULL || tempQueue == NULL){
    Serial.println("RTOS primitive allocation failed - halting");
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