#include <Arduino.h>
#include <Wire.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
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

int page = 0;
static scd41_reading_t latestReading = {0};
static bool haveReading = false;
static portMUX_TYPE readingMux = portMUX_INITIALIZER_UNLOCKED;

void changeDisplay(){
  page = (page + 1) % 3;
}


void writeDisplay(void *pvParameters) {
  const TickType_t period = pdMS_TO_TICKS(1000);  // 1 s
  TickType_t lastWakeTime = xTaskGetTickCount();

  for (;;) {
    scd41_reading_t readingCopy;
    bool ready;
    portENTER_CRITICAL(&readingMux);
    readingCopy = latestReading;
    ready = haveReading;
    portEXIT_CRITICAL(&readingMux);

    if (!ready) {
      screen_show_waiting();
    } else if (page == 0) {
      screen_show_co2(readingCopy.co2_ppm);
    } else if (page == 1) {
      screen_show_temperature(readingCopy.temperature_c);
    } else {
      screen_show_humidity(readingCopy.humidity_rh);
    }
    vTaskDelayUntil(&lastWakeTime, period);
  }
}

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
        changeDisplay();
      }
    }

    TickType_t nowTicks = xTaskGetTickCount();
    if ((nowTicks - lastReadTime) >= readInterval) {
      scd41_reading_t reading;
      if (SCD41_read(&reading)) {
        portENTER_CRITICAL(&readingMux);
        latestReading = reading;
        haveReading = true;
        portEXIT_CRITICAL(&readingMux);
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
    writeDisplay,
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
