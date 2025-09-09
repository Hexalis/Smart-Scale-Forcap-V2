#include "nvs_store.h"
#include <Preferences.h>

static Preferences prefs;
static bool s_opened = false;

bool nvs_init(const char* ns) {
  if (s_opened) return true;
  s_opened = prefs.begin(ns, /*readOnly=*/false);
  return s_opened;
}

bool nvs_save_float(const char* key, float value) {
  if (!s_opened) return false;
  size_t written = prefs.putFloat(key, value);
  return written == sizeof(float);
}

bool nvs_load_float(const char* key, float& out) {
  if (!s_opened) return false;
  if (!prefs.isKey(key)) return false;
  out = prefs.getFloat(key, 0.0f);
  return true;
}

bool nvs_save_string(const char* key, const String& value) {
  if (!s_opened) return false;
  size_t written = prefs.putString(key, value);
  return written == value.length();
}

bool nvs_load_string(const char* key, String& out) {
  if (!s_opened) return false;
  if (!prefs.isKey(key)) return false;
  out = prefs.getString(key, "");
  return out.length() > 0;
}

bool nvs_remove_key(const char* key) {
  if (!s_opened) return false;
  if (!prefs.isKey(key)) return false;
  return prefs.remove(key);
}
