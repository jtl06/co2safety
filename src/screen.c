#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "screen.h"

static LiquidCrystal_I2C lcd(0x27, 16, 2);

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
