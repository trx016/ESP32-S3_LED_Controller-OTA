#include "runtime_tasks.h"

#include <Arduino.h>

#include "../led/effects_engine.h"
#include "ota_update.h"
#include "statusLED_functions.h"
#include "system_state.h"

namespace {

WebServer *g_server = nullptr;
DNSServer *g_dnsServer = nullptr;

TaskHandle_t g_webTaskHandle = nullptr;
TaskHandle_t g_ledTaskHandle = nullptr;

void webTask(void *parameter) {
  (void)parameter;

  for (;;) {
    const uint32_t nowMs = millis();

    systemStateProcessConnectivityTick(nowMs);
#if !defined(DEBUG_DISABLE_OTA_RUNTIME) || (DEBUG_DISABLE_OTA_RUNTIME == 0)
    otaUpdateProcessTick(nowMs, systemStateIsStaConnected(), systemStateHasInternet(), systemStateIsAutoOtaEnabled());
#endif

    if (systemStateIsApEnabled()) {
      g_dnsServer->processNextRequest();
    }

    g_server->handleClient();
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

void ledTask(void *parameter) {
  (void)parameter;

  for (;;) {
#if !defined(DEBUG_DISABLE_EFFECTS_TICK) || (DEBUG_DISABLE_EFFECTS_TICK == 0)
#if !defined(DEBUG_EFFECTS_RUN_ON_MAIN_LOOP) || (DEBUG_EFFECTS_RUN_ON_MAIN_LOOP == 0)
    effectsEngineTick();
#endif
#endif
    statusLedTick();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

}  // namespace

void runtimeTasksStart(WebServer &server, DNSServer &dnsServer) {
  g_server = &server;
  g_dnsServer = &dnsServer;

  xTaskCreatePinnedToCore(webTask, "webTask", 6144, nullptr, 1, &g_webTaskHandle, 0);
  xTaskCreatePinnedToCore(ledTask, "ledTask", 3072, nullptr, 2, &g_ledTaskHandle, 1);
}
