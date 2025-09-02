#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "drivers/hx711_driver.h"
#include "features/calibration.h"
#include "storage/nvs_store.h"

static constexpr const char* KEY_SCALE = "cal_scale";

static long readRawAvg(uint16_t n) { return HX::readRawAverage(n); }

bool calibration_try_load() {
  float s = 0.0f;
  if (!nvs_load_float(KEY_SCALE, s)) {
    return false;
  }
  HX::setCalibrationFactor(s);
  Serial.printf("[CAL] Loaded scale=%.6f counts/gram\r\n", s);
  return true;
}

bool calibration_run_100g() {
  Serial.printf("\r\n=== Calibration: 100 g ===\r\n");
  Serial.printf("1) Remove all weight. Waiting to stabilize...\r\n");

  // Warm-up + tare for clean offset in getUnits()
  vTaskDelay(pdMS_TO_TICKS(1500));
  (void)HX::getUnits(20);   // throw away a few reads
  HX::tare(20);

  // Capture raw baseline (independent of tare)
  vTaskDelay(pdMS_TO_TICKS(200));
  const long raw_zero = readRawAvg(20);
  Serial.printf("[CAL] raw_zero=%ld\r\n", raw_zero);

  Serial.printf("2) Place the 100 g weight and keep it steady...\r\n");

  // Wait until “something present”
  long raw_ref = 0;
  const uint32_t t0 = millis();
  while (millis() - t0 < 10000) {
    raw_ref = readRawAvg(10);
    if (labs(raw_ref - raw_zero) > 200) break;   // heuristic threshold
    vTaskDelay(pdMS_TO_TICKS(150));
  }

  // Re-average with weight on
  raw_ref = readRawAvg(20);
  const long delta = raw_ref - raw_zero;

  if (labs(delta) < 100) {
    Serial.printf("[CAL] ERROR: too small delta (%ld). Check 100 g or wiring.\r\n", delta);
    return false;
  }

  // counts per gram for bogde/HX711 set_scale
  float scale = (float)delta / 100.0f;
  HX::setCalibrationFactor(scale);

  // Save; report if it fails
  if (!nvs_save_float(KEY_SCALE, scale)) {
    Serial.println("[CAL] WARNING: failed to save scale to NVS");
  }

  Serial.printf("[CAL] raw_ref=%ld  delta=%ld  scale=%.6f counts/gram\r\n",
                raw_ref, delta, scale);

  // Verify & auto-fix sign so 100 g reads positive
  float verify = HX::getUnits(5);
  if (verify < 0.0f) {
    scale = -scale;
    HX::setCalibrationFactor(scale);
    (void)nvs_save_float(KEY_SCALE, scale);
    verify = HX::getUnits(15);
    Serial.printf("[CAL] Sign corrected. New scale=%.6f; Verify=%.1f g\r\n", scale, verify);
  } else {
    Serial.printf("[CAL] Verify: ~%.1f g (with 100 g on)\r\n", verify);
  }

  Serial.printf("[CAL] Done. Factor saved.\r\n\r\n");
  return true;
}