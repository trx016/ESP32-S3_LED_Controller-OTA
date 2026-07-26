#include "system_state.h"

#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "ota_update.h"
#include "serial_debug.h"
#include "statusLED_functions.h"

#ifndef LED_STRIP_LENGTH
#define LED_STRIP_LENGTH 1600
#endif

namespace {

static const char *AP_SSID_PREFIX = "LED-Controller-Setup";

Preferences preferences;

String savedSsid;
String savedPass;
String deviceName;
String apSsid;
String mdnsHostName;

DNSServer *dnsServerRef = nullptr;
bool apEnabled = false;
bool staConnected = false;
bool internetConnected = false;
bool systemError = false;
bool lastStaConnected = false;
bool mdnsStarted = false;
bool autoOtaEnabled = false;
bool internetEnabled = true;
uint16_t ledCount = LED_STRIP_LENGTH;

uint32_t nextInternetProbeMs = 0;
uint32_t wifiConnectedBreathUntilMs = 0;
uint32_t nextWifiRetryMs = 0;
uint32_t apGraceUntilMs = 0;

bool lastLoggedApEnabled = false;
bool lastLoggedStaConnected = false;
bool lastLoggedInternetConnected = false;
String lastLoggedStaIp;
String lastLoggedApIp;

void debugLogNetworkSnapshotIfChanged(const char *reason) {
  const bool staNow = WiFi.status() == WL_CONNECTED;
  const bool apNow = apEnabled;
  const bool internetNow = internetConnected;
  const String staIpNow = staNow ? WiFi.localIP().toString() : String("-");
  const String apIpNow = apNow ? WiFi.softAPIP().toString() : String("-");

  const bool changed =
      staNow != lastLoggedStaConnected ||
      apNow != lastLoggedApEnabled ||
      internetNow != lastLoggedInternetConnected ||
      staIpNow != lastLoggedStaIp ||
      apIpNow != lastLoggedApIp;

  if (!changed) {
    return;
  }

  lastLoggedStaConnected = staNow;
  lastLoggedApEnabled = apNow;
  lastLoggedInternetConnected = internetNow;
  lastLoggedStaIp = staIpNow;
  lastLoggedApIp = apIpNow;

  serialDebugPrintNetworkSnapshot(reason);
}

bool isConnectionBreathWindowActive(uint32_t nowMs) {
  return static_cast<int32_t>(wifiConnectedBreathUntilMs - nowMs) > 0;
}

bool isApGraceWindowActive(uint32_t nowMs) {
  return static_cast<int32_t>(apGraceUntilMs - nowMs) > 0;
}

String buildMdnsHostName() {
  String normalized = deviceName;
  normalized.toLowerCase();

  String out;
  out.reserve(normalized.length());

  for (size_t i = 0; i < normalized.length(); ++i) {
    const char c = normalized[i];
    const bool isAlphaNum = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
    if (isAlphaNum) {
      out += c;
    } else if (c == ' ' || c == '-' || c == '_') {
      out += '-';
    }
  }

  while (out.startsWith("-")) {
    out.remove(0, 1);
  }

  while (out.endsWith("-")) {
    out.remove(out.length() - 1);
  }

  if (out.isEmpty()) {
    out = "led-controller";
  }

  if (out.length() > 63) {
    out = out.substring(0, 63);
  }

  return out;
}

void stopMdnsIfRunning() {
  if (!mdnsStarted) {
    return;
  }

  MDNS.end();
  mdnsStarted = false;
}

void startMdnsIfNeeded() {
  if (!staConnected || mdnsStarted) {
    return;
  }

  if (MDNS.begin(mdnsHostName.c_str())) {
    mdnsStarted = true;
  }
}

void updateConnectionCelebrationWindow() {
  if (staConnected && !lastStaConnected) {
    wifiConnectedBreathUntilMs = millis() + 30000;
    startMdnsIfNeeded();
  } else if (!staConnected && lastStaConnected) {
    stopMdnsIfRunning();
  }
  lastStaConnected = staConnected;
}

String statusLedModeName(StatusLedMode mode) {
  switch (mode) {
    case StatusLedMode::NormalOff:
      return "normal-off";
    case StatusLedMode::WifiConnectedBreathGreen:
      return "wifi-connected-breath-green";
    case StatusLedMode::OtaUpdateAvailableBreathPurple:
      return "ota-update-available-breath-purple";
    case StatusLedMode::OtaInstallingSolidPurple:
      return "ota-installing-solid-purple";
    case StatusLedMode::NoWifiBlinkBlueRed:
      return "no-wifi-blink-blue-red";
    case StatusLedMode::NoInternetBlinkBlue:
      return "no-internet-blink-blue";
    case StatusLedMode::ErrorBlinkRed:
      return "error-blink-red";
  }

  return "unknown";
}

void loadWifiCredentials() {
  preferences.begin("wifi", true);
  savedSsid = preferences.getString("ssid", "");
  savedPass = preferences.getString("pass", "");
  preferences.end();
}

void loadSettings() {
  preferences.begin("settings", true);
  autoOtaEnabled = preferences.getBool("auto_ota", false);
  internetEnabled = preferences.getBool("internet_enabled", true);
  const uint32_t storedLedCount = preferences.getUInt("led_count", LED_STRIP_LENGTH);
  ledCount = static_cast<uint16_t>(constrain(storedLedCount, 1U, static_cast<uint32_t>(LED_STRIP_LENGTH)));
  preferences.end();
}

void loadDeviceName() {
  preferences.begin("device", false);
  deviceName = preferences.getString("name", "");

  if (deviceName.isEmpty()) {
    deviceName = "Device 1";
    preferences.putString("name", deviceName);
  }

  preferences.end();
}

String buildApSsid() {
  String ssid = String(AP_SSID_PREFIX) + " " + deviceName;
  if (ssid.length() > 32) {
    ssid = ssid.substring(0, 32);
  }
  return ssid;
}

bool hasInternetAccess() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  WiFiClient client;
  client.setTimeout(1200);
  const bool ok = client.connect("1.1.1.1", 53);
  client.stop();
  return ok;
}

