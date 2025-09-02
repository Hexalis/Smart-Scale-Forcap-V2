#pragma once
bool calibration_try_load();     // load saved factor from NVS (if any)
bool calibration_run_100g();     // interactive: empty → 100g, compute & save
