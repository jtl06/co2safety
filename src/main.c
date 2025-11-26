#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "scd41.h"

#define DISPLAY_BUTTON 10

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

LiquidCrystal_I2C lcd(0x27, 16, 2);
static const uint8_t motorPins[4] = {6, 7, 8, 9};

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

static uint8_t stepIndex = 0;

bool motorPower = true;
static bool lastButtonState = HIGH;
static unsigned long lastChangeTime = 0;
const unsigned long debounceMs = 30;

int page = 0;
static scd41_reading_t latestReading = {0};
static bool haveReading = false;
static portMUX_TYPE readingMux = portMUX_INITIALIZER_UNLOCKED;

void writeCO2(float reading) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CO2 PPM: ");
  lcd.setCursor(10, 0);
  lcd.print(reading, 0);
}

void writeHumidity(float reading) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Humidity: ");
  lcd.setCursor(11, 0);
  lcd.print(reading, 0);
}

void writeTemperature(float reading) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.setCursor(7, 0);
  lcd.print(reading, 0);
}

void changeDisplay(){
  page = (page + 1) % 3;
}

// Now: one *step* per call when power == true.
// The 5 ms timing is handled by the FreeRTOS task.
void motorControl(bool power) {
  if (power) {
    stepIndex = (stepIndex + 1) % 8;
    for (uint8_t i = 0; i < 4; ++i) {
      digitalWrite(motorPins[i], motorSeq[stepIndex][i] ? HIGH : LOW);
    }
  } else {
    // turn all coils off to disable motor
    for (uint8_t i = 0; i < 4; ++i) {
      digitalWrite(motorPins[i], LOW);
    }
  }
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
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Waiting for");
      lcd.setCursor(0, 1);
      lcd.print("sensor...");
    } else if (page == 0) {
      writeCO2(readingCopy.co2_ppm);
    } else if (page == 1) {
      writeTemperature(readingCopy.temperature_c);
    } else {
      writeHumidity(readingCopy.humidity_rh);
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
    motorControl(motorPower); // call every 5 ms
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

  lcd.init();
  lcd.backlight();
  delay(2);
  lcd.clear();

  for (uint8_t i = 0; i < 4; ++i) {
    pinMode(motorPins[i], OUTPUT);
    digitalWrite(motorPins[i], LOW);
  }

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
