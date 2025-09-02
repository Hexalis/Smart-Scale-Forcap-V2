#pragma once
#include <Arduino.h>

namespace HX {
  // Initialize the chip. 'gain' is usually 128.
  bool  init(int dout_pin, int sck_pin, uint8_t gain = 128);

  // True if the ADC is up and data is ready
  bool  isReady();

  // Library built-ins, wrapped:
  // Capture current offset (tare) by averaging 'samples'
  bool  tare(uint16_t samples = 10);

  // Set/Read the calibration factor (counts per gram if you calibrate with grams)
  void  setCalibrationFactor(float scale);
  float getCalibrationFactor();

  // Read weight in "units" (grams if you calibrated with grams)
  float getUnits(uint16_t samples = 1);

  // Optional raw helpers
  long  readRaw();
  long  readRawAverage(uint16_t samples = 5);
}
