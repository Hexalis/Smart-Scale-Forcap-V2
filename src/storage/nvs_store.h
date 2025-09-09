#pragma once
#include <Arduino.h>

// Call once at boot
bool nvs_init(const char* ns = "smartscale");

// Basic float helpers
bool nvs_save_float(const char* key, float value);
bool nvs_load_float(const char* key, float& out);

// Basic string helpers
bool nvs_save_string(const char* key, const String& value);
bool nvs_load_string(const char* key, String& out);

// Remove any key (float, string, whatever)
bool nvs_remove_key(const char* key);

// (Room to grow later: u32, blobs, structs, etc.)
