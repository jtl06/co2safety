#pragma once

#include <Arduino.h>
#include "scd41.h"

void motor_init();
void motor_step(bool power);
void motor_set_reading(const scd41_reading_t *reading);
void motor_task(void *pvParameters);
