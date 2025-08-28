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
  xEventGroupSetBits(s_events, bits);
}

void app_clear_bits(EventBits_t bits) {
  xEventGroupClearBits(s_events, bits);
}

EventBits_t app_get_bits() {
  return xEventGroupGetBits(s_events);
}

void app_set_mode(AppMode m) { s_mode = m; }
AppMode app_get_mode() { return s_mode; }
