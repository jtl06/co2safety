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
 * @brief Updates the motor logic with the latest sensor reading.
 * * @details Thread-safe update of the reading used by the motor task.
 * @param reading Pointer to the latest scd41_reading_t.
 */
void motor_set_reading(const scd41_reading_t *reading);

/**
 * @brief FreeRTOS task that controls the motor.
 * * @param pvParameters Task parameters (unused).
 */
void motor_task(void *pvParameters);