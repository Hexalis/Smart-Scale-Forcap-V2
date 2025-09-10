#pragma once
#include <stdint.h>
// ---- Pins ----
static constexpr int LED1_PIN = 5;
static constexpr int LED2_PIN = 4;

// ---- LED engine ----
static constexpr uint16_t LED_TICK_MS = 25;

// ---- Task configs (words: 1024 words ≈ 4 KB) ----
// UI (LED/status)
static constexpr uint32_t TASK_STACK_UI     = 4096;  // ~16 KB
static constexpr uint8_t  TASK_PRIO_UI      = 2;
static constexpr int8_t   TASK_CORE_UI      = 1;     // 0/1, -1=no affinity

// Wi-Fi manager (connect/retry loop)
static constexpr uint32_t TASK_STACK_WIFI   = 12288;  // Wi-Fi callbacks can be deep
static constexpr uint8_t  TASK_PRIO_WIFI    = 3;
static constexpr int8_t   TASK_CORE_WIFI    = 0;

// Timekeeper (NTP sync)
static constexpr uint32_t TASK_STACK_TIME   = 3072;
static constexpr uint8_t  TASK_PRIO_TIME    = 2;
static constexpr int8_t   TASK_CORE_TIME    = 0;

// Sensor (HX711 @ ~10 Hz + simple logic)
static constexpr uint32_t TASK_STACK_SENSOR = 4096;
static constexpr uint8_t  TASK_PRIO_SENSOR  = 2;
static constexpr int8_t   TASK_CORE_SENSOR  = 1;

// Buttons (debounce/events)
static constexpr uint32_t TASK_STACK_BTNS   = 2048;
static constexpr uint8_t  TASK_PRIO_BTNS    = 2;
static constexpr int8_t   TASK_CORE_BTNS    = 1;

// Button handler (consumes events; may run calibration)
static constexpr uint32_t TASK_STACK_BTN_HANDLER = 4096;
static constexpr uint8_t  TASK_PRIO_BTN_HANDLER  = 1;
static constexpr int8_t   TASK_CORE_BTN_HANDLER  = 1;

// Timeouts
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000; // per attempt
static constexpr uint8_t  WIFI_MAX_ATTEMPTS       = 3;     // then we give up (for now)

// ---- Time / NTP ----
static constexpr char     NTP_SERVER_1[]      = "pool.ntp.org";
static constexpr char     NTP_SERVER_2[]      = "time.google.com";
static constexpr uint32_t NTP_SYNC_TIMEOUT_MS = 5000;     // wait up to 5s for time

// Timezone for Slovenia (Europe/Ljubljana): CET with DST to CEST
static constexpr char     TZ_SLOVENIA[]       = "CET-1CEST,M3.5.0,M10.5.0/3";

// Periodic resync interval (e.g., 1 hour)
static constexpr uint32_t TIME_RESYNC_INTERVAL_MS = 3600UL * 1000UL;

// Buttons
static constexpr int BTN1_PIN = 3;   // change to your wiring
static constexpr int BTN2_PIN = 7;   // change to your wiring
static constexpr uint16_t BTN_DEBOUNCE_MS  = 30;
static constexpr uint16_t BTN_SHORT_MIN_MS = 50;
static constexpr uint16_t BTN_LONG_MS      = 2000;

// --- Weight detection thresholds (tune later) ---
static constexpr float    DELTA_SEND_G     = 20.0f; // trigger threshold
static constexpr float    STABILITY_BAND_G = 6.0f;  // how close readings must be (±band)
static constexpr uint32_t STABILITY_MS     = 800;   // must stay stable for this long

// ---- Server / HTTP ----
static constexpr char     SERVER_BASE_URL[]   = "https://tehtnice.forcapsolutions.net";
static constexpr uint32_t HTTP_TIMEOUT_MS     = 7000;
static constexpr bool     HTTP_TLS_INSECURE   = true;   // dev: accept self-signed

// Device "friendly" name to send with weight posts
static constexpr char DEVICE_NAME[] = "Sample Scale1";

// ---- AP portal (fallback Wi-Fi setup) ----
static constexpr char AP_SSID[]   = "SmartScale-Setup";
static constexpr char AP_PASS[]   = "scale123";      // ≥8 chars or leave "" for open AP
static constexpr uint8_t  AP_CHAN = 6;
static constexpr uint32_t AP_IDLE_REBOOT_MS = 10UL * 60UL * 1000UL; // reboot after 10 min idle

// NVS keys (same "smartscale" namespace you already open via nvs_init)
static constexpr char WIFI_KEY_SSID[] = "wifi_ssid";
static constexpr char WIFI_KEY_PASS[] = "wifi_pass";

// Boot combo to wipe Wi-Fi creds
static constexpr uint32_t BOOT_WIPE_HOLD_MS = 3000;  // hold both buttons for 3s at boot