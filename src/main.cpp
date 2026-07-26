#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

static const uint8_t LED_PIN = LED_BUILTIN;
static const char *AP_SSID_PREFIX = "LED-Controller-Setup";
static const char *AP_PASS = "12345678";

Preferences preferences;
WebServer server(80);
DNSServer dnsServer;

String savedSsid;
String savedPass;
String deviceName;
String apSsid;
bool ledState = false;
bool staConnected = false;

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>LED Controller</title>
  <style>
    :root {
      --bg1: #fdf6e3;
      --bg2: #f8e8c8;
      --card: #fffaf0;
      --ink: #27211d;
      --accent: #f97316;
      --accent-2: #0ea5a3;
      --danger: #dc2626;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      font-family: "Trebuchet MS", "Segoe UI", sans-serif;
      background:
        radial-gradient(circle at 10% 20%, rgba(249, 115, 22, 0.28), transparent 35%),
        radial-gradient(circle at 90% 80%, rgba(14, 165, 163, 0.24), transparent 35%),
        linear-gradient(160deg, var(--bg1), var(--bg2));
      color: var(--ink);
      display: grid;
      place-items: center;
      padding: 20px;
    }
    .wrap {
      width: min(720px, 100%);
      display: grid;
      gap: 16px;
    }
    .card {
      background: var(--card);
      border: 2px solid rgba(0,0,0,0.07);
      border-radius: 16px;
      padding: 18px;
      box-shadow: 0 14px 30px rgba(39, 33, 29, 0.12);
      animation: rise .4s ease both;
    }
    .card:nth-child(2) { animation-delay: .08s; }
    @keyframes rise {
      from { opacity: 0; transform: translateY(10px); }
      to { opacity: 1; transform: translateY(0); }
    }
    h1, h2 { margin: 0 0 10px; }
    p { margin: 0; }
    .meta {
      display: grid;
      gap: 6px;
      margin-bottom: 14px;
      font-size: 14px;
    }
    .badge {
      display: inline-block;
      padding: 4px 10px;
      border-radius: 999px;
      background: rgba(249, 115, 22, 0.18);
      font-weight: 700;
      font-size: 12px;
      margin-left: 6px;
    }
    .controls {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      align-items: center;
    }
    button {
      border: 0;
      border-radius: 12px;
      padding: 11px 16px;
      font-size: 15px;
      font-weight: 700;
      cursor: pointer;
      transition: transform .12s ease, filter .12s ease;
    }
    button:hover { transform: translateY(-1px); filter: brightness(1.02); }
    button:active { transform: translateY(0); }
    .on { background: var(--accent); color: #fff; }
    .off { background: #ef4444; color: #fff; }
    .reboot { background: var(--accent-2); color: #fff; }
    .state {
      font-weight: 800;
      letter-spacing: 0.3px;
    }
    form {
      display: grid;
      gap: 10px;
      margin-top: 6px;
    }
    input {
      width: 100%;
      border: 1px solid rgba(39,33,29,0.2);
      border-radius: 10px;
      padding: 10px 12px;
      font-size: 14px;
      background: #fff;
      color: var(--ink);
    }
    .save { background: #16a34a; color: #fff; }
    .clear { background: var(--danger); color: #fff; }
    .msg {
      min-height: 20px;
      font-size: 14px;
      font-weight: 600;
    }
  </style>
</head>
<body>
  <div class="wrap">
    <section class="card">
      <h1>ESP32 LED Controller</h1>
      <div class="meta">
        <div>Mode: <span id="mode">-</span></div>
        <div>AP IP: <span id="apIp">-</span></div>
        <div>STA IP: <span id="staIp">-</span></div>
        <div>Wi-Fi: <span id="ssid">-</span><span class="badge" id="conn">...</span></div>
      </div>
      <div class="controls">
        <button class="on" onclick="setLed('on')">LED ON</button>
        <button class="off" onclick="setLed('off')">LED OFF</button>
        <button class="reboot" onclick="reboot()">Restart</button>
        <span class="state">LED State: <span id="led">-</span></span>
      </div>
    </section>

    <section class="card">
      <h2>Wi-Fi Setup</h2>
      <form id="wifiForm">
        <input id="wifiSsid" name="ssid" placeholder="Wi-Fi SSID" required />
        <input id="wifiPass" name="pass" placeholder="Wi-Fi Password" type="password" />
        <div class="controls">
          <button class="save" type="submit">Save & Connect</button>
          <button class="clear" type="button" onclick="clearWifi()">Clear Saved Wi-Fi</button>
        </div>
      </form>
      <p class="msg" id="msg"></p>
    </section>
  </div>

  <script>
    async function fetchStatus() {
      const res = await fetch('/api/status');
      const s = await res.json();
      document.getElementById('mode').textContent = s.mode;
      document.getElementById('apIp').textContent = s.ap_ip;
      document.getElementById('staIp').textContent = s.sta_ip;
      document.getElementById('ssid').textContent = s.ssid || '(not set)';
      document.getElementById('conn').textContent = s.connected ? 'connected' : 'offline';
      document.getElementById('led').textContent = s.led ? 'ON' : 'OFF';
      document.getElementById('wifiSsid').value = s.ssid || '';
    }

    async function setLed(state) {
      await fetch('/api/led', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `state=${encodeURIComponent(state)}`
      });
      fetchStatus();
    }

    async function reboot() {
      await fetch('/api/restart', { method: 'POST' });
      const m = document.getElementById('msg');
      m.textContent = 'Restarting device...';
    }

    document.getElementById('wifiForm').addEventListener('submit', async (e) => {
      e.preventDefault();
      const ssid = document.getElementById('wifiSsid').value.trim();
      const pass = document.getElementById('wifiPass').value;
      const m = document.getElementById('msg');
      const body = new URLSearchParams({ ssid, pass });
      const res = await fetch('/api/wifi', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });
      const text = await res.text();
      m.textContent = text;
    });

    async function clearWifi() {
      const m = document.getElementById('msg');
      const res = await fetch('/api/wifi/clear', { method: 'POST' });
      m.textContent = await res.text();
      fetchStatus();
    }

    fetchStatus();
    setInterval(fetchStatus, 3000);
  </script>
</body>
</html>
)HTML";

String wifiStatusJson() {
  const bool connected = WiFi.status() == WL_CONNECTED;
  const String mode = connected ? "AP+STA" : "AP_ONLY";
  const String apIp = WiFi.softAPIP().toString();
  const String staIp = connected ? WiFi.localIP().toString() : "-";
  String json = "{";
  json += "\"mode\":\"" + mode + "\",";
  json += "\"connected\":" + String(connected ? "true" : "false") + ",";
  json += "\"ap_ip\":\"" + apIp + "\",";
  json += "\"ap_ssid\":\"" + apSsid + "\",";
  json += "\"device_name\":\"" + deviceName + "\",";
  json += "\"sta_ip\":\"" + staIp + "\",";
  json += "\"ssid\":\"" + savedSsid + "\",";
  json += "\"led\":" + String(ledState ? "true" : "false");
  json += "}";
  return json;
}

void applyLed() {
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
}

void loadWifiCredentials() {
  preferences.begin("wifi", true);
  savedSsid = preferences.getString("ssid", "");
  savedPass = preferences.getString("pass", "");
  preferences.end();
}

void saveWifiCredentials(const String &ssid, const String &pass) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);
  preferences.end();
  savedSsid = ssid;
  savedPass = pass;
}

