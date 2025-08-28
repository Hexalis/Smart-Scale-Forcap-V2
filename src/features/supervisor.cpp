#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "core/app_state.h"
#include "drivers/led_driver.h"
#include "net/wifi_manager.h"
#include "core/timekeeper.h"


static void uiTask(void*);
static void testStateTask(void*);

void supervisor_start() {
  // Initial mode & LED
  app_set_mode(AppMode::WIFI_CONNECTING); // default at boot for now
  led_init(LED_PIN);
  led_setPattern(LEDPattern::SLOW_BLINK);

  wifi_start();

  timekeeper_start();

  // Start UI task
  xTaskCreatePinnedToCore(
    uiTask,
    "ui",
    TASK_STACK_UI,
    nullptr,
    TASK_PRIO_UI,
    nullptr,
    TASK_CORE_UI
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
  if (bits & AppBits::NET_UP)    return LEDPattern::PULSE_1S;     // online = subtle pulse
  return LEDPattern::SLOW_BLINK;                                   // default/connecting
}

static void uiTask(void*) {
  LEDPattern current = LEDPattern::SLOW_BLINK;

  for (;;) {
    // Update LED base pattern if status changed
    auto desired = selectPattern();
    if (desired != current) {
      current = desired;
      led_setPattern(current);
    }

    // Tick LED engine
    led_tick();

    vTaskDelay(pdMS_TO_TICKS(LED_TICK_MS));
  }
}

static void testStateTask(void*) {
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
