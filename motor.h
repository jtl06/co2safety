/**
 * @file motor.h
 * @brief Interface for stepper motor control based on sensor readings.
 */

#pragma once

#include <Arduino.h>
#include "scd41.h"

/**
 * @brief Initializes the motor GPIO pins.
 */
void motor_init();

/**
 * @brief Performs a single step of the motor or turns it off.
 * * @param power If true, advances the step sequence. If false, turns all pins LOW.
 */
void motor_step(bool power);

/**
 * @brief FreeRTOS task that controls the motor.
 * * @param pvParameters Task parameters (unused).
 */
void motor_task(void *pvParameters);