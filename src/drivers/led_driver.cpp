#include "led_driver.h"
#include <FastLED.h>

static int s_gpioPin = -1;
static int s_wsPin = -1;
static bool s_activeLow = true;    // true->LED is on when pin is LOW
static LEDPattern s_pat = LEDPattern::OFF;

// WS2812 state
#define NUM_LEDS 1
static CRGB leds[NUM_LEDS];

// simple scheduler
static uint32_t s_nextMs = 0;
static bool s_state  = false;          // current logical state (true = ON = pin LOW)
static uint8_t s_pulsePhase = 0;

void led_init(int pin, bool activeLow, int ws2812Pin) {
  s_gpioPin = pin;
  s_activeLow = activeLow;
  s_wsPin = ws2812Pin;

  // GPIO LED setup (only if provided)
  if (s_gpioPin >= 0) {
    pinMode(s_gpioPin, OUTPUT);
    // start OFF
    digitalWrite(s_gpioPin, s_activeLow ? HIGH : LOW);
  }

  // WS2812 setup (only if provided)
  if (s_wsPin >= 0) {
    FastLED.addLeds<WS2812, 8, GRB>(leds, NUM_LEDS); //hardcoded to pin 8

    FastLED.clear();
    FastLED.show();
  }

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

  // reset outputs to OFF
  if (s_gpioPin >= 0) {
    digitalWrite(s_gpioPin, s_activeLow ? HIGH : LOW);
  }
  if (s_wsPin >= 0) {
    FastLED.clear();
    FastLED.show();
  }
}

static inline void writeOutputs(bool on) {
  // GPIO LED
  if (s_gpioPin >= 0) {
    if (s_activeLow) {
      digitalWrite(s_gpioPin, on ? LOW : HIGH);
    } else {
      digitalWrite(s_gpioPin, on ? HIGH : LOW);
    }
  }
  // WS2812 LED
  if (s_wsPin >= 0) {
    leds[0] = on ? CRGB::Green : CRGB::Black;
    FastLED.show();
  }
}

void led_tick() {
  const uint32_t now = millis();
  if ((int32_t)(now - s_nextMs) < 0) return;

  switch (s_pat) {
    case LEDPattern::OFF:
      s_state  = false;
      s_nextMs = now + 500;
      break;

    case LEDPattern::SOLID:
      s_state  = true;
      s_nextMs = now + 500;
      break;

    case LEDPattern::SLOW_BLINK:
      s_state  = !s_state;
      s_nextMs = now + (s_state ? 200 : 800);
      break;

    case LEDPattern::FAST_BLINK:
      s_state  = !s_state;
      s_nextMs = now + 200;
      break;

    case LEDPattern::PULSE_1S:
      if (s_pulsePhase == 0) {
        s_state      = true;
        s_nextMs     = now + 50;
        s_pulsePhase = 1;
      } else {
        s_state      = false;
        s_nextMs     = now + 950;
        s_pulsePhase = 0;
      }
      break;
  }

  writeOutputs(s_state);
}

LEDPattern led_getPattern() { return s_pat; }

const char* led_patternName(LEDPattern p) {
  switch (p) {
    case LEDPattern::OFF:        return "OFF";
    case LEDPattern::SOLID:      return "SOLID";
    case LEDPattern::SLOW_BLINK: return "SLOW_BLINK";
    case LEDPattern::FAST_BLINK: return "FAST_BLINK";
    case LEDPattern::PULSE_1S:   return "PULSE_1S";
    default:                     return "?";
  }
}

