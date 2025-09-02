#pragma once
#include <Arduino.h>

// Call once at boot
bool nvs_init(const char* ns = "smartscale");

// Basic float helpers
bool nvs_save_float(const char* key, float value);
bool nvs_load_float(const char* key, float& out);

// (Room to grow later: u32, blobs, structs, etc.)
