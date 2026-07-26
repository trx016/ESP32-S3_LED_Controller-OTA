#include "runtime_tasks.h"

#include <Arduino.h>

void runtimeTasksStart(WebServer &server, DNSServer &dnsServer) {
  // Single-loop mode: networking and OTA ticks run in main loop.
  (void)server;
  (void)dnsServer;
}
