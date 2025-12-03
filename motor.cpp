/**
 * @file motor.cpp
 * @brief Implementation of stepper motor control.
 */

#include <Arduino.h>
#include "motor.h"
#include "scd41.h"

/** @brief GPIO pins connected to the motor driver. */
static uint8_t motorPinsLocal[4] = {6, 7, 8, 3};
static uint8_t stepIndex = 0;

/** @brief Half-step switching sequence (8 steps). */
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

/** @brief CO2 threshold to activate the motor. */
static const uint32_t threshold = 1000;

static scd41_reading_t latestReading = {0};
static volatile bool haveReading = false;

extern QueueHandle_t motorQueue;

/** @brief Mutex for protecting latestReading. */
static portMUX_TYPE readingMux = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief Sets up motor pins as outputs and sets them LOW.
 */
void motor_init() {
  for (uint8_t i = 0; i < 4; ++i) {
    pinMode(motorPinsLocal[i], OUTPUT);
    digitalWrite(motorPinsLocal[i], LOW);
  }
}

/**
 * @brief Advances the motor state machine.
 */
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

/**
 * @brief Thread-safe setter for the sensor reading.
 */
void motor_set_reading(const scd41_reading_t *reading) {
  if (!reading) return;
  portENTER_CRITICAL(&readingMux);
  latestReading = *reading;
  haveReading = true;
  portEXIT_CRITICAL(&readingMux);
}

/**
 * @brief Task running at 128hz to step the motor if CO2 > threshold.
 */
void motor_task(void *pvParameters) {
  // 1000ms / 8ms = 125Hz (Satisfies "approx 128Hz" requirement)
  const TickType_t period = pdMS_TO_TICKS(8); 
  TickType_t lastWakeTime = xTaskGetTickCount();
  
  // Local storage for the latest sensor data. 
  scd41_reading_t currentReading = {0}; 

  for (;;) {
    lastWakeTime = xTaskGetTickCount();

    // Check Queue for new data 
    if (xQueueReceive(motorQueue, &currentReading, 0) == pdTRUE) {
    }

    // 2. Motor Logic
    if (currentReading.co2_ppm > 0 && currentReading.co2_ppm > threshold) {
      motor_step(true);
    } else {
      motor_step(false);
    }
    
    // 3. Wait for next cycle
    vTaskDelayUntil(&lastWakeTime, period);
  }
}