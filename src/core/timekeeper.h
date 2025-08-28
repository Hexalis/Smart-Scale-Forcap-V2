#pragma once

// Starts a background task that:
// - waits for NET_UP,
// - runs NTP sync,
// - sets AppBits::TIME_VALID on success, clears it on loss of Wi-Fi,
// - re-checks on re-connects.
void timekeeper_start();

// Simple helper to check if time is valid
bool time_is_valid();

// (Optional) get current epoch (seconds since 1970). Returns 0 if not valid.
unsigned long time_epoch();