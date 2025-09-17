#include "http_client.h"
#include <HTTPClient.h>
#include "core/app_state.h"
#include "app_config.h"

static String s_base;
static constexpr uint8_t  HTTP_RETRIES    = 2;

static String build_url(const String& path) {
  if (s_base.length() == 0) return path;
  if (s_base.endsWith("/"))  return s_base + path;
  return s_base + "/" + path;
}

struct PostingScope {
  PostingScope()  { app_set_bits(AppBits::POSTING); }
  ~PostingScope() { app_clear_bits(AppBits::POSTING); }
};

void http_init(const char* base_url) {
  s_base = base_url ? String(base_url) : String();
}

String http_mac() {
  return WiFi.macAddress();
}

bool http_post_form(const String& path, const String& body, String& outResponse) {
  if (WiFi.status() != WL_CONNECTED) return false;

  PostingScope inFlight;
  const String url = build_url(path);

  for (uint8_t attempt = 0; attempt < uint8_t(1 + HTTP_RETRIES); ++attempt) {
    HTTPClient http;                 // FRESH per attempt
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.setConnectTimeout(HTTP_TIMEOUT_MS);
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("User-Agent", "SmartScale/1.0");

    int code = http.POST(body);
    Serial.printf("[HTTP] → code=%d\r\n", code);

    if (code > 0 && code >= 200 && code < 300) {
      outResponse = http.getString();
      Serial.printf("[HTTP] ← response: %s\r\n", outResponse.c_str());
      http.end();
      return true;
    }
    Serial.printf("[HTTP] POST failed: %s\r\n", http.errorToString(code).c_str());
    http.end();
    vTaskDelay(pdMS_TO_TICKS(250));
  }
  return false;
}
