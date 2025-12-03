/**
 * @file screen.h
 * @brief Interface for LCD screen management using I2C.
 */

#pragma once

#include <Arduino.h>
#include "scd41.h"

/**
 * @brief Initializes the LCD screen and I2C wire.
 * * @param sd_enable Pointer to the global SD enable variable to display status.
 */
void screen_init(volatile bool* sd_enable);

/** @brief Displays CO2 reading on the screen. */
void screen_show_co2(float reading);

/** @brief Displays humidity reading on the screen. */
void screen_show_humidity(float reading);

/** @brief Displays temperature reading on the screen. */
void screen_show_temperature(float reading);

/** @brief Displays a waiting message on the screen. */
void screen_show_waiting(void);

/** @brief Cycles to the next display page. */
void screen_next_page(void);

/**
 * @brief FreeRTOS task to manage screen updates.
 * @param pvParameters Task parameters (unused).
 */
void screen_task(void *pvParameters);