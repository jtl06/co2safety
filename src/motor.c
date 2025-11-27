#include <Arduino.h>
#include "motor.h"

static uint8_t motorPinsLocal[4] = {0};
static uint8_t stepIndex = 0;

static const uint8_t motorSeq[8][4] = {
  {1,0,0,0},
  {1,1,0,0},
  {0,1,0,0},
  {0,1,1,0},
  {0,0,1,0},
  {0,0,1,1},
  {0,0,0,1},
  {1,0,0,1}
};

void motor_init(const uint8_t pins[4]) {
  for (uint8_t i = 0; i < 4; ++i) {
    motorPinsLocal[i] = pins[i];
    pinMode(motorPinsLocal[i], OUTPUT);
    digitalWrite(motorPinsLocal[i], LOW);
  }
}

// One step per call when power == true. Caller controls timing.
void motor_step(bool power) {
  if (power) {
    stepIndex = (stepIndex + 1) % 8;
    for (uint8_t i = 0; i < 4; ++i) {
      digitalWrite(motorPinsLocal[i], motorSeq[stepIndex][i] ? HIGH : LOW);
    }
  } else {
    for (uint8_t i = 0; i < 4; ++i) {
      digitalWrite(motorPinsLocal[i], LOW);
    }
  }
}
