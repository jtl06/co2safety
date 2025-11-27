// sd_logger.h
#pragma once

#include <Arduino.h>

// Initialize SPI + SD card and prepare the log file.
// Returns true on success, false on failure.
bool SDLOG_init(void);

// Append one reading as a CSV row.
// Returns true on success, false on failure.
bool SDLOG_append(uint32_t timestamp_ms,
                  float co2_ppm,
                  float temperature_c,
                  float humidity_rh);
