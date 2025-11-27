#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "screen.h"

static LiquidCrystal_I2C lcd(0x27, 16, 2);

static int page = 0;
static scd41_reading_t latestReading = {0};
static bool haveReading = false;
static portMUX_TYPE readingMux = portMUX_INITIALIZER_UNLOCKED;

void screen_init(void) {
  lcd.init();
  lcd.backlight();
  delay(2);
  lcd.clear();
}

void screen_show_co2(float reading) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CO2 PPM: ");
  lcd.setCursor(10, 0);
  lcd.print(reading, 0);
}

void screen_show_humidity(float reading) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Humidity: ");
  lcd.setCursor(11, 0);
  lcd.print(reading, 0);
}

void screen_show_temperature(float reading) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.setCursor(7, 0);
  lcd.print(reading, 0);
}

void screen_show_waiting(void) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for");
  lcd.setCursor(0, 1);
  lcd.print("sensor...");
}

void screen_next_page(void) {
  page = (page + 1) % 3;
}

void screen_set_reading(const scd41_reading_t *reading) {
  if (!reading) return;
  portENTER_CRITICAL(&readingMux);
  latestReading = *reading;
  haveReading = true;
  portEXIT_CRITICAL(&readingMux);
}

void screen_task(void *pvParameters) {
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
