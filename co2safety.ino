#include <Arduino.h>
#include <Wire.h>
#include "scd41.h"
#include "sd_logger.h"
#include "screen.h"
#include "motor.h"

// --------- Pinout (ESP32-S3) ----------
#define I2C_SDA 4
#define I2C_SCL 5

#define NEXT_BUTTON_PIN 35
#define SD_BUTTON_PIN 37

// Forward declarations from SCD41 driver
bool SCD41_init(void);
bool SCD41_read(scd41_reading_t *out);

bool SD_ENABLE = true;

// --------- Arduino code ----------

void setup() {
  Serial.begin(115200);
  screen_init(&SD_ENABLE);
  motor_init();

  pinMode(NEXT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SD_BUTTON_PIN, INPUT_PULLUP);
  while (!Serial) {
    delay(10);
  }

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

  xTaskCreate(sensor_task, "SensorTask", 4096, NULL, 1, NULL);
  xTaskCreate(screen_task, "ScreenTask", 4096, NULL, 1, NULL);
  xTaskCreate(next_button_task, "NextButtonTask", 4096, NULL, 1, NULL);
  xTaskCreate(sd_button_task, "SDButtonTask", 4096, NULL, 1, NULL);
  xTaskCreate(motor_task, "MotorTask", 4096, NULL, 1, NULL);
}

void loop() {}

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

      screen_set_reading(&reading);
      motor_set_reading(&reading);
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

void next_button_task(void *pvParameters) {
  const TickType_t debounce_delay = pdMS_TO_TICKS(10);
  const TickType_t check_interval = pdMS_TO_TICKS(8); //~128hz
  TickType_t last_wake_time = xTaskGetTickCount();
  bool last_button_state = LOW;  // Assuming pull-up resistor

  while (true) {
    last_wake_time = xTaskGetTickCount();
    bool current_button_state = digitalRead(NEXT_BUTTON_PIN);

    if (current_button_state != last_button_state) {
      vTaskDelay(debounce_delay);
      current_button_state = digitalRead(NEXT_BUTTON_PIN);
      if (current_button_state != last_button_state) {
        last_button_state = current_button_state;
        if (current_button_state == HIGH) {  // Button pressed
          screen_next_page();
        }
      }
    }

    vTaskDelayUntil(&last_wake_time, check_interval);
  }
}

void sd_button_task(void *pvParameters) {
  const TickType_t debounce_delay = pdMS_TO_TICKS(50);
  const TickType_t check_interval = pdMS_TO_TICKS(10);
  TickType_t last_wake_time = xTaskGetTickCount();
  bool last_button_state = HIGH;  // Assuming pull-up resistor

  while (true) {
    last_wake_time = xTaskGetTickCount();
    bool current_button_state = digitalRead(SD_BUTTON_PIN);

    if (current_button_state != last_button_state) {
      vTaskDelay(debounce_delay);
      current_button_state = digitalRead(SD_BUTTON_PIN);
      if (current_button_state != last_button_state) {
        last_button_state = current_button_state;
        if (current_button_state == LOW) {  // Button pressed
          SD_ENABLE = !SD_ENABLE;
        }
      }
    }

    vTaskDelayUntil(&last_wake_time, check_interval);
  }
}
