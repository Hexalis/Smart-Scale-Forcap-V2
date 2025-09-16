#pragma once
#include <Arduino.h>

enum OTA_Status {
  OK = 0,
  NO_WIFI,
  HTTP_FAIL,
  JSON_INVALID,
  UP_TO_DATE,
  UPDATED_REBOOTING,
  ERROR
};

// Check manifest and update firmware if newer (or if forced).
// Returns one of the OTA_Status codes.
OTA_Status checkAndUpdate(bool force);

