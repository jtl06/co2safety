#include <Arduino.h>
#include <Wire.h>
#include "./inc/scd41.h"
#include "./inc/sd_logger.h"

// --------- Pinout (ESP32-S3) ----------
#define CO2_SDA 6
#define CO2_SCL 5

// Forward declarations from SCD41 driver
bool SCD41_init(void);
bool SCD41_read(scd41_reading_t *out);

// --------- Arduino code ----------

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println();
  Serial.println("Booting, setting up I2C, SCD41 and SD...");

  // I2C for SCD41
  Wire.begin(CO2_SDA, CO2_SCL, 100000);

  if (!SCD41_init()) {
    Serial.println("ERROR: Failed to initialize SCD41!");
  } else {
    Serial.println("SCD41 initialized, starting periodic measurements (every ~5s).");
  }

  // SD card logger init
  if (!SDLOG_init()) {
    Serial.println("ERROR: SDLOG_init failed, SD logging disabled.");
  } else {
    Serial.println("SDLOG: logging to /scd41_log.csv");
  }
}

void loop() {
  static uint32_t lastCheck = 0;
  const uint32_t interval_ms = 5000; // ask once per second

  if (millis() - lastCheck >= interval_ms) {
    lastCheck = millis();

    scd41_reading_t reading;
    if (SCD41_read(&reading)) {
      // --- Print to USB serial ---
      Serial.print("CO2: ");
      Serial.print(reading.co2_ppm, 0);
      Serial.print(" ppm, T: ");
      Serial.print(reading.temperature_c, 2);
      Serial.print(" °C, RH: ");
      Serial.print(reading.humidity_rh, 1);
      Serial.println(" %");

      // --- Log to SD card ---
      if (!SDLOG_append(millis(),
                        reading.co2_ppm,
                        reading.temperature_c,
                        reading.humidity_rh)) {
        Serial.println("SDLOG: write failed.");
      }

    } else {
      Serial.println("Waiting for SCD41 data...");
    }
  }
}
