#pragma once

#include <Arduino.h>
#include <DNSServer.h>

void systemStateBegin();

void systemStateStartAccessPoint(DNSServer &dnsServer);
bool systemStateConnectToSavedWifi(uint32_t timeoutMs = 15000);
void systemStateProcessConnectivityTick(uint32_t nowMs);

void systemStateSetError(bool hasError);

String systemStateStatusJson();
void systemStateSetAutoOtaEnabled(bool enabled);
bool systemStateIsAutoOtaEnabled();
void systemStateSetDeviceName(const String &name);
void systemStateSaveWifiCredentials(const String &ssid, const String &pass);
void systemStateApplyWifiCredentials(const String &ssid, const String &pass, uint32_t apGraceMs = 30000);
void systemStateClearWifiCredentials();

String systemStateGetApSsid();
String systemStateGetSavedSsid();
bool systemStateIsStaConnected();
bool systemStateHasInternet();
bool systemStateIsApEnabled();
