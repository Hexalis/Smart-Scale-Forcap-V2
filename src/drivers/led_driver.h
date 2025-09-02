#pragma once
#include <Arduino.h>

enum class LEDPattern : uint8_t {
  OFF,
  SOLID,
  SLOW_BLINK,   // 200ms on / 800ms off
  FAST_BLINK,   // 200ms on / 200ms off
  PULSE_1S      // brief 50ms “online” pulse every 1s
};

void led_init(int gpiopin, bool activeLow = true, int ws2812Pin = -1);
void led_setPattern(LEDPattern p);
void led_tick();             // call every LED_TICK_MS

// for debugging
LEDPattern led_getPattern();
const char* led_patternName(LEDPattern p);