#include "led_driver.h"

#include <FastLED.h>

static int s_pin = -1;
static bool s_activeLow = true;    // true->LED is on when pin is LOW
static LEDPattern s_pat = LEDPattern::OFF;

// WS2812 state
#define NUM_LEDS 1
static CRGB leds[NUM_LEDS];

// simple scheduler
static uint32_t s_nextMs = 0;
static bool s_state  = false;          // current logical state (true = ON = pin LOW)
static uint8_t s_pulsePhase = 0;

void led_init(int pin, bool activeLow) {
  s_pin = pin;
  s_activeLow = activeLow;

  FastLED.addLeds<WS2812, 8, GRB>(leds, NUM_LEDS);  // hardcoded to pin 8
  FastLED.clear();
  FastLED.show();

  pinMode(s_pin, OUTPUT);
  digitalWrite(s_pin, HIGH);   // start OFF (for active-low, HIGH = off)

  s_pat = LEDPattern::OFF;
  s_nextMs = 0;
  s_state = false;
  s_pulsePhase = 0;
}

void led_setPattern(LEDPattern p) {
  s_pat = p;
  // reset internal timing so pattern changes feel snappy
  s_nextMs = 0;
  s_state = false;
  s_pulsePhase = 0;

  FastLED.clear();
  FastLED.show();

  digitalWrite(s_pin, HIGH);   // reset to OFF
}

void led_tick() {
  if (s_pin < 0) return;

  const uint32_t now = millis();

  if ((int32_t)(now - s_nextMs) < 0) return;

  switch (s_pat) {
    case LEDPattern::OFF:
      s_state = false;                  // OFF
      s_nextMs = now + 500;
      break;

    case LEDPattern::SOLID:
      s_state = true;                 // ON
      s_nextMs = now + 500;
      break;

    case LEDPattern::SLOW_BLINK: {
      // 200ms on, 800ms off
      s_state = !s_state;
      s_nextMs = now + (s_state ? 200 : 800);
      break;
    }

    case LEDPattern::FAST_BLINK: {
      // 200ms on, 200ms off
      s_state = !s_state;
      s_nextMs = now + 200;
      break;
    }

    case LEDPattern::PULSE_1S: {
      // 50ms on once per second
      if (s_pulsePhase == 0) {
        s_state = true;               // ON
        s_nextMs = now + 50;
        s_pulsePhase = 1;
      } else {
        s_state = false;                // OFF
        s_nextMs = now + 950;
        s_pulsePhase = 0;
      }
      break;
    }
  }

  // write pin based on activeLow flag
  if (s_activeLow) {
    digitalWrite(s_pin, s_state ? LOW : HIGH);
  } else {
    digitalWrite(s_pin, s_state ? HIGH : LOW);
  }

  // instead of digitalWrite, update WS2812
  if (s_state) {
    leds[0] = CRGB::Green;   // ON = green (pick any color you like)
  } else {
    leds[0] = CRGB::Black;   // OFF
  }
  FastLED.show();
}
