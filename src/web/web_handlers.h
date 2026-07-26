#pragma once

#include <DNSServer.h>
#include <WebServer.h>

void setupWebRoutes(WebServer &server);
void processWebStack(WebServer &server, DNSServer &dnsServer);
