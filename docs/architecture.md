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




src/app_config.h

    Pins (HX711, LED=5, BTN1, BTN2)
    Thresholds: DELTA_SEND_G = 20, STABILITY_BAND_G = 5, STABILITY_MS = 300, FORCE_SEND_MS = 5000
    Queue sizes (e.g., MEAS_Q_LEN = 32, BTN_Q_LEN = 8)
    URLs/endpoints, timeouts, NTP servers
    OTA channel + FW_VERSION
    Task stack sizes & priorities (constants, so it’s all visible in one place)

src/core/event_bus.*

    Define enum EventType { NET_ONLINE, NET_OFFLINE, TIME_VALID, MEAS_READY, BTN_TARE, BTN_DONE, BTN_AP, POST_OK, POST_FAIL }
    Define small struct Event { type; payload (union or small struct) }
    Provide one global queue handle accessor (or a couple, e.g., measurementsQ, buttonsQ)
    Utility: publish(Event), receive(q, Event&, timeout)

src/core/app_state.*

    Create EventGroupHandle_t
    Bits constants
    Small helpers: setBit, clearBit, waitFor(bits, timeout), getBits()
    Enum AppMode { BOOT, WIFI_CONNECTING, ONLINE, OFFLINE, AP_PORTAL } and a setter/getter

src/features/supervisor.*

    start() creates tasks and timers (UI, Sensor, Buttons, Comms)
    Performs state transitions on Wi-Fi/AP events
    On NET_ONLINE → schedules time sync, then signals drain
    On OTA start → sets BIT_OTA_ACTIVE, asks Comms to pause drain

src/drivers/led_driver.*

    Pattern table: BOOT, CONNECTING, ONLINE, AP, POSTING overlay, ERROR
    init(pin), setBasePattern(enum), pulseOverlay(ms) or a setFlag(BIT_POSTING); a tick() called every ~25 ms

src/drivers/buttons.*

    Configure GPIO with interrupts
    ISR: push quick edge info to a small queue (FromISR)
    Task: debounce + classify press vs long-press
    Publish BTN_TARE (short B1), BTN_AP (long B1), BTN_DONE (short B2)

src/drivers/hx711_driver.*

    init(), tare(), setCalibration(slope,offset)
    non-blocking sample() → IIR/moving average
    returns current filtered grams
    no posting logic here

src/features/measurement_logic.*

    Holds last sent value, stability window timestamps
    Function maybeEmitMeasurement(current_g, mono_ms)
    if |current - last_sent| >= 20 AND stable for ≥ 300 ms → publish MEAS_READY
    else if now - last_forced >= FORCE_SEND_MS → publish anyway

src/net/wifi_manager.*

    begin() tries STA with timeout/backoff; set/clear BIT_NET_UP
    startAPPortal() serves credentials (you can plug WiFiManager later)
    Persist creds into NVS and reboot
    Raise/lower BIT_AP_MODE

src/core/timekeeper.*

    On NET_UP, run NTP; on success set BIT_TIME_VALID, store {epoch_at_sync, mono_at_sync}
    Utility: monoToEpoch(mono_ms) using last anchor; if invalid, return 0/flag

src/storage/nvs_store.*

    Namespaces: "cfg" and "run"
    Get/set: device_id, tare, calib, Wi-Fi creds (if you store them), anchor

src/storage/spool_queue.*

    Pick one path now:
    NVS blob ring (simpler): fixed-size array of records with head/tail indexes and CRC. Great if you won’t exceed a few hundred items.
    LittleFS (more scalable): append JSON lines and a small index; compact occasionally.
    Expose:
        push(record), peek(record&), pop(), size(), clear()

src/net/http_client.*

    Tiny helpers: postAssignId(), postMeasurement(record), postDone()
    Add retries with exponential backoff; don’t block other tasks forever (use timeouts).
    Return rich result codes so Comms can decide to requeue or drop.

src/net/ota_manager.*

    checkAndUpdate() (blocking call you run inside its own task)
    Verifies TLS or SHA-256, sets next boot partition, reboots
    Expose a scheduleCheck() the Supervisor can call
    src/features/comms_task (lives either in net/ or features/)
    Blocks on MEAS_READY queue, decides send vs spool based on bits
    Listens for NET_ONLINE to drain the spool in FIFO
    Wrap posts with BIT_POSTING flag for LED overlay