#include "system_state.h"

#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "ota_update.h"
#include "statusLED_functions.h"

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

uint32_t nextInternetProbeMs = 0;
uint32_t wifiConnectedBreathUntilMs = 0;
uint32_t nextWifiRetryMs = 0;
uint32_t apGraceUntilMs = 0;

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
  } else if (WiFi.status() != WL_CONNECTED) {
    statusLedSetMode(StatusLedMode::NoWifiBlinkBlueRed);
  } else if (isConnectionBreathWindowActive(nowMs)) {
    statusLedSetMode(StatusLedMode::WifiConnectedBreathGreen);
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
  internetConnected = hasInternetAccess();
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
}

void systemStateStartAccessPoint(DNSServer &dnsServer) {
  dnsServerRef = &dnsServer;
  apSsid = buildApSsid();
  enableAccessPoint();
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
    return;
  }

  if (!isApGraceWindowActive(nowMs)) {
    disableAccessPoint();
  } else {
    enableAccessPoint();
  }

  if (nowMs >= nextInternetProbeMs) {
    internetConnected = hasInternetAccess();
    nextInternetProbeMs = nowMs + 10000;
  }

  updateStatusLedMode();
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
  json += "\"ota_busy\":" + String(otaUpdateIsBusy() ? "true" : "false") + ",";
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
