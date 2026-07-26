#pragma once

const char SETTINGS_PAGE_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>LED Controller Settings</title>
  <style>
    :root {
      --bg1: #e8f8f4;
      --bg2: #dbf3ef;
      --card: #ffffff;
      --ink: #1c2e28;
      --accent: #0f766e;
      --accent-2: #0ea5e9;
      --ok: #16a34a;
      --warn: #f59e0b;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      font-family: "Trebuchet MS", "Segoe UI", sans-serif;
      color: var(--ink);
      background:
        radial-gradient(circle at 16% 14%, rgba(15, 118, 110, 0.22), transparent 34%),
        radial-gradient(circle at 86% 82%, rgba(14, 165, 233, 0.16), transparent 34%),
        linear-gradient(160deg, var(--bg1), var(--bg2));
      display: grid;
      place-items: center;
      padding: 18px;
    }
    .wrap {
      width: min(680px, 100%);
      display: grid;
      gap: 14px;
    }
    .card {
      background: var(--card);
      border-radius: 16px;
      border: 1px solid rgba(28, 46, 40, 0.10);
      box-shadow: 0 14px 30px rgba(28, 46, 40, 0.10);
      padding: 18px;
    }
    h1 { margin: 0 0 8px; }
    p { margin: 0; }
    .row {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      align-items: center;
      margin-top: 14px;
    }
    .switchRow {
      margin-top: 12px;
      border: 1px solid rgba(28, 46, 40, 0.12);
      border-radius: 12px;
      padding: 12px;
      background: #f7fffd;
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
    }
    .switchLabel {
      font-size: 15px;
      font-weight: 700;
    }
    .hint {
      margin-top: 6px;
      font-size: 13px;
      opacity: 0.85;
    }
    .toggle {
      width: 22px;
      height: 22px;
      accent-color: var(--accent);
      cursor: pointer;
    }
    button, a.btn {
      border: 0;
      border-radius: 12px;
      padding: 10px 14px;
      font-size: 14px;
      font-weight: 700;
      cursor: pointer;
      text-decoration: none;
      color: #fff;
      background: var(--accent);
      transition: transform .12s ease, filter .12s ease;
    }
    button:hover, a.btn:hover { transform: translateY(-1px); filter: brightness(1.03); }
    button:active, a.btn:active { transform: translateY(0); }
    .alt { background: var(--accent-2); }
    .pill {
      display: inline-block;
      padding: 4px 10px;
      border-radius: 999px;
      font-size: 12px;
      font-weight: 700;
      background: rgba(245, 158, 11, 0.2);
      color: #92400e;
    }
    .pill.ok {
      background: rgba(22, 163, 74, 0.18);
      color: #166534;
    }
    .msg {
      min-height: 20px;
      margin-top: 8px;
      font-size: 13px;
      font-weight: 700;
      color: #0f766e;
    }
    .meta {
      margin-top: 12px;
      display: grid;
      gap: 6px;
      font-size: 13px;
    }
  </style>
</head>
<body>
  <main class="wrap">
    <section class="card">
      <h1>Device Settings</h1>
      <p>Configure persistent behavior for this controller.</p>

      <div class="row">
        <span class="pill" id="wifiPill">Wi-Fi: checking</span>
      </div>

      <div class="switchRow">
        <div>
          <div class="switchLabel">Automatic OTA Updates</div>
          <div class="hint">If enabled, firmware can auto-apply OTA logic when that service is active.</div>
        </div>
        <input class="toggle" id="autoOta" type="checkbox" />
      </div>

      <div class="meta">
        <div>Current firmware: <strong id="otaCurrent">-</strong></div>
        <div>Latest release: <strong id="otaLatest">-</strong></div>
        <div>OTA status: <strong id="otaStatus">-</strong></div>
      </div>

      <div class="row">
        <button onclick="saveSettings()">Save Settings</button>
        <button class="alt" onclick="checkOtaNow()">Check OTA Now</button>
        <button onclick="installOtaNow()">Install Update Now</button>
        <a class="btn alt" id="backBtn" href="/">Back</a>
      </div>

      <p class="msg" id="msg"></p>
    </section>
  </main>

  <script>
    let latestConnected = false;

    function setWifiPill(connected) {
      const pill = document.getElementById('wifiPill');
      pill.textContent = connected ? 'Wi-Fi: connected' : 'Wi-Fi: setup mode';
      pill.className = connected ? 'pill ok' : 'pill';
      document.getElementById('backBtn').setAttribute('href', connected ? '/home' : '/setup');
    }

    async function fetchStatus() {
      const res = await fetch('/api/status');
      const s = await res.json();
      latestConnected = !!s.connected;
      setWifiPill(latestConnected);
      document.getElementById('autoOta').checked = !!s.auto_ota;
      document.getElementById('otaCurrent').textContent = s.ota_current || '-';
      document.getElementById('otaLatest').textContent = s.ota_latest || '-';
      document.getElementById('otaStatus').textContent = s.ota_status || '-';
    }

    async function saveSettings() {
      const enabled = document.getElementById('autoOta').checked ? '1' : '0';
      const body = new URLSearchParams({ enabled });
      const res = await fetch('/api/settings/ota', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });

      const text = await res.text();
      const msg = document.getElementById('msg');
      msg.textContent = text;

      if (res.ok) {
        setWifiPill(latestConnected);
      }
    }

    async function checkOtaNow() {
      const msg = document.getElementById('msg');
      const res = await fetch('/api/ota/check', { method: 'POST' });
      msg.textContent = await res.text();
      fetchStatus();
    }

    async function installOtaNow() {
      const msg = document.getElementById('msg');
      const res = await fetch('/api/ota/install', { method: 'POST' });
      msg.textContent = await res.text();
      fetchStatus();
    }

    fetchStatus();
    setInterval(fetchStatus, 3000);
  </script>
</body>
</html>
)HTML";
