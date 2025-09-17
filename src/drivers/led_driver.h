#pragma once
#include <Arduino.h>

enum class LedId : uint8_t {
  LED1 = 0,
  LED2 = 1
};

enum class LEDPattern : uint8_t {
  OFF,
  SOLID,
  SLOW_BLINK,
  FAST_BLINK,
  PULSE_1S,
  PULSE_5S,
  PULSE_3S,
  DOUBLE_PULSE_2S,
};

// GPIO LEDs
void led_init_gpio(LedId id, int pin, bool activeLow = false);
void led_setPattern(LedId id, LEDPattern p);
void led_tick_all();
const char* led_patternName(LEDPattern p);

// NEW: mirror LED1 to a single WS2812 pixel (optional)
void led_enable_ws2812_mirror(
  int dataPin,
  uint8_t r_on,
  uint8_t g_on,
  uint8_t b_on,
  uint8_t r_off = 0,
  uint8_t g_off = 0,
  uint8_t b_off = 0
);
