#include "ota_manager.h"
#include "app_config.h"   // defines FW_VERSION and MANIFEST_URL
#include "core/app_state.h"

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

namespace OTA {

static OTA_Status performOTA(const String& binUrl);

// compare semantic versions "x.y.z"
static bool isNewerVersion(const String& latest, const String& current) {
  auto to3 = [](const String& v, int out[3]) {
    int part = 0, start = 0;
    for (int i = 0; i <= v.length() && part < 3; ++i) {
      if (i == v.length() || v[i] == '.') {
        out[part++] = v.substring(start, i).toInt();
        start = i + 1;
      }
    }
    while (part < 3) out[part++] = 0;
  };
  int a[3], b[3];
  to3(latest, a); to3(current, b);
  if (a[0] != b[0]) return a[0] > b[0];
  if (a[1] != b[1]) return a[1] > b[1];
  return a[2] > b[2];
}

OTA_Status checkAndUpdate(bool force) {
  if (app_get_bits() & AppBits::NET_UP == 0) {
    Serial.println(F("[OTA] No WiFi"));
    return NO_WIFI;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000);

  Serial.printf("[OTA] GET manifest: %s\r\n", MANIFEST_URL);
  if (!http.begin(client, MANIFEST_URL)) {
    Serial.println(F("[OTA] http.begin() failed"));
    return HTTP_FAIL;
  }

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[OTA] Manifest GET failed: %d\r\n", code);
    http.end();
    return HTTP_FAIL;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, payload)) {
    Serial.println(F("[OTA] JSON parse error"));
    return JSON_INVALID;
  }

  String latest = doc["version"] | "";
  String binUrl = doc["url"]     | "";
  bool forceMan = doc["force"]   | false;

  if (latest.isEmpty() || binUrl.isEmpty()) {
    Serial.println(F("[OTA] Manifest missing fields"));
    return JSON_INVALID;
  }

  Serial.printf("[OTA] Current=%s, Latest=%s\r\n", FW_VERSION, latest.c_str());

  if (forceMan && latest == FW_VERSION) {
    Serial.println(F("[OTA] Force requested but already on version"));
    return UP_TO_DATE;
  }

  if (!force && !forceMan && !isNewerVersion(latest, FW_VERSION)) {
    Serial.println(F("[OTA] Already up to date"));
    return UP_TO_DATE;
  }

  Serial.printf("[OTA] Update available: %s\r\n", binUrl.c_str());
  return performOTA(binUrl);
}

static OTA_Status performOTA(const String& binUrl) {
  app_set_bits(AppBits::OTA_ACTIVE);
  
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(30000);

  if (!http.begin(client, binUrl)) {
    Serial.println(F("[OTA] http.begin() for bin failed"));
    app_clear_bits(AppBits::OTA_ACTIVE);
    return HTTP_FAIL;
  }

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[OTA] Bin GET failed: %d\r\n", code);
    http.end();
    app_clear_bits(AppBits::OTA_ACTIVE);
    return HTTP_FAIL;
  }

  int len = http.getSize();
  bool beginOK = (len > 0) ? Update.begin(len) : Update.begin(UPDATE_SIZE_UNKNOWN);
  if (!beginOK) {
    Serial.printf("[OTA] Update.begin() failed: %s\r\n", Update.errorString());
    http.end();
    app_clear_bits(AppBits::OTA_ACTIVE);
    return ERROR;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t written = Update.writeStream(*stream);

  if (len > 0 && written != (size_t)len) {
    Serial.printf("[OTA] Written %u / %d bytes\r\n", (unsigned)written, len);
    Update.abort();
    http.end();
    app_clear_bits(AppBits::OTA_ACTIVE);
    return ERROR;
  }

  if (!Update.end()) {
    Serial.printf("[OTA] Update.end() error: %s\r\n", Update.errorString());
    http.end();
    app_clear_bits(AppBits::OTA_ACTIVE);
    return ERROR;
  }

  http.end();

  if (!Update.isFinished()) {
    Serial.println(F("[OTA] Update not finished"));
    app_clear_bits(AppBits::OTA_ACTIVE);
    return ERROR;
  }

  Serial.println(F("[OTA] Success. Rebooting…"));
  delay(500);
  ESP.restart();
  return UPDATED_REBOOTING;
}

} // namespace OTA