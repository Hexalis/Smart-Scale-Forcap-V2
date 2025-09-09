#include "ap_portal.h"

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#include "app_config.h"
#include "core/app_state.h"
#include "storage/nvs_store.h"

static WebServer server(80);
static DNSServer dns;

static const byte DNS_PORT = 53;
static bool saved = false;
static uint32_t lastActivityMs = 0;

static const char FORM_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>SmartScale Wi-Fi Setup</title>
<style>
body{font-family:sans-serif;margin:24px;max-width:600px}
input{width:100%;padding:10px;margin:8px 0;box-sizing:border-box}
button{padding:10px 16px}
</style>
</head><body>
<h2>SmartScale Wi-Fi Setup</h2>
<form action="/save" method="post">
  <label>SSID</label><br><input name="ssid" maxlength="32" required><br>
  <label>Password</label><br><input name="pass" type="password" maxlength="64"><br>
  <button type="submit">Save</button>
</form>
</body></html>)HTML";

// -- helpers --
static void touch_activity() { lastActivityMs = millis(); }

// Common redirect to "/"
static void captive_redirect() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
  touch_activity();
}

// Handlers
static void handle_root() {
  server.send(200, "text/html", FORM_HTML);
  touch_activity();
}

static void handle_save() {
  if (!server.hasArg("ssid")) { server.send(400, "text/plain", "Missing ssid"); return; }
  String ssid = server.arg("ssid");
  String pass = server.hasArg("pass") ? server.arg("pass") : "";

  if (ssid.isEmpty()) { server.send(400, "text/plain", "SSID empty"); return; }

  // Ensure NVS is open (in case caller forgot)
  if (!nvs_init("smartscale")) {
    server.send(500, "text/plain", "NVS init failed");
    return;
  }

  bool ok1 = nvs_save_string(WIFI_KEY_SSID, ssid);
  bool ok2 = nvs_save_string(WIFI_KEY_PASS, pass);

  Serial.printf("[AP] save creds: ssid=\"%s\" (%s), pass_len=%u (%s)\r\n",
                ssid.c_str(), ok1 ? "OK" : "ERR",
                (unsigned)pass.length(), ok2 ? "OK" : "ERR");

  if (ok1 && ok2) {
    saved = true;
    server.send(200, "text/plain", "Saved. Device will reboot...");
  } else {
    server.send(500, "text/plain", "Save failed");
  }
  touch_activity();
}

void ap_portal_run() {
  // Make sure NVS is open so saving works
  if (!nvs_init("smartscale")) {
    Serial.println("[AP] ERROR: nvs_init failed");
  }

  // Start AP
  WiFi.mode(WIFI_AP);
  if (strlen(AP_PASS) >= 8)
    WiFi.softAP(AP_SSID, AP_PASS, AP_CHAN);
  else
    WiFi.softAP(AP_SSID, nullptr, AP_CHAN); // open AP if short/empty pass

  IPAddress ip = WiFi.softAPIP();
  Serial.printf("[AP] \"%s\" started. IP=%s\r\n", AP_SSID, ip.toString().c_str());

  app_set_bits(AppBits::AP_MODE);  // your LED should switch to FAST_BLINK

  // DNS catch-all (captive)
  dns.start(DNS_PORT, "*", ip);

  // HTTP server routes
  server.on("/", HTTP_GET, handle_root);
  server.on("/save", HTTP_POST, handle_save);

  // OS captive probes → send 200/204 or redirect to "/"
  server.on("/generate_204", HTTP_GET, [](){ // Android
    server.send(204, "text/plain", "");
    touch_activity();
  });
  server.on("/gen_204", HTTP_GET, [](){ // Android alt
    server.send(204, "text/plain", "");
    touch_activity();
  });
  server.on("/hotspot-detect.html", HTTP_GET, [](){ // iOS/macOS
    handle_root();
  });
  server.on("/ncsi.txt", HTTP_GET, [](){ // Windows
    server.send(200, "text/plain", "Microsoft NCSI");
    touch_activity();
  });
  server.on("/connecttest.txt", HTTP_GET, [](){ // Windows alt
    server.send(200, "text/plain", "OK");
    touch_activity();
  });
  server.on("/success.txt", HTTP_GET, [](){ // Kindle etc.
    server.send(200, "text/plain", "success");
    touch_activity();
  });
  server.on("/fwlink", HTTP_GET, [](){ // old Windows
    captive_redirect();
  });

  // Any other path → redirect to "/"
  server.onNotFound([](){ captive_redirect(); });

  server.begin();

  saved = false;
  lastActivityMs = millis();

  // Service loop
  for (;;) {
    dns.processNextRequest();
    server.handleClient();

    // Idle reboot (optional)
    if (AP_IDLE_REBOOT_MS > 0 && (millis() - lastActivityMs) >= AP_IDLE_REBOOT_MS) {
      Serial.println("[AP] Idle timeout → reboot");
      server.stop();
      WiFi.softAPdisconnect(true);
      app_clear_bits(AppBits::AP_MODE);
      vTaskDelay(pdMS_TO_TICKS(200));
      ESP.restart();
    }

    vTaskDelay(pdMS_TO_TICKS(10)); // yields to other tasks (LED/UI/etc.)

    if (saved) {
      // Shutdown then reboot
      vTaskDelay(pdMS_TO_TICKS(300)); // allow client to receive response
      server.stop();
      WiFi.softAPdisconnect(true);
      app_clear_bits(AppBits::AP_MODE);
      Serial.println("[AP] Saved creds → rebooting...");
      vTaskDelay(pdMS_TO_TICKS(200));
      ESP.restart();
    }
  }
}
