#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

enum class AppMode : uint8_t {
  BOOT,
  WIFI_CONNECTING,
  ONLINE,
  OFFLINE,
  AP_PORTAL
};

// EventGroup bits
namespace AppBits {
  static constexpr EventBits_t NET_UP      = 1 << 0;
  static constexpr EventBits_t TIME_VALID  = 1 << 1;
  static constexpr EventBits_t AP_MODE     = 1 << 2;
  static constexpr EventBits_t POSTING     = 1 << 3;
  static constexpr EventBits_t OTA_ACTIVE  = 1 << 4;
  static constexpr EventBits_t CALIB_ACTIVE= 1 << 5;
}

void app_state_init();
EventGroupHandle_t app_events();

void app_set_bits(EventBits_t bits);
void app_clear_bits(EventBits_t bits);
EventBits_t app_get_bits();

void app_set_mode(AppMode m);
AppMode app_get_mode();

void app_debug_print();
