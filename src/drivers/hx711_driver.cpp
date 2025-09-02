#include "hx711_driver.h"
#include <HX711.h>

static HX711 s_hx;
static bool  s_inited = false;

namespace HX {

bool init(int dout_pin, int sck_pin, uint8_t gain) {
  s_hx.begin(dout_pin, sck_pin, gain);
  // quick readiness probe (up to ~1s)
  unsigned long t0 = millis();
  while (!s_hx.is_ready() && (millis() - t0) < 1000) { vTaskDelay(pdMS_TO_TICKS(5));; }
  s_inited = s_hx.is_ready();
  return s_inited;
}

bool isReady() {
  return s_inited && s_hx.is_ready();
}

bool tare(uint16_t samples) {
  if (!s_inited) return false;
  s_hx.tare(samples);  // library captures offset internally
  return true;
}

void setCalibrationFactor(float scale) {
  // In bogde/HX711: units = (raw_avg - offset) / scale
  // If your known mass is W grams and delta counts is D, then scale = D / W
  s_hx.set_scale(scale);
}

float getCalibrationFactor() {
  return s_hx.get_scale();
}

float getUnits(uint16_t samples) {
  if (!s_inited) {
    Serial.println("[HX] getUnits() called before init!");
  return 0.0f;
}
  return s_hx.get_units(samples);
}

long readRaw() {
  if (!s_inited) return 0;
  return s_hx.read(); // blocks until ready
}

long readRawAverage(uint16_t samples) {
  if (!s_inited || samples == 0) return 0;
  long sum = 0;
  for (uint16_t i = 0; i < samples; ++i) { sum += s_hx.read(); }
  return sum / (long)samples;
}

} // namespace HX
