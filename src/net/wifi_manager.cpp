#include "wifi_manager.h"

#include <Arduino.h>
#include <WiFi.h>   // Arduino WiFi for ESP32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_config.h"
#include "core/app_state.h"

static void wifiTask(void*);
static void onWiFiEvent(WiFiEvent_t event);

void wifi_start() {
  // Optional: set hostname so your router shows a friendly name
  WiFi.setHostname("SmartScale");

  // Register event callback to react to disconnects/reconnects
  WiFi.onEvent(onWiFiEvent);

  // Spawn the background task that attempts connection and retries
  xTaskCreatePinnedToCore(
    wifiTask,
    "wifi",
    4096,
    nullptr,
    3,
    nullptr,            // prio 3 is fine; networking is important
    0                      // pin to core 0 (ESP32 WiFi stack prefers core 0)
  );
}

static void wifiTask(void*) {
  // Station mode
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);      // don't write to flash on each connect

  uint8_t attempt = 0;

  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      // Already connected; sleep a bit and re-check
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    // Try to connect
    Serial.printf("[WiFi] Connecting to %s ...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    const uint32_t t0 = millis();
    bool connected = false;
    while (millis() - t0 < WIFI_CONNECT_TIMEOUT_MS) {
        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    if (connected) {
      attempt = 0; // reset attempts on success
      Serial.printf("[WiFi] Connected. IP: %s RSSI: %d\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
      app_set_bits(AppBits::NET_UP);         // <-- LED will flip to PULSE_1S
    } else {
      attempt++;
      Serial.println("[WiFi] Connect timeout.");
      app_clear_bits(AppBits::NET_UP);       // ensure flag is clear

      if (attempt >= WIFI_MAX_ATTEMPTS) {
        Serial.println("[WiFi] Max attempts reached. Waiting before retry...");
        // Later we’ll switch to AP-portal here. For now just back off.
        vTaskDelay(pdMS_TO_TICKS(5000));
        attempt = 0; // allow further retries
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
      Serial.printf("[WiFi] GOT_IP: %s\n", WiFi.localIP().toString().c_str());
      app_set_bits(AppBits::NET_UP);
      break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("[WiFi] DISCONNECTED");
      app_clear_bits(AppBits::NET_UP);   // LED back to SLOW_BLINK
      // WiFi.begin(...) will be re-called by the task loop if needed
      break;

    default:
      break;
  }
}