void enableAccessPoint() {
  if (apEnabled) {
    return;
  }

  // Apply latest device name to SSID when AP is (re)started.
  apSsid = buildApSsid();

  WiFi.mode(WIFI_AP_STA);
  const bool started = WiFi.softAP(apSsid.c_str());
  if (!started) {
    apEnabled = false;
    return;
  }

  if (dnsServerRef != nullptr) {
    dnsServerRef->start(53, "*", WiFi.softAPIP());
  }
  apEnabled = true;
}

void disableAccessPoint() {
  if (!apEnabled) {
    return;
  }

  if (dnsServerRef != nullptr) {
    dnsServerRef->stop();
  }

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apEnabled = false;
}

void scheduleNextWifiRetry(uint32_t nowMs) {
  nextWifiRetryMs = nowMs + 10000;
}

void startWifiReconnectAttempt(uint32_t nowMs) {
  if (savedSsid.isEmpty()) {
    return;
  }

  WiFi.mode(apEnabled ? WIFI_AP_STA : WIFI_STA);
  WiFi.begin(savedSsid.c_str(), savedPass.c_str());
  scheduleNextWifiRetry(nowMs);
}

void updateStatusLedMode() {
  const uint32_t nowMs = millis();

  if (systemError) {
    statusLedSetMode(StatusLedMode::ErrorBlinkRed);
  } else if (otaUpdateIsInstalling()) {
    statusLedSetMode(StatusLedMode::OtaInstallingSolidPurple);
  } else if (WiFi.status() != WL_CONNECTED) {
    statusLedSetMode(StatusLedMode::NoWifiBlinkBlueRed);
  } else if (isConnectionBreathWindowActive(nowMs)) {
    statusLedSetMode(StatusLedMode::WifiConnectedBreathGreen);
  } else if (!internetEnabled) {
    statusLedSetMode(StatusLedMode::NormalOff);
  } else if (internetConnected && !autoOtaEnabled && otaUpdateIsUpdateAvailable()) {
    statusLedSetMode(StatusLedMode::OtaUpdateAvailableBreathPurple);
  } else if (!internetConnected) {
    statusLedSetMode(StatusLedMode::NoInternetBlinkBlue);
  } else {
    statusLedSetMode(StatusLedMode::NormalOff);
  }
}

void refreshConnectivityNow() {
  staConnected = WiFi.status() == WL_CONNECTED;
  updateConnectionCelebrationWindow();
  internetConnected = internetEnabled && hasInternetAccess();
  updateStatusLedMode();
}

}  // namespace

