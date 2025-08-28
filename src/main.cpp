#include <Arduino.h>
#include "core/app_state.h"
#include "features/supervisor.h"

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.println("SmartScale boot");

  app_state_init();
  supervisor_start();

  Serial.println("Supervisor started");
}

void loop() {
  // Nothing here. Everything runs in tasks.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
