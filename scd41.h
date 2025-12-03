/**
 * @file scd41.h
 * @brief Driver interface for the SCD41 CO2, Temperature, and Humidity sensor.
 */

#pragma once

#include <Arduino.h>

/**
 * @struct scd41_reading_t
 * @brief Container for a single sensor measurement.
 */
typedef struct {
  float co2_ppm;       ///< CO2 concentration in PPM.
  float temperature_c; ///< Temperature in degrees Celsius.
  float humidity_rh;   ///< Relative humidity in percent.
} scd41_reading_t;

/**
 * @brief Initializes the SCD41 sensor.
 * * @details Stops any previous periodic measurement and starts a new 
 * periodic measurement cycle (approx 5s interval).
 * * @return true If the start command was acknowledged by the sensor.
 * @return false If I2C communication failed.
 */
bool SCD41_init(void);

/**
 * @brief Reads the latest measurement from the SCD41.
 * * @details Checks if data is ready before attempting to read.
 * * @param[out] out Pointer to an scd41_reading_t struct to store the data.
 * @return true If data was ready and read successfully.
 * @return false If data was not ready or I2C communication failed.
 */
bool SCD41_read(scd41_reading_t *out);