void clearWifiCredentials() {
  preferences.begin("wifi", false);
  preferences.remove("ssid");
  preferences.remove("pass");
  preferences.end();
  savedSsid = "";
  savedPass = "";
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

bool connectToSavedWifi(uint32_t timeoutMs = 15000) {
  if (savedSsid.isEmpty()) {
    staConnected = false;
    return false;
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(savedSsid.c_str(), savedPass.c_str());

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250);
  }

  staConnected = WiFi.status() == WL_CONNECTED;
  return staConnected;
}

void startSetupAccessPoint() {
  WiFi.mode(WIFI_AP_STA);
  apSsid = buildApSsid();
  WiFi.softAP(apSsid.c_str(), AP_PASS);
  dnsServer.start(53, "*", WiFi.softAPIP());
}

void sendPortalRedirect() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleCaptivePortalProbe() {
  sendPortalRedirect();
}

void handleStatus() {
  server.send(200, "application/json", wifiStatusJson());
}

void handleLedControl() {
  if (!server.hasArg("state")) {
    server.send(400, "text/plain", "Missing state=on|off");
    return;
  }

  const String state = server.arg("state");
  if (state == "on") {
    ledState = true;
  } else if (state == "off") {
    ledState = false;
  } else {
    server.send(400, "text/plain", "State must be on or off");
    return;
  }

  applyLed();
  server.send(200, "application/json", wifiStatusJson());
}

void handleWifiSave() {
  if (!server.hasArg("ssid")) {
    server.send(400, "text/plain", "Missing SSID");
    return;
  }

  const String ssid = server.arg("ssid");
  const String pass = server.hasArg("pass") ? server.arg("pass") : "";

  if (ssid.isEmpty()) {
    server.send(400, "text/plain", "SSID cannot be empty");
    return;
  }

  saveWifiCredentials(ssid, pass);
  server.send(200, "text/plain", "Saved Wi-Fi credentials. Restarting...");
  delay(400);
  ESP.restart();
}

void handleWifiClear() {
  clearWifiCredentials();
  WiFi.disconnect(true, false);
  staConnected = false;
  server.send(200, "text/plain", "Saved Wi-Fi cleared.");
}

void handleRestart() {
  server.send(200, "text/plain", "Restarting device...");
  delay(300);
  ESP.restart();
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/generate_204", HTTP_GET, handleCaptivePortalProbe);
  server.on("/gen_204", HTTP_GET, handleCaptivePortalProbe);
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptivePortalProbe);
  server.on("/library/test/success.html", HTTP_GET, handleCaptivePortalProbe);
  server.on("/connecttest.txt", HTTP_GET, handleCaptivePortalProbe);
  server.on("/ncsi.txt", HTTP_GET, handleCaptivePortalProbe);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/led", HTTP_POST, handleLedControl);
  server.on("/api/wifi", HTTP_POST, handleWifiSave);
  server.on("/api/wifi/clear", HTTP_POST, handleWifiClear);
  server.on("/api/restart", HTTP_POST, handleRestart);
  server.onNotFound([]() {
    sendPortalRedirect();
  });
  server.begin();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  applyLed();

  loadDeviceName();
  startSetupAccessPoint();
  loadWifiCredentials();
  const bool connected = connectToSavedWifi();

  Serial.println("Started setup AP mode");
  Serial.print("SSID: ");
  Serial.println(apSsid);
  Serial.print("Password: ");
  Serial.println(AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  if (connected) {
    Serial.print("Connected to Wi-Fi: ");
    Serial.println(savedSsid);
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("No STA connection yet. Use web setup page to configure Wi-Fi.");
  }

  setupWebServer();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}