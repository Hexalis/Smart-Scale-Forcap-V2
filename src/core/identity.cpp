#include "identity.h"
#include "storage/nvs_store.h"
#include "net/api_client.h"
#include "net/http_client.h"

static constexpr const char* KEY_DEVICE_ID = "device_id";

bool identity_load_id(String& out) {
  return nvs_load_string(KEY_DEVICE_ID, out);
}

bool identity_save_id(const String& id) {
  return nvs_save_string(KEY_DEVICE_ID, id);
}

String identity_get_id() {
  String id;
  if(!identity_load_id(id) || id.length() == 0) {
    return "none";
  }
  return id;
}

void identity_ensure_welcome() {
  String localId;
  bool haveLocal = identity_load_id(localId);
  if (!haveLocal) localId = "none";

  String mac = http_mac();

  Serial.printf("[SERVER] → welcome POST: mac=%s id=%s\r\n", mac.c_str(), localId.c_str());

  String serverId = api_welcome(mac, localId);
  if (serverId.length() == 0) {
    Serial.println("[ID] welcome: server gave no id (or parse failed) → keeping current");
    return;
  }
  else{
    Serial.printf("[SERVER] ← server_id=%s\r\n", serverId.c_str());
  }

  if (serverId != localId) {
    if (identity_save_id(serverId)) {
      Serial.printf("[ID] updated device_id → %s\r\n", serverId.c_str());
    } else {
      Serial.println("[ID] ERROR: failed to save device_id");
    }
  } else {
    Serial.println("[ID] device_id unchanged");
  }
}