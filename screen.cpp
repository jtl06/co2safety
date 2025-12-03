/**
 * @file screen.cpp
 * @brief Implementation of the LCD screen display logic.
 */

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "screen.h"
#include <Wire.h>
#include <stdint.h>

/** @brief LCD object initialization (Addr: 0x27, 16 chars, 2 lines). */
LiquidCrystal_I2C lcd(0x27, 16, 2);

static int page = 0;
static scd41_reading_t latestReading = {0};
static bool haveReading = false;
static portMUX_TYPE readingMux = portMUX_INITIALIZER_UNLOCKED;

/** @brief Pointer to external SD enable flag. */
volatile bool* SD_var;

/**
 * @brief Initialize screen and backlight.
 */
void screen_init(volatile bool* sd_enable) {
 
  SD_var = sd_enable;
  Wire.begin(4,5); // SDA, SCL
  lcd.init();
  lcd.backlight();
  delay(2);
  lcd.clear();
}

void screen_show_co2(float reading) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CO2 PPM: ");
  lcd.setCursor(9, 0);
  lcd.print(reading, 0);
}

void screen_show_humidity(float reading) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Humidity: ");
  lcd.setCursor(10, 0);
  lcd.print(reading, 0);
}

void screen_show_temperature(float reading) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.setCursor(6, 0);
  lcd.print(reading, 0);
}

void screen_show_waiting(void) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for");
  lcd.setCursor(0, 1);
  lcd.print("sensor...");
  Serial.println("screen test");
}

void screen_next_page(void) {
  page = (page + 1) % 3;
}

/**
 * @brief Prints the SD logging status on the second line.
 */
void screen_sd_line(void) {
  lcd.setCursor(0, 1);
  lcd.print("SD Log: ");
  lcd.setCursor(8, 1);
  if(*SD_var){
    lcd.print("Enabled");
  } else {
    lcd.print("Disabled");
  }
}

/**
 * @brief Thread-safe update of the reading data.
 */
void screen_set_reading(const scd41_reading_t *reading) {
  if (!reading) return;
  portENTER_CRITICAL(&readingMux);
  latestReading = *reading;
  haveReading = true;
  portEXIT_CRITICAL(&readingMux);
}

/**
 * @brief Task for refreshing the screen at ~2Hz.
 */
void screen_task(void *pvParameters) {
  const TickType_t period = pdMS_TO_TICKS(500);  // 2hz
  TickType_t lastWakeTime = xTaskGetTickCount();

  for (;;) {
    lastWakeTime = xTaskGetTickCount();
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
      screen_sd_line();
    } else if (page == 1) {
      screen_show_temperature(readingCopy.temperature_c);
      screen_sd_line();
    } else {
      screen_show_humidity(readingCopy.humidity_rh);
      screen_sd_line();
    }
    vTaskDelayUntil(&lastWakeTime, period);
  }
}