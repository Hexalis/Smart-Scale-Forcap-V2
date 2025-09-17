#include "wifi_manager.h"

#include <Arduino.h>
#include <WiFi.h>   // Arduino WiFi for ESP32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "core/app_state.h"
#include "net/ap_portal.h"
#include "storage/nvs_store.h"
#include "core/identity.h"

static void wifiTask(void*);
static void onWiFiEvent(WiFiEvent_t event);

// set by GOT_IP, cleared by DISCONNECTED
static volatile bool s_got_ip = false;

void wifi_start() {
  // Optional: set hostname so your router shows a friendly name
  WiFi.setHostname("SmartScale");

  // Register event callback to react to disconnects/reconnects
  WiFi.onEvent(onWiFiEvent);

  // Spawn the background task that attempts connection and retries
  xTaskCreatePinnedToCore(
    wifiTask,
    "wifi",
    TASK_STACK_WIFI,
    nullptr,
    TASK_PRIO_WIFI,
    nullptr,            // prio 3 is fine; networking is important
    (TASK_CORE_WIFI < 0) ? tskNO_AFFINITY : TASK_CORE_WIFI                     // pin to core 0 (ESP32 WiFi stack prefers core 0)
  );
}

static void wifiTask(void*) {
  // Station mode
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);


  // Try to load saved creds from NVS
  String ssid, pass;
  bool haveCreds = nvs_load_string(WIFI_KEY_SSID, ssid);
  if (haveCreds) (void)nvs_load_string(WIFI_KEY_PASS, pass);

  if (!haveCreds) {
    Serial.println("[WiFi] No saved creds → AP portal");
    ap_portal_run();   // will reboot after saving
    vTaskDelete(nullptr);
    return;
  }

  Serial.printf("[WiFi] Saved SSID: \"%s\" (pass_len=%u)\r\n", ssid.c_str(), (unsigned)pass.length());

  uint8_t attempt = 0;

  for (;;) {

    if (WiFi.status() == WL_CONNECTED) {
      // Already connected; sleep a bit and re-check
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    // Try to connect
    Serial.printf("[WiFi] Connecting to %s ...\r\n", ssid.c_str());
    s_got_ip = false;
    WiFi.begin(ssid.c_str(), pass.c_str());

    const uint32_t t0 = millis();
    while (!s_got_ip && (millis() - t0 < WIFI_CONNECT_TIMEOUT_MS)) {
      vTaskDelay(pdMS_TO_TICKS(100));   // yield while waiting for GOT_IP
    }

    if (s_got_ip) {
      attempt = 0;
      Serial.printf("[WiFi] Connected. IP: %s RSSI: %d\r\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
      
      vTaskDelay(pdMS_TO_TICKS(2000));

      //Do welcome POST here (once per connection)
      identity_ensure_welcome();

      // small debounce before heavy HTTP (TLS warm-up, DHCP settle)
      vTaskDelay(pdMS_TO_TICKS(500));

    } else {
      attempt++;
      Serial.println("[WiFi] Connect timeout.");

      if (attempt >= WIFI_MAX_ATTEMPTS) {
        Serial.println("[WiFi] Max attempts reached → AP portal");
        ap_portal_run();   // never returns, reboots
      } else {
        vTaskDelay(pdMS_TO_TICKS(1000));
      }
    }

    // Small idle delay to avoid tight loops
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

static void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("[WiFi] STA_CONNECTED");
      break;

    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.printf("[WiFi] GOT_IP: %s\r\n", WiFi.localIP().toString().c_str());
      s_got_ip = true;
      app_set_bits(AppBits::NET_UP);
      break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("[WiFi] DISCONNECTED");
      s_got_ip = false;
      app_clear_bits(AppBits::NET_UP);   // LED back to SLOW_BLINK
      // WiFi.begin(...) will be re-called by the task loop if needed
      break;

    default:
      break;
  }
}