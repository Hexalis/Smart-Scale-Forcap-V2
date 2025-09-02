#include "app_state.h"

static EventGroupHandle_t s_events = nullptr;
static AppMode s_mode = AppMode::BOOT;

void app_state_init() {
  if (!s_events) {
    s_events = xEventGroupCreate();
  }
  s_mode = AppMode::BOOT;
}

EventGroupHandle_t app_events() {
  return s_events;
}

void app_set_bits(EventBits_t bits) {
  if (s_events) xEventGroupSetBits(s_events, bits);
}

void app_clear_bits(EventBits_t bits) {
  if (s_events) xEventGroupClearBits(s_events, bits);
}

EventBits_t app_get_bits() {
  if (!s_events) return 0;
  return xEventGroupGetBits(s_events);
}

void app_set_mode(AppMode m) { s_mode = m; }
AppMode app_get_mode() { return s_mode; }

void app_debug_print() {
  auto b = app_get_bits();
  Serial.printf("[APP] bits=0x%02X\r\n", (unsigned)b);
  Serial.printf("  NET_UP=%d TIME_VALID=%d AP_MODE=%d POSTING=%d OTA=%d CALIB=%d\r\n",
    !!(b & AppBits::NET_UP), !!(b & AppBits::TIME_VALID), !!(b & AppBits::AP_MODE),
    !!(b & AppBits::POSTING), !!(b & AppBits::OTA_ACTIVE), !!(b & AppBits::CALIB_ACTIVE));
}
