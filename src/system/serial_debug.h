#pragma once

#include <Arduino.h>

void serialDebugBegin(uint32_t baudRate = 115200);
void serialDebugPrintNetworkStartup(const String &apSsid, bool staConnected, bool apEnabled, const String &savedSsid);
void serialDebugPrintTaskStartup();
void serialDebugPrintNetworkSnapshot(const char *reason);
void serialDebugPollCommands();
