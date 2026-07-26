#include "ota_update.h"

#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>

#ifndef APP_VERSION
#define APP_VERSION "0.1.0"
#endif

namespace {

static const char *GITHUB_OWNER = "trx016";
static const char *GITHUB_REPO = "ESP32-S3_LED_Controller-OTA";
static const uint32_t OTA_CHECK_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;

String g_lastStatus = "idle";
String g_latestVersion = "";
bool g_busy = false;
bool g_checkNowRequested = false;
uint32_t g_nextAutoCheckMs = 0;

String jsonExtractString(const String &json, const String &key) {
  const String token = String("\"") + key + "\"";
  const int keyPos = json.indexOf(token);
  if (keyPos < 0) {
    return "";
  }

  const int colonPos = json.indexOf(':', keyPos + token.length());
  if (colonPos < 0) {
    return "";
  }

  const int firstQuote = json.indexOf('"', colonPos + 1);
  if (firstQuote < 0) {
    return "";
  }

  const int secondQuote = json.indexOf('"', firstQuote + 1);
  if (secondQuote < 0) {
    return "";
  }

  return json.substring(firstQuote + 1, secondQuote);
}

String findFirstBinAssetUrl(const String &json) {
  int scanFrom = 0;

  while (true) {
    const int keyPos = json.indexOf("\"browser_download_url\"", scanFrom);
    if (keyPos < 0) {
      return "";
    }

    const int firstQuote = json.indexOf('"', keyPos + 22);
    if (firstQuote < 0) {
      return "";
    }

    const int secondQuote = json.indexOf('"', firstQuote + 1);
    if (secondQuote < 0) {
      return "";
    }

    const String url = json.substring(firstQuote + 1, secondQuote);
    if (url.endsWith(".bin")) {
      return url;
    }

    scanFrom = secondQuote + 1;
  }
}

String normalizeVersion(const String &input) {
  String v = input;
  v.trim();
  if (v.startsWith("v") || v.startsWith("V")) {
    v.remove(0, 1);
  }
  return v;
}

bool isNewerVersionAvailable(const String &latestTag) {
  const String current = normalizeVersion(APP_VERSION);
  const String latest = normalizeVersion(latestTag);
  return !latest.isEmpty() && latest != current;
}

bool fetchLatestRelease(String &outTag, String &outBinUrl, String &outError) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  const String url = String("https://api.github.com/repos/") + GITHUB_OWNER + "/" + GITHUB_REPO + "/releases/latest";

  if (!http.begin(client, url)) {
    outError = "Failed to start release request";
    return false;
  }

  http.addHeader("User-Agent", "ESP32-LED-Controller");
  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    outError = String("Release API HTTP ") + code;
    http.end();
    return false;
  }

  const String payload = http.getString();
  http.end();

  outTag = jsonExtractString(payload, "tag_name");
  outBinUrl = findFirstBinAssetUrl(payload);

  if (outTag.isEmpty()) {
    outError = "No tag_name in release";
    return false;
  }

  return true;
}

bool performOtaFromUrl(const String &binUrl, String &outError) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, binUrl)) {
    outError = "Failed to start binary request";
    return false;
  }

  http.addHeader("User-Agent", "ESP32-LED-Controller");
  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    outError = String("Binary HTTP ") + code;
    http.end();
    return false;
  }

  const int contentLength = http.getSize();
  if (!Update.begin(contentLength > 0 ? static_cast<size_t>(contentLength) : UPDATE_SIZE_UNKNOWN)) {
    outError = String("Update.begin failed: ") + Update.errorString();
    http.end();
    return false;
  }

  WiFiClient *stream = http.getStreamPtr();
  const size_t written = Update.writeStream(*stream);
  if (contentLength > 0 && written != static_cast<size_t>(contentLength)) {
    outError = "Incomplete firmware download";
    Update.abort();
    http.end();
    return false;
  }

  if (!Update.end()) {
    outError = String("Update.end failed: ") + Update.errorString();
    http.end();
    return false;
  }

  if (!Update.isFinished()) {
    outError = "Update not finished";
    http.end();
    return false;
  }

  http.end();
  return true;
}

void runCheckAndMaybeUpdate() {
  String tag;
  String binUrl;
  String err;

  if (!fetchLatestRelease(tag, binUrl, err)) {
    g_lastStatus = String("OTA check failed: ") + err;
    return;
  }

  g_latestVersion = tag;
  if (!isNewerVersionAvailable(tag)) {
    g_lastStatus = String("Already up to date (") + APP_VERSION + ")";
    return;
  }

  if (binUrl.isEmpty()) {
    g_lastStatus = "New release found, but no .bin asset";
    return;
  }

  g_lastStatus = String("Updating to ") + tag + "...";

  if (!performOtaFromUrl(binUrl, err)) {
    g_lastStatus = String("OTA failed: ") + err;
    return;
  }

  g_lastStatus = String("OTA success. Rebooting to ") + tag;
  delay(300);
  ESP.restart();
}

}  // namespace

void otaUpdateBegin() {
  g_lastStatus = String("ready (v") + APP_VERSION + ")";
  g_latestVersion = "";
  g_busy = false;
  g_checkNowRequested = false;
  g_nextAutoCheckMs = millis() + 20000;
}

void otaUpdateProcessTick(uint32_t nowMs, bool staConnected, bool internetConnected, bool autoEnabled) {
  if (g_busy) {
    return;
  }

  if (!staConnected || !internetConnected) {
    return;
  }

  const bool autoDue = autoEnabled && static_cast<int32_t>(nowMs - g_nextAutoCheckMs) >= 0;
  if (!g_checkNowRequested && !autoDue) {
    return;
  }

  g_busy = true;
  g_checkNowRequested = false;
  g_nextAutoCheckMs = nowMs + OTA_CHECK_INTERVAL_MS;
  runCheckAndMaybeUpdate();
  g_busy = false;
}

void otaUpdateRequestCheckNow() {
  g_checkNowRequested = true;
}

String otaUpdateGetCurrentVersion() {
  return APP_VERSION;
}

String otaUpdateGetLatestVersion() {
  return g_latestVersion;
}

String otaUpdateGetLastStatus() {
  return g_lastStatus;
}

bool otaUpdateIsBusy() {
  return g_busy;
}
