#include "http_client.h"

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "app_config.h"
#include "core/app_state.h"

// ----------------- helpers -----------------

static bool is_https_base() {
  return (strncmp(SERVER_BASE_URL, "https://", 8) == 0);
}

String http_build_url(const char* path) {
  String base = SERVER_BASE_URL;
  String p    = path ? path : "/";
  if (!base.endsWith("/")) base += "/";
  if (p.startsWith("/"))   p.remove(0, 1);
  return base + p;
}

static bool ensure_net() {
  if (!(app_get_bits() & AppBits::NET_UP)) {
    Serial.println("[HTTP] NET_UP is not set");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] WiFi.status != CONNECTED");
    return false;
  }
  return true;
}

// ----------------- core POST engine -----------------

static bool do_post(const char* path,
                    const String& body,
                    const char* contentType,
                    int& out_status,
                    String* out_body)
{
  out_status = 0;
  if (!ensure_net()) return false;

  WiFiClient *raw = nullptr;
  WiFiClientSecure *tls = nullptr;

  if (is_https_base()) {
    tls = new WiFiClientSecure();
    if (!tls) {
      Serial.println("[HTTP] oom tls");
      return false;
    }
#if defined(ARDUINO_ARCH_ESP32)
    if (HTTP_TLS_INSECURE) {
      tls->setInsecure(); // WARNING: dev only
    } else {
      // TODO: tls->setCACert(root_ca_pem); // for production
    }
#endif
    raw = tls;
  } else {
    raw = new WiFiClient();
    if (!raw) {
      Serial.println("[HTTP] oom tcp");
      return false;
    }
  }

  bool ok = false;
  HTTPClient http;
  const String url = http_build_url(path);

  do {
    if (!http.begin(*raw, url)) {
      Serial.printf("[HTTP] begin() failed: %s\n", url.c_str());
      break;
    }

    http.setTimeout(HTTP_TIMEOUT_MS);
    http.addHeader("Content-Type", contentType);

    Serial.printf("[HTTP] POST %s  len=%u  type=%s\n",
                  url.c_str(), body.length(), contentType);

    int code = http.POST((uint8_t*)body.c_str(), body.length());
    out_status = code;

    if (code <= 0) {
      Serial.printf("[HTTP] POST error: %s\n",
                    http.errorToString(code).c_str());
      break;
    }

    String resp = http.getString();
    Serial.printf("[HTTP] code=%d  bodyLen=%u\n",
                  code, resp.length());
    if (out_body) *out_body = resp;

    ok = (code >= 200 && code < 300);
  } while (false);

  http.end();
  delete raw; // cleans up WiFiClient*

  return ok;
}

// ----------------- public wrappers -----------------

bool http_post_json(const char* path,
                    const String& json,
                    int& out_status,
                    String* out_body)
{
  return do_post(path, json, "application/json", out_status, out_body);
}

bool http_post_form(const char* path,
                    const String& formBody,
                    int& out_status,
                    String* out_body)
{
  return do_post(path, formBody, "application/x-www-form-urlencoded",
                 out_status, out_body);
}
