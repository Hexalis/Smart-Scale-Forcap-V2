#pragma once
#include <Arduino.h>

enum class LEDPattern : uint8_t {
  OFF,
  SOLID,
  SLOW_BLINK,   // 200ms on / 800ms off
  FAST_BLINK,   // 200ms on / 200ms off
  PULSE_1S      // brief 50ms “online” pulse every 1s
};

void led_init(int pin, bool activeLow = false);
void led_setPattern(LEDPattern p);
void led_tick();             // call every LED_TICK_MS
