#include "ota_update.h"

#include <ctype.h>

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
static const int32_t OTA_MIN_IMAGE_BYTES = 128 * 1024;

String g_lastStatus = "idle";
String g_latestVersion = "";
bool g_updateAvailable = false;
bool g_busy = false;
bool g_checkNowRequested = false;
bool g_installNowRequested = false;
uint32_t g_nextAutoCheckMs = 0;

struct SemVersion {
  int major = 0;
  int minor = 0;
  int patch = 0;
  bool valid = false;
};

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

String findPreferredBinAssetUrl(const String &json) {
  int scanFrom = 0;
  String fallbackUrl;

  while (true) {
    const int nameKey = json.indexOf("\"name\"", scanFrom);
    if (nameKey < 0) {
      return fallbackUrl;
    }

    const int nameQuote1 = json.indexOf('"', json.indexOf(':', nameKey) + 1);
    const int nameQuote2 = nameQuote1 >= 0 ? json.indexOf('"', nameQuote1 + 1) : -1;
    if (nameQuote1 < 0 || nameQuote2 < 0) {
      return fallbackUrl;
    }

    const String assetName = json.substring(nameQuote1 + 1, nameQuote2);
    const int urlKey = json.indexOf("\"browser_download_url\"", nameQuote2);
    if (urlKey < 0) {
      return fallbackUrl;
    }

    const int urlQuote1 = json.indexOf('"', json.indexOf(':', urlKey) + 1);
    const int urlQuote2 = urlQuote1 >= 0 ? json.indexOf('"', urlQuote1 + 1) : -1;
    if (urlQuote1 < 0 || urlQuote2 < 0) {
      return fallbackUrl;
    }

    const String assetUrl = json.substring(urlQuote1 + 1, urlQuote2);
    const bool isBin = assetName.endsWith(".bin") || assetUrl.endsWith(".bin");
    if (isBin) {
      if (fallbackUrl.isEmpty()) {
        fallbackUrl = assetUrl;
      }

      String loweredName = assetName;
      loweredName.toLowerCase();
      if (loweredName.indexOf("firmware") >= 0 || loweredName.indexOf("led_controller") >= 0) {
        return assetUrl;
      }
    }

    scanFrom = urlQuote2 + 1;
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

SemVersion parseSemVersion(const String &input) {
  const String normalized = normalizeVersion(input);
  SemVersion out;

  if (normalized.isEmpty()) {
    return out;
  }

  int values[3] = {0, 0, 0};
  int partIndex = 0;
  int pos = 0;

  while (partIndex < 3 && pos < normalized.length()) {
    String token;
    while (pos < normalized.length() && normalized[pos] != '.') {
      token += normalized[pos++];
    }
    if (pos < normalized.length() && normalized[pos] == '.') {
      ++pos;
    }

    String digits;
    for (size_t i = 0; i < token.length(); ++i) {
      const char c = token[i];
      if (isdigit(static_cast<unsigned char>(c)) == 0) {
        break;
      }
      digits += c;
    }

    if (digits.isEmpty()) {
      return out;
    }

    values[partIndex++] = digits.toInt();
  }

  out.major = values[0];
  out.minor = values[1];
  out.patch = values[2];
  out.valid = true;
  return out;
}

bool isLatestNewer(const String &latestTag, String &outReason) {
  const SemVersion current = parseSemVersion(APP_VERSION);
  const SemVersion latest = parseSemVersion(latestTag);

  if (!current.valid || !latest.valid) {
    outReason = "Invalid version format";
    return false;
  }

  if (latest.major != current.major) {
    return latest.major > current.major;
  }

  if (latest.minor != current.minor) {
    return latest.minor > current.minor;
  }

  if (latest.patch != current.patch) {
    return latest.patch > current.patch;
  }

  outReason = "Already up to date";
  return false;
}

bool fetchLatestRelease(String &outTag, String &outBinUrl, String &outError) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  const String baseUrl = String("https://api.github.com/repos/") + GITHUB_OWNER + "/" + GITHUB_REPO;
  const String latestUrl = baseUrl + "/releases/latest";

  if (!http.begin(client, latestUrl)) {
    outError = "Failed to start release request";
    return false;
  }

  http.setTimeout(12000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("User-Agent", "ESP32-LED-Controller");
  http.addHeader("Accept", "application/vnd.github+json");
  int code = http.GET();
  String payload = http.getString();
  http.end();

  // Fallback for accounts/repos where /latest is unavailable.
  if (code == HTTP_CODE_NOT_FOUND) {
    const String listUrl = baseUrl + "/releases?per_page=1";
    if (!http.begin(client, listUrl)) {
      outError = "Failed to start fallback release request";
      return false;
    }

    http.setTimeout(12000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.addHeader("User-Agent", "ESP32-LED-Controller");
    http.addHeader("Accept", "application/vnd.github+json");

    code = http.GET();
    payload = http.getString();
    http.end();
  }

  if (code != HTTP_CODE_OK) {
    outError = String("Release API HTTP ") + code;
    return false;
  }

  outTag = jsonExtractString(payload, "tag_name");
  outBinUrl = findPreferredBinAssetUrl(payload);

  if (outBinUrl.isEmpty()) {
    outBinUrl = findFirstBinAssetUrl(payload);
  }

  if (outTag.isEmpty()) {
    outError = "No tag_name in release payload";
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

  http.setTimeout(18000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("User-Agent", "ESP32-LED-Controller");
  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    outError = String("Binary HTTP ") + code;
    http.end();
    return false;
  }

  const int contentLength = http.getSize();
  if (contentLength > 0 && contentLength < OTA_MIN_IMAGE_BYTES) {
    outError = "Firmware image unexpectedly small";
    http.end();
    return false;
  }

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

void runCheckAndMaybeUpdate(bool allowInstall) {
  String tag;
  String binUrl;
  String err;

  if (!fetchLatestRelease(tag, binUrl, err)) {
    g_lastStatus = String("OTA check failed: ") + err;
    return;
  }

  g_latestVersion = tag;
  if (!binUrl.startsWith("https://")) {
    g_lastStatus = "Release asset URL must be HTTPS";
    return;
  }

  String versionReason;
  if (!isLatestNewer(tag, versionReason)) {
    g_updateAvailable = false;
    if (versionReason == "Invalid version format") {
      g_lastStatus = String("Version parse error. Current=") + APP_VERSION + ", latest=" + tag;
      return;
    }

    g_lastStatus = String("Already up to date (") + APP_VERSION + ")";
    return;
  }

  if (binUrl.isEmpty()) {
    g_updateAvailable = true;
    g_lastStatus = "New release found, but no .bin asset";
    return;
  }

  g_updateAvailable = true;

  if (!allowInstall) {
    g_lastStatus = String("Update available: ") + tag + " (manual mode)";
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
  g_updateAvailable = false;
  g_busy = false;
  g_checkNowRequested = false;
  g_installNowRequested = false;
  g_nextAutoCheckMs = millis() + 20000;
}

void otaUpdateProcessTick(uint32_t nowMs, bool staConnected, bool internetConnected, bool autoEnabled) {
  if (g_busy) {
    return;
  }

  if (!staConnected || !internetConnected) {
    return;
  }

  const bool installNow = g_installNowRequested;
  const bool autoDue = autoEnabled && static_cast<int32_t>(nowMs - g_nextAutoCheckMs) >= 0;
  const bool checkNow = g_checkNowRequested;
  if (!installNow && !checkNow && !autoDue) {
    return;
  }

  g_busy = true;
  g_checkNowRequested = false;
  g_installNowRequested = false;
  g_nextAutoCheckMs = nowMs + OTA_CHECK_INTERVAL_MS;

  if (installNow) {
    runCheckAndMaybeUpdate(true);
  } else if (checkNow) {
    // Manual check should only detect update availability.
    runCheckAndMaybeUpdate(false);
  } else {
    runCheckAndMaybeUpdate(true);
  }

  g_busy = false;
}

void otaUpdateRequestCheckNow() {
  g_checkNowRequested = true;
}

void otaUpdateRequestInstallNow() {
  g_installNowRequested = true;
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

bool otaUpdateIsUpdateAvailable() {
  return g_updateAvailable;
}
