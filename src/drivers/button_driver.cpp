#include "button_driver.h"
#include "app_config.h"

static ButtonDriverConfig s_cfg{};
static QueueHandle_t s_evtq = nullptr;

static void buttonsTask(void*);

static inline bool physPressed(int pin, bool activeLow) {
  int v = digitalRead(pin);
  return activeLow ? (v == LOW) : (v == HIGH);
}

bool buttons_start(const ButtonDriverConfig& cfg) {
  s_cfg = cfg;

  pinMode(s_cfg.pin1, s_cfg.activeLow ? INPUT_PULLUP : INPUT);
  pinMode(s_cfg.pin2, s_cfg.activeLow ? INPUT_PULLUP : INPUT);

  if (!s_evtq) {
    s_evtq = xQueueCreate(8, sizeof(ButtonEvent));
    if (!s_evtq) return false;
  }

  BaseType_t ok = xTaskCreatePinnedToCore(
    buttonsTask,
    "buttons",
    TASK_STACK_BTNS,
    nullptr,
    TASK_PRIO_BTNS,
    nullptr,
    (TASK_CORE_BTNS < 0) ? tskNO_AFFINITY : TASK_CORE_BTNS
  );
  return ok == pdPASS;
}

bool buttons_is_ready() { return s_evtq != nullptr; }

bool buttons_get_event(ButtonEvent& out, TickType_t waitTicks) {
  if (!s_evtq) return false;
  return xQueueReceive(s_evtq, &out, waitTicks) == pdPASS;
}

static void emit(ButtonEventType type) {
  if (!s_evtq || type == ButtonEventType::NONE) return;
  ButtonEvent ev{ type, millis() };
  xQueueSendToBack(s_evtq, &ev, 0);
}

static void buttonsTask(void*) {
  // Debounce state
  bool lastRaw1 = physPressed(s_cfg.pin1, s_cfg.activeLow);
  bool lastRaw2 = physPressed(s_cfg.pin2, s_cfg.activeLow);
  bool deb1 = lastRaw1, deb2 = lastRaw2;
  uint32_t lastChange1 = millis(), lastChange2 = millis();

  // Press timing
  bool pressed1 = deb1, pressed2 = deb2;
  uint32_t downMs1 = pressed1 ? lastChange1 : 0;
  uint32_t downMs2 = pressed2 ? lastChange2 : 0;

  // BOTH_LONG detection
  bool bothLongFired = false;
  bool swallowSingles = false;   // NEW

  const TickType_t tick = pdMS_TO_TICKS(10);

  for (;;) {
    const bool raw1 = physPressed(s_cfg.pin1, s_cfg.activeLow);
    const bool raw2 = physPressed(s_cfg.pin2, s_cfg.activeLow);
    const uint32_t now = millis();

    // Debounce BTN1
    if (raw1 != lastRaw1) { lastRaw1 = raw1; lastChange1 = now; }
    if ((now - lastChange1) >= s_cfg.debounceMs) deb1 = raw1;

    // Debounce BTN2
    if (raw2 != lastRaw2) { lastRaw2 = raw2; lastChange2 = now; }
    if ((now - lastChange2) >= s_cfg.debounceMs) deb2 = raw2;

    // Track press/release + durations
    if (deb1 && !pressed1) { pressed1 = true;  downMs1 = now; }
    if (!deb1 && pressed1) {
      uint32_t dur = now - downMs1;
      if (!swallowSingles) {   // NEW: ignore if BOTH_LONG fired
        if (dur >= s_cfg.longPressMs) emit(ButtonEventType::BTN1_LONG);
        else if (dur >= s_cfg.shortMinMs) emit(ButtonEventType::BTN1_SHORT);
      }
      pressed1 = false;
    }

    if (deb2 && !pressed2) { pressed2 = true;  downMs2 = now; }
    if (!deb2 && pressed2) {
      uint32_t dur = now - downMs2;
      if (!swallowSingles) {   // NEW: ignore if BOTH_LONG fired
        if (dur >= s_cfg.longPressMs) emit(ButtonEventType::BTN2_LONG);
        else if (dur >= s_cfg.shortMinMs) emit(ButtonEventType::BTN2_SHORT);
      }
      pressed2 = false;
    }

    // BOTH_LONG: both held continuously >= longPressMs
    if (deb1 && deb2) {
      const uint32_t bothDownSince = (downMs1 > downMs2) ? downMs1 : downMs2;
      if (!bothLongFired && (now - bothDownSince) >= s_cfg.longPressMs) {
        emit(ButtonEventType::BOTH_LONG);
        bothLongFired = true;
        swallowSingles = true;    // NEW: block single events until release
        // consume individual events
        pressed1 = false;
        pressed2 = false;
        downMs1 = now;
        downMs2 = now;
      }
    } else {
      bothLongFired = false;
      if (!deb1 && !deb2) {
        swallowSingles = false;  // NEW: re-enable singles once both released
      }
    }

    vTaskDelay(tick);
  }
}