void systemStateBegin() {
  loadDeviceName();
  loadWifiCredentials();
  loadSettings();
  mdnsHostName = buildMdnsHostName();
  nextInternetProbeMs = 0;
  nextWifiRetryMs = 0;
  apGraceUntilMs = 0;
  lastLoggedApEnabled = false;
  lastLoggedStaConnected = false;
  lastLoggedInternetConnected = false;
  lastLoggedStaIp = "";
  lastLoggedApIp = "";
}

void systemStateStartAccessPoint(DNSServer &dnsServer) {
  dnsServerRef = &dnsServer;
  apSsid = buildApSsid();
  enableAccessPoint();
  debugLogNetworkSnapshotIfChanged("access point start");
}

bool systemStateConnectToSavedWifi(uint32_t timeoutMs) {
  if (savedSsid.isEmpty()) {
    staConnected = false;
    internetConnected = false;
    updateStatusLedMode();
    return false;
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(savedSsid.c_str(), savedPass.c_str());

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250);
  }

  refreshConnectivityNow();
  const uint32_t nowMs = millis();
  nextInternetProbeMs = nowMs + 10000;

  if (staConnected) {
    apGraceUntilMs = 0;
    disableAccessPoint();
  } else {
    enableAccessPoint();
    scheduleNextWifiRetry(nowMs);
  }

  debugLogNetworkSnapshotIfChanged("initial wifi connect attempt");

  return staConnected;
}

void systemStateProcessConnectivityTick(uint32_t nowMs) {
  staConnected = WiFi.status() == WL_CONNECTED;
  updateConnectionCelebrationWindow();

  if (!staConnected) {
    internetConnected = false;
    enableAccessPoint();

    if (!savedSsid.isEmpty() && nowMs >= nextWifiRetryMs) {
      startWifiReconnectAttempt(nowMs);
    }

    updateStatusLedMode();
    nextInternetProbeMs = nowMs + 10000;
    debugLogNetworkSnapshotIfChanged("connectivity tick");
    return;
  }

  if (!isApGraceWindowActive(nowMs)) {
    disableAccessPoint();
  } else {
    enableAccessPoint();
  }

  if (nowMs >= nextInternetProbeMs) {
    internetConnected = internetEnabled && hasInternetAccess();
    nextInternetProbeMs = nowMs + 10000;
  }

  updateStatusLedMode();
  debugLogNetworkSnapshotIfChanged("connectivity tick");
}

void systemStateSetError(bool hasError) {
  systemError = hasError;
  updateStatusLedMode();
}

String systemStateStatusJson() {
  const bool connected = WiFi.status() == WL_CONNECTED;
  String mode = "STA_RETRY";
  if (connected && apEnabled) {
    mode = "AP+STA";
  } else if (connected && !apEnabled) {
    mode = "STA_ONLY";
  } else if (!connected && apEnabled) {
    mode = "AP_ONLY";
  }

  const String apIp = WiFi.softAPIP().toString();
  const String staIp = connected ? WiFi.localIP().toString() : "-";
  const String localUrl = (connected && mdnsStarted) ? String("http://") + mdnsHostName + ".local/" : "";
  const String lanUrl = connected ? String("http://") + WiFi.localIP().toString() + "/" : "";

  String json = "{";
  json += "\"mode\":\"" + mode + "\",";
  json += "\"connected\":" + String(connected ? "true" : "false") + ",";
  json += "\"ap_enabled\":" + String(apEnabled ? "true" : "false") + ",";
  json += "\"ap_ip\":\"" + apIp + "\",";
  json += "\"ap_ssid\":\"" + apSsid + "\",";
  json += "\"device_name\":\"" + deviceName + "\",";
  json += "\"sta_ip\":\"" + staIp + "\",";
  json += "\"mdns_host\":\"" + mdnsHostName + "\",";
  json += "\"local_url\":\"" + localUrl + "\",";
  json += "\"lan_url\":\"" + lanUrl + "\",";
  json += "\"ssid\":\"" + savedSsid + "\",";
  json += "\"auto_ota\":" + String(autoOtaEnabled ? "true" : "false") + ",";
  json += "\"internet_enabled\":" + String(internetEnabled ? "true" : "false") + ",";
  json += "\"led_count\":" + String(ledCount) + ",";
  json += "\"led_count_max\":" + String(LED_STRIP_LENGTH) + ",";
  json += "\"ota_busy\":" + String(otaUpdateIsBusy() ? "true" : "false") + ",";
  json += "\"ota_update_available\":" + String(otaUpdateIsUpdateAvailable() ? "true" : "false") + ",";
  json += "\"ota_current\":\"" + otaUpdateGetCurrentVersion() + "\",";
  json += "\"ota_latest\":\"" + otaUpdateGetLatestVersion() + "\",";
  json += "\"ota_status\":\"" + otaUpdateGetLastStatus() + "\",";
  json += "\"internet\":" + String(internetConnected ? "true" : "false") + ",";
  json += "\"status_led\":\"" + statusLedModeName(statusLedGetMode()) + "\"";
  json += "}";
  return json;
}

