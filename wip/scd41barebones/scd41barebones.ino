#include <Arduino.h>
#include <Wire.h>

// --------- Pinout (ESP32-S3) ----------
#define I2C_SDA 6
#define I2C_SCL 5

//-- Baud Rate
#define BAUD_RATE 115200

// --------- SCD41 Types ----------
typedef struct {
  float co2_ppm;
  float temperature_c;
  float humidity_rh;
} scd41_reading_t;

bool SCD41_init(void);
bool SCD41_read(scd41_reading_t *out);

// --------- SCD41 Constants ----------
#define SCD41_I2C_ADDR 0x62

// commands
#define CMD_START_PERIODIC_MEAS   0x21B1
#define CMD_READ_MEASUREMENT      0xEC05
#define CMD_STOP_PERIODIC_MEAS    0x3F86
#define CMD_GET_DATA_READY_STATUS 0xE4B8

// checksum
#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT       0xFF

// --------- Low-level helpers ----------
static uint8_t scd41_crc8(const uint8_t *data, uint16_t count) {
  uint8_t crc = CRC8_INIT;
  for (uint16_t i = 0; i < count; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 0x80) crc = (crc << 1) ^ CRC8_POLYNOMIAL;
      else            crc = (crc << 1);
    }
  }
  return crc;
}

static bool scd41_send_cmd(uint16_t cmd) {
  Wire.beginTransmission(SCD41_I2C_ADDR);
  Wire.write((uint8_t)(cmd >> 8));
  Wire.write((uint8_t)(cmd & 0xFF));
  return (Wire.endTransmission() == 0);
}

static bool scd41_read_words(uint16_t cmd, uint16_t *words, uint8_t n_words, uint16_t exec_time_ms) {
  if (!scd41_send_cmd(cmd)) return false;
  delay(exec_time_ms); // read-type commands need ~1 ms

  uint8_t to_read = n_words * 3;
  uint8_t got = Wire.requestFrom(SCD41_I2C_ADDR, to_read);
  if (got != to_read) return false;

  for (uint8_t i = 0; i < n_words; i++) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    uint8_t crc = Wire.read();
    uint8_t buf[2] = { msb, lsb };
    if (scd41_crc8(buf, 2) != crc) return false;
    words[i] = ((uint16_t)msb << 8) | lsb;
  }
  return true;
}

static bool scd41_data_ready(void) {
  uint16_t w;
  if (!scd41_read_words(CMD_GET_DATA_READY_STATUS, &w, 1, 1)) return false;
  // datasheet: least significant 11 bits nonzero => ready
  return (w & 0x07FF) != 0;
}

// --------- Public API ----------

bool SCD41_init(void) {
  delay(50); // power-up max 30 ms, give a little margin

  // Try to stop any previous periodic mode (ignore failure).
  scd41_send_cmd(CMD_STOP_PERIODIC_MEAS);
  delay(500); // sensor needs up to 500 ms after stop to accept new cmds

  // Start periodic measurements (5 s update interval).
  return scd41_send_cmd(CMD_START_PERIODIC_MEAS);
}

// Read one sample in periodic mode.
// If data isn't ready yet, returns false and leaves outputs unchanged.
bool SCD41_read(scd41_reading_t *out) {
  if (!out) return false;

  if (!scd41_data_ready()) {
    return false; // not ready or comms error
  }

  uint16_t w[3];
  if (!scd41_read_words(CMD_READ_MEASUREMENT, w, 3, 1)) return false;

  // Conversion per datasheet:
  out->co2_ppm       = (float)w[0];
  out->temperature_c = -45.0f + 175.0f * ((float)w[1] / 65535.0f);
  out->humidity_rh   = 100.0f * ((float)w[2] / 65535.0f);

  return true;
}

// --------- Arduino code ----------

void setup() {
  // USB serial on ESP32-S3 (shows up as a COM/tty device)
  Serial.begin(BAUD_RATE);
  while (!Serial) {
    // wait for USB CDC to be ready
    delay(10);
  }

  Serial.println();
  Serial.println("Booting, setting up I2C and SCD41...");

  // I2C on custom pins for ESP32-S3
  Wire.begin(I2C_SDA, I2C_SCL, 100000); // SDA, SCL, 100kHz

  if (!SCD41_init()) {
    Serial.println("ERROR: Failed to initialize SCD41!");
  } else {
    Serial.println("SCD41 initialized, starting periodic measurements (every ~5s).");
  }
}

void loop() {
  static uint32_t lastCheck = 0;
  const uint32_t interval_ms = 5000; // ask once per 5 second

  if (millis() - lastCheck >= interval_ms) {
    lastCheck = millis();

    scd41_reading_t reading;
    if (SCD41_read(&reading)) {
      Serial.print("CO2: ");
      Serial.print(reading.co2_ppm, 0);
      Serial.print(" ppm, T: ");
      Serial.print(reading.temperature_c, 2);
      Serial.print(" °C, RH: ");
      Serial.print(reading.humidity_rh, 1);
      Serial.println(" %");
    } else {
      // No new data yet – SCD41 only gives fresh data every ~5s
      Serial.println("Waiting for SCD41 data...");
    }
  }
}
