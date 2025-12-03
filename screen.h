#pragma once

#include <Arduino.h>
#include "scd41.h"

void screen_init(bool* sd_enable);
void screen_show_co2(float reading);
void screen_show_humidity(float reading);
void screen_show_temperature(float reading);
void screen_show_waiting(void);
void screen_next_page(void);
void screen_set_reading(const scd41_reading_t *reading);
void screen_task(void *pvParameters);
