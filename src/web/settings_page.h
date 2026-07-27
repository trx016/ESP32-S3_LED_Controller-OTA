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
    .topbar {
      background: rgba(255, 255, 255, 0.72);
      border: 1px solid rgba(28, 46, 40, 0.12);
      border-radius: 14px;
      padding: 10px;
      backdrop-filter: blur(6px);
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
    }
    .nav {
      border: 1px solid rgba(28, 46, 40, 0.12);
      border-radius: 10px;
      padding: 8px 12px;
      text-decoration: none;
      color: var(--ink);
      font-weight: 700;
      font-size: 13px;
      background: #ffffff;
    }
    .nav.active {
      border-color: rgba(15, 118, 110, 0.35);
      background: rgba(15, 118, 110, 0.14);
      color: #0f766e;
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
    .numberInput {
      width: 140px;
      border-radius: 8px;
      border: 1px solid rgba(28, 46, 40, 0.25);
      padding: 8px;
      font: inherit;
      font-weight: 700;
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
    <nav class="topbar">
      <a class="nav" href="/setup">Setup</a>
      <a class="nav" href="/home">Home</a>
      <a class="nav" href="/effects">Effects</a>
      <a class="nav active" href="/settings">Settings</a>
    </nav>

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

      <div class="switchRow">
        <div>
          <div class="switchLabel">Enable Internet Connectivity</div>
          <div class="hint">Disable this to keep the device on local-only operation (no internet probes or OTA checks).</div>
        </div>
        <input class="toggle" id="internetEnabled" type="checkbox" />
      </div>

      <div class="switchRow">
        <div>
          <div class="switchLabel">Active LED Count</div>
          <div class="hint">Set how many LEDs are actively rendered by all effects.</div>
        </div>
        <input class="numberInput" id="ledCount" type="number" min="1" step="1" />
      </div>

      <div class="switchRow">
        <div>
          <div class="switchLabel">Global Effects FPS</div>
          <div class="hint">Shared frame-rate cap applied to all effects.</div>
        </div>
        <input class="numberInput" id="effectsFps" type="number" min="15" step="1" />
      </div>

      <div class="meta">
        <div>Current firmware: <strong id="otaCurrent">-</strong></div>
        <div>Latest release: <strong id="otaLatest">-</strong></div>
        <div>OTA status: <strong id="otaStatus">-</strong></div>
        <div>Max effects FPS (current LEDs): <strong id="fpsMax">-</strong></div>
      </div>

      <div class="row">
        <button onclick="saveSettings()">Save Settings</button>
        <button class="alt" onclick="checkOtaNow()">Check OTA Now</button>
        <button id="installBtn" onclick="installOtaNow()" style="display:none;">Install Update Now</button>
        <a class="btn alt" id="backBtn" href="/">Back</a>
      </div>

      <p class="msg" id="msg"></p>
    </section>
  </main>

  <script>
    let latestConnected = false;
    let lastStatusRaw = '';
    let statusInFlight = false;

    function setTextIfChanged(id, value) {
      const el = document.getElementById(id);
      if (el.textContent !== value) {
        el.textContent = value;
      }
    }

    function setCheckedIfChanged(id, checked) {
      const el = document.getElementById(id);
      if (el.checked !== checked) {
        el.checked = checked;
      }
    }

    function setValueIfChanged(id, value) {
      const el = document.getElementById(id);
      if (el.value !== value) {
        el.value = value;
      }
    }

    function setWifiPill(connected) {
      const pill = document.getElementById('wifiPill');
      const text = connected ? 'Wi-Fi: connected' : 'Wi-Fi: setup mode';
      const cls = connected ? 'pill ok' : 'pill';
      if (pill.textContent !== text) {
        pill.textContent = text;
      }
      if (pill.className !== cls) {
        pill.className = cls;
      }
      document.getElementById('backBtn').setAttribute('href', connected ? '/home' : '/setup');
    }

    async function fetchStatus() {
      if (statusInFlight) {
        return;
      }

      statusInFlight = true;
      try {
        const res = await fetch('/api/status');
        const raw = await res.text();
        if (!res.ok) {
          return;
        }

        const s = JSON.parse(raw);
        if (raw === lastStatusRaw) {
          return;
        }

        lastStatusRaw = raw;

        const autoOtaEl = document.getElementById('autoOta');
        const internetEl = document.getElementById('internetEnabled');
        const ledCountEl = document.getElementById('ledCount');
        const effectsFpsEl = document.getElementById('effectsFps');
        const editingControls =
          document.activeElement === autoOtaEl ||
          document.activeElement === internetEl ||
          document.activeElement === ledCountEl ||
          document.activeElement === effectsFpsEl;

      latestConnected = !!s.connected;
      setWifiPill(latestConnected);

        if (!editingControls) {
          setCheckedIfChanged('autoOta', !!s.auto_ota);
          setCheckedIfChanged('internetEnabled', (s.internet_enabled !== false));
          setValueIfChanged('ledCount', String(s.led_count || 1));
          setValueIfChanged('effectsFps', String(s.effects_fps || 15));
        }

        const ledCountMax = String(s.led_count_max || 1);
        if (ledCountEl.max !== ledCountMax) {
          ledCountEl.max = ledCountMax;
        }

        const fpsMax = String(s.effects_fps_max || 15);
        if (effectsFpsEl.max !== fpsMax) {
          effectsFpsEl.max = fpsMax;
        }

        setTextIfChanged('otaCurrent', s.ota_current || '-');
        setTextIfChanged('otaLatest', s.ota_latest || '-');
        setTextIfChanged('otaStatus', s.ota_status || '-');
        setTextIfChanged('fpsMax', String(s.effects_fps_max || '-'));

      const installBtn = document.getElementById('installBtn');
      const manualMode = !s.auto_ota;
      const hasUpdate = !!s.ota_update_available;
        const nextDisplay = (manualMode && hasUpdate) ? 'inline-block' : 'none';
        if (installBtn.style.display !== nextDisplay) {
          installBtn.style.display = nextDisplay;
        }
      } finally {
        statusInFlight = false;
      }
    }

    async function saveSettings() {
      const otaEnabled = document.getElementById('autoOta').checked ? '1' : '0';
      const internetEnabled = document.getElementById('internetEnabled').checked ? '1' : '0';
      const ledCount = document.getElementById('ledCount').value || '1';
      const effectsFps = document.getElementById('effectsFps').value || '15';

      const otaRes = await fetch('/api/settings/ota', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ enabled: otaEnabled })
      });

      const internetRes = await fetch('/api/settings/internet', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ enabled: internetEnabled })
      });

      const ledRes = await fetch('/api/settings/leds', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ count: ledCount })
      });

      const fpsRes = await fetch('/api/settings/effects-fps', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ fps: effectsFps })
      });

      const otaText = await otaRes.text();
      const internetText = await internetRes.text();
      const ledText = await ledRes.text();
      const fpsText = await fpsRes.text();
      const msg = document.getElementById('msg');
      msg.textContent = `${otaText} ${internetText} ${ledText} ${fpsText}`;

      if (otaRes.ok && internetRes.ok && ledRes.ok && fpsRes.ok) {
        setWifiPill(latestConnected);
      }

      fetchStatus();
    }

    async function checkOtaNow() {
      const msg = document.getElementById('msg');
      const res = await fetch('/api/ota/check', { method: 'POST' });
      msg.textContent = await res.text();
      fetchStatus();
    }

    async function installOtaNow() {
      const latest = document.getElementById('otaLatest').textContent || 'new release';
      const ok = confirm(`Install firmware update ${latest}? Device will reboot after successful update.`);
      if (!ok) {
        return;
      }

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
