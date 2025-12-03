/**
 * @file co2safety.ino
 * @brief Main firmware entry point for the CO2 Safety Monitor.
 * * @details This file initializes the system and spawns FreeRTOS tasks for 
 * reading sensors, updating the screen, handling buttons, and controlling the motor.
 */

#include <Arduino.h>
#include <Wire.h>
#include "scd41.h"
#include "sd_logger.h"
#include "inputs.h"
#include "screen.h"
#include "motor.h"
#include <stdint.h>

// --------- Pinout (ESP32-S3) ----------
#define I2C_SDA 4
#define I2C_SCL 5

// --------- Other Macros ----------
#define BAUD_RATE 115200

// Forward declarations from SCD41 driver
bool SCD41_init(void);
bool SCD41_read(scd41_reading_t *out);

/** @brief Global flag to enable/disable SD card logging. */
volatile bool SD_ENABLE = 1;

// Define queue handles
QueueHandle_t screenQueue;
QueueHandle_t motorQueue;

// --------- Arduino code ----------

/**
 * @brief Arduino setup function.
 * * @details Initializes Serial, Screen, Motor, Buttons, I2C, SCD41, and SD Card.
 * Then creates the FreeRTOS tasks.
 */
void setup() {
  Serial.begin(BAUD_RATE);
  screen_init(&SD_ENABLE);
  motor_init();
  inputs_init(&SD_ENABLE);
  
  while (!Serial) {
    delay(10);
  }

  // Queue init
  // Queue holds 1 item of type 'scd41_reading_t'
  screenQueue = xQueueCreate(1, sizeof(scd41_reading_t));
  motorQueue  = xQueueCreate(1, sizeof(scd41_reading_t));

  Serial.println();
  Serial.println("Booting, setting up I2C, SCD41 and SD...");

  // I2C init
  Wire.begin(I2C_SDA, I2C_SCL, 100000);
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
  //
  xTaskCreatePinnedToCore(sensor_task, "SensorTask", 4096, NULL, 2, NULL, 0);
  // UI + buttons + motor on core 1
  xTaskCreatePinnedToCore(screen_task,      "ScreenTask",      4096, NULL, 1, NULL, 1);  
  xTaskCreatePinnedToCore(motor_task,       "MotorTask",       4096, NULL, 1, NULL, 1);
}

/**
 * @brief Arduino main loop.
 * @details Not used because FreeRTOS tasks are handling execution.
 */
void loop() {}

/**
 * @brief Task to read the SCD41 sensor periodically.
 * * @param pvParameters Unused.
 */
void sensor_task(void *pvParameters) {
  scd41_reading_t reading;
  TickType_t last_wake_time = xTaskGetTickCount();
  while (true) {
    last_wake_time = xTaskGetTickCount();
    if (SCD41_read(&reading)) {
      // --- Print to USB serial ---
      // Serial.print("CO2: ");
      // Serial.print(reading.co2_ppm, 0);
      // Serial.print(" ppm, T: ");
      // Serial.print(reading.temperature_c, 2);
      // Serial.print(" °C, RH: ");
      // Serial.print(reading.humidity_rh, 1);
      // Serial.println(" %");

      xQueueOverwrite(screenQueue, &reading);
      xQueueOverwrite(motorQueue, &reading);
      // --- Log to SD card ---
      if(SD_ENABLE){
        if (!SDLOG_append(millis(),
                          reading.co2_ppm,
                          reading.temperature_c,
                          reading.humidity_rh)) {
          Serial.println("SDLOG: write failed.");
        }
      }
    } else {
      Serial.println("Waiting for SCD41 data...");
    }

    vTaskDelayUntil(&last_wake_time, 5000 / portTICK_PERIOD_MS);
  }
}