void systemStateSetAutoOtaEnabled(bool enabled) {
  if (autoOtaEnabled == enabled) {
    return;
  }

  preferences.begin("settings", false);
  preferences.putBool("auto_ota", enabled);
  preferences.end();

  autoOtaEnabled = enabled;
}

bool systemStateIsAutoOtaEnabled() {
  return autoOtaEnabled;
}

void systemStateSetInternetEnabled(bool enabled) {
  if (internetEnabled == enabled) {
    return;
  }

  preferences.begin("settings", false);
  preferences.putBool("internet_enabled", enabled);
  preferences.end();

  internetEnabled = enabled;
  if (!internetEnabled) {
    internetConnected = false;
  }
  updateStatusLedMode();
  serialDebugPrintNetworkSnapshot(enabled ? "internet enabled" : "internet disabled");
}

bool systemStateIsInternetEnabled() {
  return internetEnabled;
}

void systemStateSetLedCount(uint16_t count) {
  const uint16_t clamped = static_cast<uint16_t>(constrain(count, static_cast<uint16_t>(1), static_cast<uint16_t>(LED_STRIP_LENGTH)));
  if (ledCount == clamped) {
    return;
  }

  preferences.begin("settings", false);
  preferences.putUInt("led_count", clamped);
  preferences.end();

  ledCount = clamped;
}

uint16_t systemStateGetLedCount() {
  return ledCount;
}

uint16_t systemStateGetLedCountMax() {
  return LED_STRIP_LENGTH;
}

void systemStateSaveWifiCredentials(const String &ssid, const String &pass) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);
  preferences.end();

  savedSsid = ssid;
  savedPass = pass;
}

void systemStateSetDeviceName(const String &name) {
  String nextName = name;
  nextName.trim();

  if (nextName.isEmpty()) {
    nextName = "Device 1";
  }

  if (nextName.length() > 24) {
    nextName = nextName.substring(0, 24);
  }

  if (nextName == deviceName) {
    return;
  }

  preferences.begin("device", false);
  preferences.putString("name", nextName);
  preferences.end();

  deviceName = nextName;
  mdnsHostName = buildMdnsHostName();
}

void systemStateApplyWifiCredentials(const String &ssid, const String &pass, uint32_t apGraceMs) {
  systemStateSaveWifiCredentials(ssid, pass);

  const uint32_t nowMs = millis();
  apGraceUntilMs = nowMs + apGraceMs;

  // Keep setup AP available briefly while the STA link comes up.
  enableAccessPoint();
  startWifiReconnectAttempt(nowMs);

  internetConnected = false;
  updateStatusLedMode();
  serialDebugPrintNetworkSnapshot("wifi credentials applied");
}

void systemStateClearWifiCredentials() {
  preferences.begin("wifi", false);
  preferences.remove("ssid");
  preferences.remove("pass");
  preferences.end();

  savedSsid = "";
  savedPass = "";

  // Force immediate return to setup mode for deterministic test behavior.
  WiFi.disconnect(true, false);
  apEnabled = false;
  staConnected = false;
  lastStaConnected = false;
  internetConnected = false;
  nextWifiRetryMs = 0;
  apGraceUntilMs = 0;

  stopMdnsIfRunning();

  // Give Wi-Fi stack a brief moment before bringing AP back up.
  delay(120);
  enableAccessPoint();
  updateStatusLedMode();
  serialDebugPrintNetworkSnapshot("wifi credentials cleared");
}

String systemStateGetApSsid() {
  return apSsid;
}

String systemStateGetSavedSsid() {
  return savedSsid;
}

bool systemStateIsStaConnected() {
  return staConnected;
}

bool systemStateHasInternet() {
  return internetConnected;
}

bool systemStateIsApEnabled() {
  return apEnabled;
}
