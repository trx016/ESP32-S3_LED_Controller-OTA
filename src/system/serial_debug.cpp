#include "serial_debug.h"

#include <WiFi.h>

#include "ota_update.h"
#include "system_state.h"

namespace {

String g_serialCommandBuffer;
uint32_t g_lastCommandByteMs = 0;

void serialDebugPrintCommandHelp() {
  Serial.println("Serial commands:");
  Serial.println("  help       - show this help");
  Serial.println("  commands   - show this help");
  Serial.println("  ?          - show this help");
  Serial.println("  clearwifi  - clear saved Wi-Fi credentials");
  Serial.println("  otacheck   - trigger immediate OTA check");
}

void serialDebugHandleCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();

  if (cmd.isEmpty()) {
    return;
  }

  if (cmd == "help" || cmd == "commands" || cmd == "?") {
    serialDebugPrintCommandHelp();
    return;
  }

  if (cmd == "clearwifi") {
    systemStateClearWifiCredentials();
    Serial.println("Saved Wi-Fi credentials cleared.");
    Serial.println("You can now test setup mode and enter new credentials.");
    return;
  }

  if (cmd == "otacheck") {
    otaUpdateRequestCheckNow();
    Serial.println("OTA check requested.");
    return;
  }

  Serial.print("Unknown command: ");
  Serial.println(cmd);
  serialDebugPrintCommandHelp();
}

}  // namespace

void serialDebugBegin(uint32_t baudRate) {
  Serial.begin(baudRate);
  Serial.println("Serial debug ready. Type 'help' for commands.");
  serialDebugPrintCommandHelp();
}

void serialDebugPrintNetworkStartup(const String &apSsid, bool staConnected, bool apEnabled, const String &savedSsid) {
  if (apEnabled) {
    Serial.println("Setup AP mode active");
    Serial.print("SSID: ");
    Serial.println(apSsid);
    Serial.println("Password: (open)");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Setup AP mode disabled (STA connected)");
  }

  if (staConnected) {
    Serial.print("Connected to Wi-Fi: ");
    Serial.println(savedSsid);
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("No STA connection yet. Use web setup page to configure Wi-Fi.");
  }
}

void serialDebugPrintTaskStartup() {
  Serial.println("Tasks started: webTask on Core 0, ledTask on Core 1");
}

void serialDebugPollCommands() {
  bool gotByte = false;

  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    gotByte = true;
    g_lastCommandByteMs = millis();

    if (c == '\r' || c == '\n') {
      serialDebugHandleCommand(g_serialCommandBuffer);
      g_serialCommandBuffer = "";
      continue;
    }

    if (c == '\0') {
      continue;
    }

    g_serialCommandBuffer += c;
    if (g_serialCommandBuffer.length() > 64) {
      g_serialCommandBuffer = "";
      Serial.println("Command too long. Cleared input buffer.");
    }
  }

  // Some terminal integrations send framed text without newline.
  if (!gotByte && !g_serialCommandBuffer.isEmpty()) {
    const uint32_t now = millis();
    if (now - g_lastCommandByteMs > 120) {
      serialDebugHandleCommand(g_serialCommandBuffer);
      g_serialCommandBuffer = "";
    }
  }
}
