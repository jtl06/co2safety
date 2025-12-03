/**
 * @file sd_logger.h
 * @brief Interface for logging sensor data to an SD card.
 */

#pragma once

#include <Arduino.h>

/**
 * @brief Initializes the SPI bus and the SD card.
 * * @details This function initializes SPI with specific pins, attempts to mount
 * the SD card, and prepares the log file. If the file is empty, it writes
 * the CSV header.
 * * @return true If the SD card was initialized and the file is ready.
 * @return false If the SD card mount failed or the file could not be opened.
 */
bool SDLOG_init(void);

/**
 * @brief Appends a single sensor reading to the log file as a CSV row.
 * * @param timestamp_ms The system timestamp in milliseconds.
 * @param co2_ppm The CO2 concentration in Parts Per Million.
 * @param temperature_c The temperature in Celsius.
 * @param humidity_rh The relative humidity percentage.
 * @return true If the write was successful.
 * @return false If the file could not be opened for writing.
 */
bool SDLOG_append(uint32_t timestamp_ms,
                  float co2_ppm,
                  float temperature_c,
                  float humidity_rh);