#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>

#include "led/effects_engine.h"
#include "statusLED_functions.h"
#include "system/ota_update.h"
#include "system/runtime_tasks.h"
#include "system/serial_debug.h"
#include "system/system_state.h"
#include "web/web_handlers.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

static const uint8_t LED_PIN = LED_BUILTIN;

WebServer server(80);
DNSServer dnsServer;

void setupWebServer() {
  setupWebRoutes(server);
}

void setup() {
  serialDebugBegin();
  statusLedBegin(LED_PIN);

  systemStateBegin();
  effectsEngineBegin();
  effectsEngineSetActiveLedCount(systemStateGetLedCount());
  otaUpdateBegin();
  systemStateStartAccessPoint(dnsServer);
  const bool connected = systemStateConnectToSavedWifi();
  systemStateSetError(false);
  serialDebugPrintNetworkStartup(
      systemStateGetApSsid(), connected, systemStateIsApEnabled(), systemStateGetSavedSsid());

  setupWebServer();
  runtimeTasksStart(server, dnsServer);

  serialDebugPrintTaskStartup();
}

void loop() {
  serialDebugPollCommands();
  vTaskDelay(pdMS_TO_TICKS(20));
}