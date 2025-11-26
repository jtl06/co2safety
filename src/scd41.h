#pragma once

#include <Arduino.h>

typedef struct {
  float co2_ppm;
  float temperature_c;
  float humidity_rh;
} scd41_reading_t;

bool SCD41_init(void);
bool SCD41_read(scd41_reading_t *out);
