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
#include "net/http_client.h"
#include "core/identity.h"
#include "net/api_client.h"


static void uiTask(void*);
static void buttonTask(void*);
static void testStateTask(void*); //delete later


static bool physPressed(int pin, bool activeLow) {
  int v = digitalRead(pin);
  return activeLow ? (v == LOW) : (v == HIGH);
}

// Returns true ONLY if BOTH buttons are already held at call time,
// and they remain held continuously for 'holdMs'. Otherwise returns false quickly.
static bool boot_combo_held(uint32_t holdMs, bool activeLow) {
  // Configure inputs (pullups for active-low buttons)
  pinMode(BTN1_PIN, activeLow ? INPUT_PULLUP : INPUT);
  pinMode(BTN2_PIN, activeLow ? INPUT_PULLUP : INPUT);

  // If both are NOT held right now → do not block
  if (!(physPressed(BTN1_PIN, activeLow) && physPressed(BTN2_PIN, activeLow))) {
    return false;
  }

  // Both are down now → require a continuous hold for the full duration
  const TickType_t tick = pdMS_TO_TICKS(10);
  const uint32_t start = millis();

  while ((millis() - start) < holdMs) {
    if (!(physPressed(BTN1_PIN, activeLow) && physPressed(BTN2_PIN, activeLow))) {
      return false; // released before timeout
    }
    vTaskDelay(tick); // yield so UI/LED continues to run
  }
  return true; // held long enough
}

// Maps bits to LED base pattern for the main status LED
static LEDPattern selectPatternMain() {
  auto bits = app_get_bits();
  if (bits & AppBits::OTA_ACTIVE)   return LEDPattern::FAST_BLINK;
  if (bits & AppBits::AP_MODE)      return LEDPattern::FAST_BLINK;
  if (bits & AppBits::CALIB_ACTIVE) return LEDPattern::FAST_BLINK;
  if (bits & AppBits::NET_UP)       return LEDPattern::PULSE_1S; // online = subtle pulse
  return LEDPattern::SLOW_BLINK;                                  // default/connecting
}

// Example policy for LED2 (aux LED): show Wi-Fi only
static LEDPattern selectPatternAux() {
  auto bits = app_get_bits();
  if (bits & AppBits::AP_MODE) return LEDPattern::FAST_BLINK; // in setup portal
  if (bits & AppBits::NET_UP)  return LEDPattern::SOLID;      // Wi-Fi OK
  return LEDPattern::OFF;
}



void supervisor_start() {
  // Initial mode & LED
  app_set_mode(AppMode::WIFI_CONNECTING); // default at boot for now

  //LED1
  led_init_gpio(LedId::LED1, LED1_PIN, /*activeLow=*/true);
  // Use GPIO for LED2
  led_init_gpio(LedId::LED2, LED2_PIN, /*activeLow=*/true);
  
  // Optional: mirror LED1 to built-in WS pixel on pin 8, ON = green
  led_enable_ws2812_mirror(/*dataPin=*/8, /*r*/0,/*g*/255,/*b*/0);

  // Default patterns at boot
  led_setPattern(LedId::LED1, LEDPattern::SLOW_BLINK);
  led_setPattern(LedId::LED2, LEDPattern::OFF);

  nvs_init("smartscale");

  // Indicate we’re checking for boot combo
  led_setPattern(LedId::LED1, LEDPattern::FAST_BLINK);
  Serial.println("[BOOT] Hold BOTH buttons ~3s to clear Wi-Fi creds...");

  if (boot_combo_held(BOOT_WIPE_HOLD_MS, /*activeLow=*/true)) {
    Serial.println("[BOOT] Combo detected → clearing Wi-Fi creds");
    led_setPattern(LedId::LED1, LEDPattern::SOLID);

    vTaskDelay(pdMS_TO_TICKS(1000));

    nvs_remove_key(WIFI_KEY_SSID);
    nvs_remove_key(WIFI_KEY_PASS);

    vTaskDelay(pdMS_TO_TICKS(300));
    ESP.restart();
  }

  // continue normal boot
  led_setPattern(LedId::LED1, LEDPattern::SLOW_BLINK);

  wifi_start();

  timekeeper_start();

  http_init(SERVER_BASE_URL);


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

static void uiTask(void*) {
  LEDPattern curMain = LEDPattern::SLOW_BLINK;
  LEDPattern curAux  = LEDPattern::OFF;

  for (;;) {
    LEDPattern desMain = selectPatternMain();
    LEDPattern desAux  = selectPatternAux();

    if (desMain != curMain) {
      curMain = desMain;
      led_setPattern(LedId::LED1, curMain);
      Serial.printf("[LED1] pattern → %s\r\n", led_patternName(curMain));
    }
    if (desAux != curAux) {
      curAux = desAux;
      led_setPattern(LedId::LED2, curAux);
      Serial.printf("[LED2] pattern → %s\r\n", led_patternName(curAux));
    }

    // Tick both LEDs once per loop
    led_tick_all();
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
        Serial.println("[BTN] Measurement finished");
        uint32_t ts = time_epoch();
        bool ok = api_post_finish(ts);
        Serial.printf("[BTN] Measurement finished → POST %s\r\n", ok ? "OK" : "FAIL");
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
