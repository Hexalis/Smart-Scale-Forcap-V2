#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

enum class ButtonEventType : uint8_t {
  NONE = 0,
  BTN1_SHORT,
  BTN2_SHORT,
  BTN1_LONG,
  BTN2_LONG,
  BOTH_LONG
};

struct ButtonEvent {
  ButtonEventType type;
  uint32_t ms;  // millis() when event was generated
};

struct ButtonDriverConfig {
  int  pin1;
  int  pin2;
  bool activeLow;           // true if pressed == LOW
  uint16_t debounceMs;      // e.g., 30
  uint16_t shortMinMs;      // min press to count as "short", e.g., 50
  uint16_t longPressMs;     // long press threshold, e.g., 2000
};

// Start the background task (creates internal queue). Returns false on failure.
bool buttons_start(const ButtonDriverConfig& cfg);

// Pop next event; wait up to waitTicks. Returns true if an event was received.
bool buttons_get_event(ButtonEvent& out, TickType_t waitTicks = 0);

// For convenience: is the queue alive?
bool buttons_is_ready();
