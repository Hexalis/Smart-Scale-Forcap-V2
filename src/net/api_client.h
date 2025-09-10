#pragma once
#include <Arduino.h>

// Returns server-reported device_id (may equal your current ID).
// If server doesn’t send an id (or parse fails), returns empty string.
String api_welcome(const String& mac, const String& currentId);

// These we’ll implement after welcome works:
bool api_post_weight(float w, const String& name);
bool api_post_finish(uint32_t epoch);