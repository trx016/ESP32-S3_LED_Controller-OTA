#pragma once

#include <Arduino.h>

void otaUpdateBegin();
void otaUpdateProcessTick(uint32_t nowMs, bool staConnected, bool internetConnected, bool autoEnabled);
void otaUpdateRequestCheckNow();

String otaUpdateGetCurrentVersion();
String otaUpdateGetLatestVersion();
String otaUpdateGetLastStatus();
bool otaUpdateIsBusy();
bool otaUpdateIsUpdateAvailable();
