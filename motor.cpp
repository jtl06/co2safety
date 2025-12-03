#include <Arduino.h>
#include "motor.h"
#include "scd41.h"

static uint8_t motorPinsLocal[4] = {6, 7, 8, 3};
static uint8_t stepIndex = 0;

static const uint8_t motorSeq[8][4] = {
  {1,0,0,0},
  {1,1,0,0},
  {0,1,0,0},
  {0,1,1,0},
  {0,0,1,0},
  {0,0,1,1},
  {0,0,0,1},
  {1,0,0,1}
};

static const uint32_t threshold = 1000;

static scd41_reading_t latestReading = {0};
static bool haveReading = false;

static portMUX_TYPE readingMux = portMUX_INITIALIZER_UNLOCKED;

void motor_init() {
  for (uint8_t i = 0; i < 4; ++i) {
    pinMode(motorPinsLocal[i], OUTPUT);
    digitalWrite(motorPinsLocal[i], LOW);
  }
}

// One step per call when power == true. Caller controls timing.
void motor_step(bool power) {
  if (power) {
    stepIndex = (stepIndex + 1) % 8;
    for (uint8_t i = 0; i < 4; ++i) {
      digitalWrite(motorPinsLocal[i], motorSeq[stepIndex][i] ? HIGH : LOW);
    }
  } else {
    for (uint8_t i = 0; i < 4; ++i) {
      digitalWrite(motorPinsLocal[i], LOW);
    }
  }
}

void motor_set_reading(const scd41_reading_t *reading) {
  if (!reading) return;
  portENTER_CRITICAL(&readingMux);
  latestReading = *reading;
  haveReading = true;
  portEXIT_CRITICAL(&readingMux);
}

void motor_task(void *pvParameters) {
  const TickType_t period = pdMS_TO_TICKS(1); //1000hz
  TickType_t lastWakeTime = xTaskGetTickCount();

  for (;;) {
    lastWakeTime = xTaskGetTickCount();
    scd41_reading_t readingCopy;
    bool ready;
    portENTER_CRITICAL(&readingMux);
    readingCopy = latestReading;
    ready = haveReading;
    portEXIT_CRITICAL(&readingMux);

    if (ready) {
      // Simple control: power motor if CO2 > threshold
      if (readingCopy.co2_ppm > threshold) {
        motor_step(true);
      } else {
        motor_step(false);
      }
    } else {
      motor_step(false);
    }
    
    vTaskDelayUntil(&lastWakeTime, period);
  }
}
