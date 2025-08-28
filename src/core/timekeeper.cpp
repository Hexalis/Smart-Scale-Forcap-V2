#include "timekeeper.h"

#include <Arduino.h>
#include <WiFi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "core/app_state.h"

static void timeTask(void*);

void timekeeper_start() {
  xTaskCreatePinnedToCore(
    timeTask,
    "timekeeper",
    3072,
    nullptr,
    2,
    nullptr,
    0   // time/network work on core 0 is fine
  );
}

static bool waitForTime(uint32_t timeoutMs) {
  // configTime(tz, server1, server2). If you don’t need TZ yet, pass empty TZ.
  // You can set a TZ later like "CET-1CEST,M3.5.0/2,M10.5.0/3"
  configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);

  const uint32_t t0 = millis();
  // getLocalTime tries to fill a tm struct; returns true if time is set
  struct tm tinfo;
  while (millis() - t0 < timeoutMs) {
    if (getLocalTime(&tinfo)) {
      return true;
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
  return false;
}

static void timeTask(void*) {
  bool lastNet = false;

  for (;;) {
    // Check NET_UP bit
    const bool netUp = (app_get_bits() & AppBits::NET_UP) != 0;

    if (netUp && !lastNet) {
    // Just came online → try NTP
    Serial.println("[Time] NET_UP: syncing NTP...");
    if (waitForTime(NTP_SYNC_TIMEOUT_MS)) {
      app_set_bits(AppBits::TIME_VALID);
      time_t now = time(nullptr);
      Serial.printf("[Time] Sync OK. Epoch=%ld\n", (long)now);
    } else {
      app_clear_bits(AppBits::TIME_VALID);
      Serial.println("[Time] Sync FAILED.");
    }
  }

    if (!netUp && lastNet) {
      // Lost network → mark time as potentially invalid (you can keep it if you prefer)
      app_clear_bits(AppBits::TIME_VALID);
      Serial.println("[Time] NET_DOWN: time marked invalid.");
    }

    lastNet = netUp;
    vTaskDelay(pdMS_TO_TICKS(500));
    }
}

bool time_is_valid() {
  return (app_get_bits() & AppBits::TIME_VALID) != 0;
}

unsigned long time_epoch() {
  if (!time_is_valid()) return 0;
  return (unsigned long)time(nullptr);
}
