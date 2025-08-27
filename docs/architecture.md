your-project/
├─ platformio.ini
├─ src/
│  ├─ main.cpp                          // boots, starts tasks (tiny), includes app_config
│  ├─ app_config.h                      // pins, URLs, thresholds, stack sizes
│  ├─ core/
│  │  ├─ event_bus.h / event_bus.cpp    // event types, publish/subscribe queue
│  │  ├─ app_state.h / app_state.cpp    // FreeRTOS EventGroup bits, state machine enum
│  │  ├─ timekeeper.h / timekeeper.cpp  // NTP sync + epoch/monotonic anchor
│  ├─ drivers/
│  │  ├─ hx711_driver.h / hx711_driver.cpp   // raw read, calibration, tare
│  │  ├─ led_driver.h / led_driver.cpp       // non-blocking LED patterns
│  │  ├─ buttons.h / buttons.cpp             // ISR/queue debounce, short/long press
│  ├─ net/
│  │  ├─ wifi_manager.h / wifi_manager.cpp   // STA connect, AP portal fallback
│  │  ├─ http_client.h / http_client.cpp     // POST helpers, assign-ID call, backoff
│  │  ├─ ota_manager.h / ota_manager.cpp     // OTA check/apply, versioning, policies
│  ├─ storage/
│  │  ├─ nvs_store.h / nvs_store.cpp         // KV: device_id, creds (if used), tare, calib, anchors
│  │  ├─ spool_queue.h / spool_queue.cpp     // offline FIFO (NVS blob or LittleFS)
│  ├─ features/
│  │  ├─ measurement_logic.h / measurement_logic.cpp // 20 g rule, stability, hysteresis
│  │  ├─ supervisor.h / supervisor.cpp       // orchestrates states, draining, watchdogs
│  └─ util/
│     ├─ log.h / log.cpp                     // logging macros, build info
│     └─ crc.h / crc.cpp                     // simple CRC for spool records (optional)
├─ include/                                  // (optional) shared public headers if you want shorter includes
├─ lib/                                       // third-party libs you vendor locally (usually empty; use lib_deps)
├─ data/                                      // LittleFS/SPIFFS assets (e.g., AP portal page)
├─ test/                                      // PIO unit tests later (spool logic, CRC)
├─ docs/
│  └─ architecture.md                         // copy this structure + event list for future you
└─ scripts/                                   // (optional) custom PIO scripts (e.g., embed git hash)
