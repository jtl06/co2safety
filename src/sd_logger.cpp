// sd_logger.cpp
#include "../inc/sd_logger.h"

#include <SPI.h>
#include <SD.h>

// --- SD card pins for ESP32-S3 (adjust if needed) ---
#define SD_CS   10
#define SD_MOSI 11
#define SD_MISO 13
#define SD_SCK  12

// Log file path on the SD card
static const char *SDLOG_PATH = "/scd41_log.csv";

bool SDLOG_init(void) {
  // Init SPI bus with custom pins
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS)) {
    Serial.println("SDLOG: SD card mount failed!");
    return false;
  }

  Serial.println("SDLOG: SD card initialized.");

  // Create/open the log file and add header if empty
  File f = SD.open(SDLOG_PATH, FILE_APPEND);
  if (!f) {
    Serial.println("SDLOG: Could not open log file for header.");
    return false;
  }

  if (f.size() == 0) {
    f.println("millis,co2_ppm,temperature_c,humidity_rh");
  }

  f.close();
  return true;
}

bool SDLOG_append(uint32_t timestamp_ms,
                  float co2_ppm,
                  float temperature_c,
                  float humidity_rh) {
  File f = SD.open(SDLOG_PATH, FILE_APPEND);
  if (!f) {
    Serial.println("SDLOG: Could not open log file for writing.");
    return false;
  }

  f.print(timestamp_ms);
  f.print(",");
  f.print(co2_ppm, 1);
  f.print(",");
  f.print(temperature_c, 2);
  f.print(",");
  f.println(humidity_rh, 1);

  f.close();
  return true;
}
