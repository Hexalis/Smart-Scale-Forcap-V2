#pragma once
#include <Arduino.h>

// Load/save ID from the same NVS namespace you already open in nvs_init()
bool identity_load_id(String& out);       // returns true if key exists
bool identity_save_id(const String& id);  // returns true if written

// Welcome handshake: reads local ID, posts welcome(mac,id),
// if server returns different id, updates NVS and logs change.
void identity_ensure_welcome();

String identity_get_id(); // returns current ID or empty string if none