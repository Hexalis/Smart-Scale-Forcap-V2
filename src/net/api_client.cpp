#include "api_client.h"
#include <ArduinoJson.h>
#include "net/http_client.h"
#include "core/identity.h"
#include "core/timekeeper.h"

// Adjust paths if your server uses subpaths; empty "" means base URL
static constexpr const char* PATH_WELCOME = "";
static constexpr const char* PATH_WEIGHT  = "";
static constexpr const char* PATH_FINISH  = "";
static constexpr const char* PATH_READY  = "";

String api_welcome(const String& mac, const String& currentId) {
  String body = "mac=" + mac + "&id=" + (currentId.length() ? currentId : "none");
  String resp;

  Serial.printf("[SERVER] → POST body: %s\r\n", body.c_str());

  if (!http_post_form(PATH_WELCOME, body, resp)) {
    Serial.println("[API] welcome: post failed");
    return String(); // empty
  }

  Serial.printf("[SERVER] ← response: %s\r\n", resp.c_str());

  // Try parse JSON for {"device_id":"..."}
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, resp);
  if (err) {
    Serial.println("[API] welcome: non-JSON (ok if your server replies plain text)");
    return String(); // empty = no change
  }
  if (doc.containsKey("device_id")) {
    String newId = doc["device_id"].as<String>();
    Serial.printf("[SERVER] parsed device_id: %s\r\n", newId.c_str());
    return newId; // may be same or different from current
  }
  return String();
}

// stubs for later:
bool api_post_weight(float w, const String& name) { 
  const String mac = http_mac();
  const String id = identity_get_id();

  char wBuf[24];
  // 2 decimals; adjust if you need 1/3 decimals
  snprintf(wBuf, sizeof(wBuf), "%.2f", w);

  // Build form body exactly as your server expects:
  // mac, id, name, w
  String body;
  body.reserve(128);
  body += "mac=";  body += mac;
  body += "&id=";  body += id;
  body += "&name="; body += name;
  body += "&w=";   body += wBuf;

  String resp;
  Serial.printf("[SERVER] → WEIGHT: %s\r\n", body.c_str());
  const bool ok = http_post_form(PATH_WEIGHT, body, resp);
  Serial.printf("[SERVER] ← WEIGHT resp: %s (ok=%d)\r\n", resp.c_str(), ok);
  return ok;
}

bool api_post_ready(uint32_t epoch) {
  const String mac = http_mac();
  const String id  = identity_get_id();

  String body;
  body.reserve(96);
  body += "mac=";    body += mac;
  body += "&id=";    body += id;
  body += "&event=ready";
  body += "&ts=";    body += String(epoch);   // ok if 0

  String resp;
  Serial.printf("[SERVER] → READY: %s\r\n", body.c_str());
  const bool ok = http_post_form(PATH_READY, body, resp);
  Serial.printf("[SERVER] ← READY resp: %s (ok=%d)\r\n", resp.c_str(), ok);
  return ok;
}

bool api_post_finish(uint32_t epoch) {
  const String mac = http_mac();
  const String id  = identity_get_id();

  String body;
  body.reserve(96);
  body += "mac=";    body += mac;
  body += "&id=";    body += id;
  body += "&event=finish";
  body += "&ts=";    body += String(epoch);   // ok if 0

  String resp;
  Serial.printf("[SERVER] → FINISH: %s\r\n", body.c_str());
  const bool ok = http_post_form(PATH_FINISH, body, resp);
  Serial.printf("[SERVER] ← FINISH resp: %s (ok=%d)\r\n", resp.c_str(), ok);
  return ok;
}