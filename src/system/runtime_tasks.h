#pragma once

#include <DNSServer.h>
#include <WebServer.h>

void runtimeTasksStart(WebServer &server, DNSServer &dnsServer);
