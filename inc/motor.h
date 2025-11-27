#pragma once

#include <Arduino.h>

#define MOTOR_PIN1      4
#define MOTOR_PIN2      5
#define MOTOR_PIN3      6
#define MOTOR_PIN4      7

void motor_init(const uint8_t pins[4]);
void motor_step(bool power);
