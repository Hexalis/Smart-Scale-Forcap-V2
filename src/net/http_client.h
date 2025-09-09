#pragma once
#include <Arduino.h>

bool http_post_json(const char* path,
                    const String& json,
                    int& out_status,
                    String* out_body = nullptr);

bool http_post_form(const char* path,          // NEW
                    const String& formBody,    // "a=1&b=2" (already encoded)
                    int& out_status,
                    String* out_body = nullptr);

String http_build_url(const char* path);
