#include "led_driver.h"
#include <FastLED.h>

#ifndef MAX_LEDS_LOGICAL
#define MAX_LEDS_LOGICAL 2
#endif

struct LedState {
  int       pin         = -1;
  bool      activeLow   = false;
  LEDPattern pattern    = LEDPattern::OFF;
  uint32_t  nextMs      = 0;
  bool      logicalOn   = false;  // abstract on/off decided by pattern
  uint8_t   pulsePhase  = 0;
};

static LedState s_leds[MAX_LEDS_LOGICAL];

// ---- WS2812 mirror (optional) ----
static bool   s_wsMirrorEnabled = false;
static CRGB   s_wsPixel[1];
static uint8_t s_ws_on[3]  = {0, 255, 0}; // default ON = green
static uint8_t s_ws_off[3] = {0, 0, 0};   // default OFF = black

static void write_gpio(const LedState* s, bool on) {
  if (!s || s->pin < 0) return;
  digitalWrite(s->pin, (s->activeLow ? (on ? LOW : HIGH) : (on ? HIGH : LOW)));
}

static void reset_runtime(LedState* S) {
  if (!S) return;
  S->nextMs = 0;
  S->logicalOn = false;
  S->pulsePhase = 0;
  write_gpio(S, false);
}

static void tick_one(LedState* L, uint32_t now) {
  if (!L || L->pin < 0) return;
  if ((int32_t)(now - L->nextMs) < 0) return;

  switch (L->pattern) {
    case LEDPattern::OFF:
      L->logicalOn = false; L->nextMs = now + 500; break;
    case LEDPattern::SOLID:
      L->logicalOn = true;  L->nextMs = now + 500; break;
    case LEDPattern::SLOW_BLINK:
      L->logicalOn = !L->logicalOn;
      L->nextMs = now + (L->logicalOn ? 200 : 800);
      break;
    case LEDPattern::FAST_BLINK:
      L->logicalOn = !L->logicalOn;
      L->nextMs = now + 200;
      break;
    case LEDPattern::PULSE_1S:
      if (L->pulsePhase == 0) {
        L->logicalOn = true;  L->nextMs = now + 50;  L->pulsePhase = 1;
      } else {
        L->logicalOn = false; L->nextMs = now + 950; L->pulsePhase = 0;
      }
      break;
  }

  write_gpio(L, L->logicalOn);
}

void led_init_gpio(LedId id, int pin, bool activeLow) {
  uint8_t i = (id == LedId::LED1) ? 0 : 1;
  s_leds[i].pin = pin;
  s_leds[i].activeLow = activeLow;
  s_leds[i].pattern = LEDPattern::OFF;
  s_leds[i].nextMs = 0;
  s_leds[i].logicalOn = false;
  s_leds[i].pulsePhase = 0;

  pinMode(pin, OUTPUT);
  write_gpio(&s_leds[i], false);
}

void led_setPattern(LedId id, LEDPattern p) {
  uint8_t i = (id == LedId::LED1) ? 0 : 1;
  s_leds[i].pattern = p;
  reset_runtime(&s_leds[i]);
}

void led_tick_all() {
  uint32_t now = millis();

  // Tick LED1 and LED2 (GPIO)
  tick_one(&s_leds[0], now);
  tick_one(&s_leds[1], now);

  // Mirror LED1’s logical state onto WS2812 if enabled
  if (s_wsMirrorEnabled) {
    if (s_leds[0].logicalOn) {
      s_wsPixel[0].setRGB(s_ws_on[0], s_ws_on[1], s_ws_on[2]);
    } else {
      s_wsPixel[0].setRGB(s_ws_off[0], s_ws_off[1], s_ws_off[2]);
    }
    FastLED.show();
  }
}

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

void led_enable_ws2812_mirror(int dataPin,
                              uint8_t r_on, uint8_t g_on, uint8_t b_on,
                              uint8_t r_off, uint8_t g_off, uint8_t b_off) {
  FastLED.addLeds<WS2812, /*DATA=*/8, GRB>(s_wsPixel, 1);
  s_wsPixel[0] = CRGB::Black;
  FastLED.show();

  s_ws_on[0] = r_on;  s_ws_on[1] = g_on;  s_ws_on[2] = b_on;
  s_ws_off[0] = r_off; s_ws_off[1] = g_off; s_ws_off[2] = b_off;

  s_wsMirrorEnabled = true;
}
