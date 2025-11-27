#include <Arduino.h>
#include <Wire.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "scd41.h"
#include "screen.h"
#include "motor.h"


//macros
#define DISPLAY_BUTTON 10

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

static const uint8_t motorPins[4] = {MOTOR_PIN1, MOTOR_PIN2, MOTOR_PIN3, MOTOR_PIN4};

bool motorPower = true;
static bool lastButtonState = HIGH;
static unsigned long lastChangeTime = 0;
const unsigned long debounceMs = 30;

// Sensor task: runs every 10 ms
void sensorTask(void *pvParameters) {
  const TickType_t period = pdMS_TO_TICKS(10);  // 10 ms
  const TickType_t readInterval = pdMS_TO_TICKS(1000); // try read every second
  TickType_t lastWakeTime = xTaskGetTickCount();
  TickType_t lastReadTime = 0;

  for (;;) {
    bool rawState = digitalRead(DISPLAY_BUTTON);

    unsigned long now = millis();

    if (rawState != lastButtonState && (now - lastChangeTime) > debounceMs) {
      lastChangeTime = now;
      lastButtonState = rawState;

      if (rawState == LOW) {
        screen_next_page();
      }
    }

    TickType_t nowTicks = xTaskGetTickCount();
    if ((nowTicks - lastReadTime) >= readInterval) {
      scd41_reading_t reading;
      if (SCD41_read(&reading)) {
        screen_set_reading(&reading);
      }
      lastReadTime = nowTicks;
    }

    vTaskDelayUntil(&lastWakeTime, period);
  }
}

// Motor task: runs every 5 ms
void motorTask(void *pvParameters) {
  const TickType_t period = pdMS_TO_TICKS(5);   // 5 ms
  TickType_t lastWakeTime = xTaskGetTickCount();

  for (;;) {
    motor_step(motorPower); // call every 5 ms
    vTaskDelayUntil(&lastWakeTime, period);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(DISPLAY_BUTTON, INPUT_PULLUP);
  lastButtonState = digitalRead(DISPLAY_BUTTON);

  Wire.begin(4, 5);
  if (!SCD41_init()) {
    Serial.println("Failed to start SCD41 periodic measurements");
  }

  screen_init();
  motor_init(motorPins);

  // Create sensor task (10 ms)
  xTaskCreatePinnedToCore(
    sensorTask,
    "SensorTask",
    4096,
    NULL,
    1,          // priority
    NULL,
    0           // core 0
  );

  // Create sensor task (10 ms)
  xTaskCreatePinnedToCore(
    screen_task,
    "displayTask",
    4096,
    NULL,
    1,          // priority
    NULL,
    0           // core 0
  );

  // Create motor task (5 ms)
  xTaskCreatePinnedToCore(
    motorTask,
    "MotorTask",
    4096,
    NULL,
    2,          // higher priority
    NULL,
    1           // core 1
  );
}

void loop() {}
