#include "web_handlers.h"

#include <WiFi.h>

#include "../system/ota_update.h"
#include "../system/system_state.h"
#include "config_page.h"
#include "home_page.h"
#include "settings_page.h"

namespace {

WebServer *g_server = nullptr;

bool isSetupStage() {
  return !systemStateIsStaConnected();
}

void sendPortalRedirect(const char *path = "/") {
  g_server->sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + path, true);
  g_server->send(302, "text/plain", "");
}

void handleSetupPage() {
  g_server->send_P(200, "text/html", CONFIG_PAGE_HTML);
}

void handleHomePage() {
  if (isSetupStage()) {
    sendPortalRedirect("/setup");
    return;
  }

  g_server->send_P(200, "text/html", HOME_PAGE_HTML);
}

void handleSettingsPage() {
  g_server->send_P(200, "text/html", SETTINGS_PAGE_HTML);
}

void handleRoot() {
  if (isSetupStage()) {
    handleSetupPage();
    return;
  }

  handleHomePage();
}

void handleCaptivePortalProbe() {
  sendPortalRedirect(isSetupStage() ? "/setup" : "/");
}

void handleStatus() {
  g_server->send(200, "application/json", systemStateStatusJson());
}

void handleWifiSave() {
  if (!g_server->hasArg("ssid")) {
    g_server->send(400, "text/plain", "Missing SSID");
    return;
  }

  const String ssid = g_server->arg("ssid");
  const String pass = g_server->hasArg("pass") ? g_server->arg("pass") : "";

  if (ssid.isEmpty()) {
    g_server->send(400, "text/plain", "SSID cannot be empty");
    return;
  }

  systemStateApplyWifiCredentials(ssid, pass, 30000);
  g_server->send(200, "text/plain", "Saved Wi-Fi. Connecting now; AP stays up for 30 seconds after connect.");
}

void handleWifiClear() {
  systemStateClearWifiCredentials();
  g_server->send(200, "text/plain", "Saved Wi-Fi cleared.");
}

void handleDeviceNameSave() {
  if (!g_server->hasArg("name")) {
    g_server->send(400, "text/plain", "Missing device name");
    return;
  }

  const String name = g_server->arg("name");
  systemStateSetDeviceName(name);
  g_server->send(200, "text/plain", "Device name saved.");
}

void handleOtaSettingSave() {
  if (!g_server->hasArg("enabled")) {
    g_server->send(400, "text/plain", "Missing enabled value");
    return;
  }

  const String enabledArg = g_server->arg("enabled");
  const bool enabled = enabledArg == "1" || enabledArg == "true" || enabledArg == "on";

  systemStateSetAutoOtaEnabled(enabled);
  g_server->send(200, "text/plain", enabled ? "Automatic OTA updates enabled." : "Automatic OTA updates disabled.");
}

void handleOtaCheckNow() {
  otaUpdateRequestCheckNow();
  g_server->send(202, "text/plain", "OTA check requested.");
}

void handleOtaInstallNow() {
  otaUpdateRequestInstallNow();
  g_server->send(202, "text/plain", "OTA install requested.");
}

void handleRestart() {
  g_server->send(200, "text/plain", "Restarting device...");
  delay(300);
  ESP.restart();
}

}  // namespace

void setupWebRoutes(WebServer &server) {
  g_server = &server;

  server.on("/", HTTP_GET, handleRoot);
  server.on("/home", HTTP_GET, handleHomePage);
  server.on("/setup", HTTP_GET, handleSetupPage);
  server.on("/settings", HTTP_GET, handleSettingsPage);
  server.on("/generate_204", HTTP_GET, handleCaptivePortalProbe);
  server.on("/gen_204", HTTP_GET, handleCaptivePortalProbe);
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptivePortalProbe);
  server.on("/library/test/success.html", HTTP_GET, handleCaptivePortalProbe);
  server.on("/connecttest.txt", HTTP_GET, handleCaptivePortalProbe);
  server.on("/ncsi.txt", HTTP_GET, handleCaptivePortalProbe);

  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/wifi", HTTP_POST, handleWifiSave);
  server.on("/api/wifi/clear", HTTP_POST, handleWifiClear);
  server.on("/api/device", HTTP_POST, handleDeviceNameSave);
  server.on("/api/settings/ota", HTTP_POST, handleOtaSettingSave);
  server.on("/api/ota/check", HTTP_POST, handleOtaCheckNow);
  server.on("/api/ota/install", HTTP_POST, handleOtaInstallNow);
  server.on("/api/restart", HTTP_POST, handleRestart);

  server.onNotFound([]() { sendPortalRedirect(isSetupStage() ? "/setup" : "/"); });

  server.begin();
}

void processWebStack(WebServer &server, DNSServer &dnsServer) {
  dnsServer.processNextRequest();
  server.handleClient();
}
