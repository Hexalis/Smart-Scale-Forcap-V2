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
    TASK_STACK_TIME,
    nullptr,
    TASK_PRIO_TIME,
    nullptr,
    (TASK_CORE_TIME < 0) ? tskNO_AFFINITY : TASK_CORE_TIME   // time/network work on core 0 is fine
  );
}

static bool waitForTime(uint32_t timeoutMs) {
  // Configure TZ + NTP
  configTzTime(TZ_SLOVENIA, NTP_SERVER_1, NTP_SERVER_2);

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

// Helper: print both Local & UTC
static void print_times(const char* tag) {
  time_t now = time(nullptr);

  // Local
  struct tm lt;
  localtime_r(&now, &lt);
  char loc[40];
  strftime(loc, sizeof(loc), "%Y-%m-%d %H:%M:%S %Z", &lt);

  // UTC
  struct tm ut;
  gmtime_r(&now, &ut);
  char utc[40];
  strftime(utc, sizeof(utc), "%Y-%m-%d %H:%M:%S UTC", &ut);

  Serial.printf("[Time] %s  Local=%s  UTC=%s\r\n", tag, loc, utc);
}

static void timeTask(void*) {

  bool lastNet = false;
  uint32_t lastSyncMs = 0;  // 0 = never synced in this boot

  for (;;) {
    const bool netUp = (app_get_bits() & AppBits::NET_UP) != 0;
    const uint32_t nowMs = millis();

    if (netUp && !lastNet) {
      // Just came online → try NTP
      Serial.println("[Time] NET_UP: syncing NTP...");
      if (waitForTime(NTP_SYNC_TIMEOUT_MS)) {
        app_set_bits(AppBits::TIME_VALID);
        lastSyncMs = nowMs;
        print_times("Sync OK.");
      }
    }

    if (!netUp && lastNet) {
      // Lost network → mark time as potentially invalid
      app_clear_bits(AppBits::TIME_VALID);
      Serial.println("[Time] NET_DOWN: time marked invalid.");
    }

    // Periodic resync while online
    if (netUp && (nowMs - lastSyncMs >= TIME_RESYNC_INTERVAL_MS)) {
      Serial.println("[Time] Periodic NTP resync...");
      if (waitForTime(NTP_SYNC_TIMEOUT_MS)) {
        app_set_bits(AppBits::TIME_VALID);
        lastSyncMs = nowMs;
        print_times("Resync OK.");
      }
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
