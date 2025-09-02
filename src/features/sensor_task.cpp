#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "drivers/hx711_driver.h"
#include "core/app_state.h"
#include "features/calibration.h"

// --- Pins (set to your wiring) ---
static constexpr int HX_DOUT = 1;   // change me
static constexpr int HX_SCK  = 10;  // change me

// --- Calibration factor ---
// If unknown, set to 1.0 so "units" are raw counts.
// After calibration set proper factor so HX::getUnits() returns grams.
static constexpr float INITIAL_SCALE = 42.40f;

// --- RTOS task entry ---
static void sensorTask(void*);

void sensor_start() {
  xTaskCreatePinnedToCore(
    sensorTask,
    "sensor",
    TASK_STACK_SENSOR,          // stack words (~16 KB)
    nullptr,
    TASK_PRIO_SENSOR,             // priority
    nullptr,
    (TASK_CORE_SENSOR < 0) ? tskNO_AFFINITY : TASK_CORE_SENSOR             // run on core 1 (Wi-Fi mostly uses core 0)
  );
}

static void sensorTask(void*) {
  Serial.printf("[SENSOR] init HX711...\r\n");
  if (!HX::init(HX_DOUT, HX_SCK, 128)) {
    Serial.printf("[SENSOR] HX711 not ready (check pins/wiring)\r\n");
    vTaskDelete(nullptr);
    return;
  }

  HX::setCalibrationFactor(INITIAL_SCALE);
  
  calibration_try_load();

  // Tare at boot (scale empty!)
  vTaskDelay(pdMS_TO_TICKS(2000));
  (void)HX::getUnits(20);   // throw away
  HX::tare(50);
  Serial.printf("[SENSOR] tared\r\n");

  const TickType_t period = pdMS_TO_TICKS(100); // 100 ms = 10 Hz

  // --- Filtering for smoothness (single-pole IIR) ---
  bool  haveFilt = false;
  float filt     = 0.0f;

  // --- Simple state machine for stable detection ---
  enum DetectState { IDLE, STABILIZING };
  DetectState state = IDLE;

  float    lastStable   = 0.0f;  // last accepted stable value
  float    candidate    = 0.0f;  // target we’re trying to stabilize around
  bool     inBand       = false;
  uint32_t stableSince  = 0;
  
  for (;;) {
  // Pause sensor during calibration
  if (app_get_bits() & AppBits::CALIB_ACTIVE) {
    vTaskDelay(pdMS_TO_TICKS(100));
    continue;
  }

  // Read an instantaneous-but-averaged value from HX711
  float reading = HX::getUnits(10);   // already averaged by the lib

  switch (state) {
    case IDLE: {
      float delta = fabsf(reading - lastStable);
      if (delta >= DELTA_SEND_G) {
        candidate   = reading;
        inBand      = false;
        stableSince = millis();
        state       = STABILIZING;
        Serial.printf("[MEAS] Δ=%.1fg detected → stabilizing near %.1f g\r\n", delta, candidate);
      }
      break;
    }

    case STABILIZING: {
      if (fabsf(reading - candidate) <= STABILITY_BAND_G) {
        if (!inBand) { inBand = true; stableSince = millis(); }
      } else {
        candidate   = reading;
        inBand      = false;
        stableSince = millis();
      }

      if (inBand && (millis() - stableSince) >= STABILITY_MS) {
        // Take a precise value NOW (don’t use any IIR)
        float finalVal = HX::getUnits(20);

        float prev = lastStable;
        lastStable = finalVal;

        const bool increased = (finalVal - prev) >= 0.0f;
        const char* kind     = increased ? "ADD" : "REMOVE";
        Serial.printf("[MEAS] %s stable: %.1f g (prev %.1f g)\r\n", kind, finalVal, prev);

        state  = IDLE;
        inBand = false;
      }
      break;
    }
  }

  vTaskDelay(period);  // ~100 ms (10 Hz)
}

}
