/**
 * @file inputs.cpp
 * @brief Implementation of hardware timer-based button polling.
 */

#include "inputs.h"
#include "screen.h"

// --- Pin Definitions ---
#define NEXT_BUTTON_PIN 35
#define SD_BUTTON_PIN   37

// --- State Variables ---
static volatile bool* _sd_enable_ptr = nullptr;
static hw_timer_t* btnTimer = nullptr;

/**
 * @brief Timer Interrupt Service Routine (ISR).
 * @details Runs every 10ms to check button states and debounce them.
 */
void IRAM_ATTR onButtonTimer() {
  static uint8_t next_counter = 0;
  static uint8_t sd_counter = 0;
  static bool next_last_state = LOW; // Logic: LOW is released
  static bool sd_last_state = HIGH;  // Logic: HIGH is released (pullup)

  // 1. Handle "Next Page" Button (Active HIGH based on previous code)
  bool next_curr = digitalRead(NEXT_BUTTON_PIN);
  if (next_curr != next_last_state) {
    next_counter++;
    if (next_counter >= 5) { // 50ms stable
      next_last_state = next_curr;
      next_counter = 0;
      // Trigger action on press (transition to HIGH)
      if (next_curr == HIGH) { 
         screen_next_page(); 
      }
    }
  } else {
    next_counter = 0;
  }

  // 2. Handle "SD Toggle" Button (Active LOW based on previous code)
  bool sd_curr = digitalRead(SD_BUTTON_PIN);
  if (sd_curr != sd_last_state) {
    sd_counter++;
    if (sd_counter >= 5) { // 50ms stable
      sd_last_state = sd_curr;
      sd_counter = 0;
      // Trigger action on press (transition to LOW)
      if (sd_curr == LOW && _sd_enable_ptr != nullptr) {
         *_sd_enable_ptr = !(*_sd_enable_ptr); // Toggle the value
      }
    }
  } else {
    sd_counter = 0;
  }
}

void inputs_init(volatile bool* sd_enable_ptr) {
  _sd_enable_ptr = sd_enable_ptr;

  // Configure Pins
  pinMode(NEXT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SD_BUTTON_PIN, INPUT_PULLUP);

  // Configure hardware timer
  // 1mhz
  btnTimer = timerBegin(1000000);
  
  // Attach ISR
  timerAttachInterrupt(btnTimer, &onButtonTimer);
  
  // Set alarm for 10ms (10000 ticks * 1us)
  timerAlarm(btnTimer, 10000, true, 0);
}