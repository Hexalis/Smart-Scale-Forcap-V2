#pragma once
#include <Arduino.h>

// Start the Wi-Fi setup portal.
// Blocks until credentials are saved, then reboots.
// Returns only on reboot (never returns normally).
void ap_portal_run();
