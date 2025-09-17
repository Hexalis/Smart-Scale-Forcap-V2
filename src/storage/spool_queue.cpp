#include "spool_queue.h"
#include "app_config.h"

#include <LittleFS.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static const char* SPOOL_PATH = "/spool.jsonl";
static const char* SPOOL_TMP  = "/spool.tmp";
static SemaphoreHandle_t s_mutex = nullptr;

// --------------- helpers ---------------
static inline void lock()   { if (s_mutex) xSemaphoreTake(s_mutex, portMAX_DELAY); }
static inline void unlock() { if (s_mutex) xSemaphoreGive(s_mutex); }

bool spool_init() {
  if (!LittleFS.begin(true)) {        // true -> format on fail (first boot)
    Serial.println("[SPOOL] LittleFS.begin() failed");
    return false;
  }
  if (!LittleFS.exists(SPOOL_PATH)) {
    File f = LittleFS.open(SPOOL_PATH, "w");
    if (!f) {
      Serial.println("[SPOOL] Create file failed");
      return false;
    }
    f.close();
  }
  if (!s_mutex) s_mutex = xSemaphoreCreateMutex();
  return s_mutex != nullptr;
}


static size_t count_lines(File& f) {
  size_t n = 0; f.seek(0, SeekSet);
  while (f.available()) {
    if (f.readStringUntil('\n').length() > 0) n++;
    yield();
  }
  return n;
}

// --------------- API ---------------
bool spool_enqueue(uint32_t epoch, float diff) {
  lock();
  File f = LittleFS.open(SPOOL_PATH, "a+");
  if (!f) { unlock(); return false; }

  // Optional cap: if over limit, don’t grow further
  // (We could drop oldest by rewriting; keep simple and just refuse.)
  size_t curr = count_lines(f);
  if (curr >= SPOOL_MAX_ENTRIES) {
    Serial.println("[SPOOL] At capacity, refusing enqueue");
    f.close();
    unlock();
    return false;
  }

  // Write compact JSON line
  StaticJsonDocument<96> doc;
  doc["ts"]   = epoch;
  doc["diff"] = diff;
  String line;
  serializeJson(doc, line);
  line += '\n';

  size_t w = f.print(line);
  f.flush();         // ensure written
  f.close();
  unlock();
  return (w == line.length());
}

size_t spool_flush(spool_post_fn post) {
  if (!post) return 0;

  lock();
  File f = LittleFS.open(SPOOL_PATH, "r");
  if (!f) { unlock(); return 0; }

  File tmp = LittleFS.open(SPOOL_TMP, "w");   // keep failures here
  if (!tmp) {
    f.close();
    unlock();
    return 0;
  }

  size_t okCount = 0;
  StaticJsonDocument<96> doc;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    if (line.length() == 0) { yield(); continue; }

    DeserializationError err = deserializeJson(doc, line);
    if (err) {
      // Malformed line → drop it (don’t poison the queue)
      Serial.printf("[SPOOL] Bad line, dropping: %s\n", line.c_str());
      continue;
    }

    uint32_t ts = doc["ts"] | 0;
    float    df = doc["diff"] | 0.0f;

    bool ok = post(ts, df);
    if (ok) {
      okCount++;
    } else {
      // Keep it for next time
      tmp.print(line);
    }

    // Yield so other tasks (LED) run
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  f.close();
  tmp.flush();
  tmp.close();

  // Replace original with remaining failures
  LittleFS.remove(SPOOL_PATH);
  LittleFS.rename(SPOOL_TMP, SPOOL_PATH);

  unlock();
  return okCount;
}

size_t spool_count() {
  lock();
  File f = LittleFS.open(SPOOL_PATH, "r");
  if (!f) { unlock(); return 0; }
  size_t n = count_lines(f);
  f.close();
  unlock();
  return n;
}
