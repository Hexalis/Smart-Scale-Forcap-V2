#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "core/app_state.h"
#include "drivers/led_driver.h"
#include "drivers/hx711_driver.h"
#include "net/wifi_manager.h"
#include "core/timekeeper.h"
#include "features/sensor_task.h"
#include "storage/nvs_store.h"
#include "drivers/button_driver.h"
#include "features/calibration.h"


static void uiTask(void*);
static void buttonTask(void*);
static void testStateTask(void*); //delete later

void supervisor_start() {
  // Initial mode & LED
  app_set_mode(AppMode::WIFI_CONNECTING); // default at boot for now
  led_init(LED_PIN, true, 8);
  led_setPattern(LEDPattern::SLOW_BLINK);

  wifi_start();

  timekeeper_start();

  nvs_init("smartscale");

  ButtonDriverConfig bcfg{
    .pin1 = BTN1_PIN,
    .pin2 = BTN2_PIN,
    .activeLow = true,
    .debounceMs = BTN_DEBOUNCE_MS,
    .shortMinMs = BTN_SHORT_MIN_MS,
    .longPressMs = BTN_LONG_MS
  };
  buttons_start(bcfg);

  calibration_try_load();

  sensor_start();

  // Start UI task
  xTaskCreatePinnedToCore(
    uiTask,
    "ui",
    TASK_STACK_UI,
    nullptr,
    TASK_PRIO_UI,
    nullptr,
    (TASK_CORE_UI < 0) ? tskNO_AFFINITY : TASK_CORE_UI
  );

  xTaskCreatePinnedToCore(
    buttonTask,
    "btn_handler",
    TASK_STACK_BTN_HANDLER,
    nullptr,    // stack: calibration needs some room
    TASK_PRIO_BTN_HANDLER,
    nullptr,
    (TASK_CORE_BTN_HANDLER < 0) ? tskNO_AFFINITY : TASK_CORE_BTN_HANDLER 
  );

    // TEMP: start test state toggler
  /*xTaskCreatePinnedToCore(
    testStateTask,
    "testState",
    2048,
    nullptr,
    1,
    nullptr,
    1
  );*/
}

// Maps bits to LED base pattern
static LEDPattern selectPattern() {
  auto bits = app_get_bits();
  if (bits & AppBits::OTA_ACTIVE) return LEDPattern::FAST_BLINK;  // visible activity
  if (bits & AppBits::AP_MODE)   return LEDPattern::FAST_BLINK;   // portal = fast blink
  if (bits & AppBits::CALIB_ACTIVE) return LEDPattern::FAST_BLINK;
  if (bits & AppBits::NET_UP)    return LEDPattern::PULSE_1S;     // online = subtle pulse
  return LEDPattern::SLOW_BLINK;                                   // default/connecting
}

static void uiTask(void*) {
  LEDPattern current = LEDPattern::SLOW_BLINK;

  for (;;) {
    auto desired = selectPattern();
    if (desired != current) {
      current = desired;
      led_setPattern(current);
      Serial.printf("[LED] pattern → %s\r\n", led_patternName(current));
    }

    led_tick();
    vTaskDelay(pdMS_TO_TICKS(LED_TICK_MS));
  }
}

static void buttonTask(void*) {
  for (;;) {
    ButtonEvent ev;
    if (buttons_get_event(ev, portMAX_DELAY)) {   // block until event
      if (ev.type == ButtonEventType::BOTH_LONG) {
        Serial.printf("[BTN] Both long → calibration\r\n");
        app_set_bits(AppBits::CALIB_ACTIVE);

        bool ok = calibration_run_100g();

        app_clear_bits(AppBits::CALIB_ACTIVE);
        Serial.printf("[BTN] Calibration %s\r\n", ok ? "OK" : "FAILED");
      }
      else if (ev.type == ButtonEventType::BTN1_SHORT) {
        app_set_bits(AppBits::CALIB_ACTIVE);
        HX::tare(30);
        app_clear_bits(AppBits::CALIB_ACTIVE);
        Serial.println("[BTN] Tare done");
      }
      else if (ev.type == ButtonEventType::BTN2_SHORT) {
        Serial.println("[BTN] Measurement finished (TODO: POST)");
      }
    }
  }
}

static void testStateTask(void*) { //delete later
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(3000));
    app_set_bits(AppBits::NET_UP);

    vTaskDelay(pdMS_TO_TICKS(3000));
    app_set_bits(AppBits::AP_MODE);

    vTaskDelay(pdMS_TO_TICKS(3000));
    app_clear_bits(AppBits::AP_MODE);

    vTaskDelay(pdMS_TO_TICKS(3000));
    app_set_bits(AppBits::OTA_ACTIVE);

    vTaskDelay(pdMS_TO_TICKS(3000));
    app_clear_bits(AppBits::OTA_ACTIVE);

    vTaskDelay(pdMS_TO_TICKS(3000));
    app_clear_bits(AppBits::NET_UP);
  }
}
