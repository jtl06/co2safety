#pragma once

#include <Arduino.h>

void screen_init(void);
void screen_show_co2(float reading);
void screen_show_humidity(float reading);
void screen_show_temperature(float reading);
void screen_show_waiting(void);
