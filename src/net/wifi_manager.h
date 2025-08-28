#pragma once

// Start a background task that tries to connect STA and keeps NET_UP updated.
// For now it uses WIFI_SSID/WIFI_PASS from app_config.h.
void wifi_start();