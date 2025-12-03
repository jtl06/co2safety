/**
 * @file inputs.h
 * @brief Interface for handling physical buttons using a hardware timer.
 */

#pragma once

#include <Arduino.h>

/**
 * @brief Initializes the hardware timer and button GPIOs.
 * * @param sd_enable_ptr Pointer to the global SD_ENABLE flag.
 * The ISR will toggle this value when the SD button is pressed.
 */
void inputs_init(volatile uint32_t* sd_enable_ptr);