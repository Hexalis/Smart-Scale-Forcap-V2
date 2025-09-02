src/
  main.cpp                          // tiny: boot + supervisor_start()

  app_config.h                      // pins, task sizes/priorities, tunables

  core/
    app_state.{h,cpp}               // EventGroup bits + mode getters/setters
    timekeeper.{h,cpp}              // NTP task → sets TIME_VALID

  drivers/
    led_driver.{h,cpp}              // LED patterns (active-low aware), no task
    hx711_driver.{h,cpp}            // thin wrapper over bogde/HX711, no task
    button_driver.{h,cpp}           // debounced buttons → events (task inside)

  net/
    wifi_manager.{h,cpp}            // Wi-Fi connect/retry, sets NET_UP (task)
    http_client.{h,cpp}             // POST helpers (no task)  🟡 (later)

  storage/
    nvs_store.{h,cpp}               // nvs_init + save/load float/struct
    spool_queue.{h,cpp}             // offline measurement FIFO  🟡 (later)

  features/
    supervisor.{h,cpp}              // starts subsystems + LED UI task
    sensor_task.{h,cpp}             // owns HX711 loop @10Hz + 20g logic
    calibration.{h,cpp}             // blocking 100g flow (called when CALIB_ACTIVE)




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