#pragma once
#include <Arduino.h>

void http_init(const char* base_url);

// Low-level POST helper (form encoded). Returns true on 2xx and fills response.
bool http_post_form(const String& path, const String& body, String& outResponse);

// Optional helpers
String http_mac();     // Wi-Fi MAC ("AA:BB:...")