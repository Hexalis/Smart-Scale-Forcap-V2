#pragma once
#include <stdint.h>
// ---- Pins ----
static constexpr int LED_PIN = 5;

// ---- LED engine ----
static constexpr uint16_t LED_TICK_MS = 25;

// ---- FreeRTOS tasks (stack given in words: 1024 words ≈ 4 KB) ----
static constexpr uint32_t TASK_STACK_UI = 2048;  // words (~8 KB)
static constexpr uint8_t  TASK_PRIO_UI  = 1;     // priority
static constexpr int8_t   TASK_CORE_UI  = 1;     // 0 or 1; -1 means "no affinity"

// TEMP test creds — change to your network for now
static constexpr char WIFI_SSID[] = "TP-Link_2C80";
static constexpr char WIFI_PASS[] = "51326410";

// Timeouts
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000; // per attempt
static constexpr uint8_t  WIFI_MAX_ATTEMPTS       = 3;     // then we give up (for now)

// ---- Time / NTP ----
static constexpr char     NTP_SERVER_1[]      = "pool.ntp.org";
static constexpr char     NTP_SERVER_2[]      = "time.google.com";
static constexpr uint32_t NTP_SYNC_TIMEOUT_MS = 5000;   // wait up to 5s for time
