#pragma once

const char CONFIG_PAGE_HTML[] PROGMEM = R"HTML(
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
    .topbar {
      background: rgba(255, 255, 255, 0.72);
      border: 1px solid rgba(39, 33, 29, 0.12);
      border-radius: 14px;
      padding: 10px;
      backdrop-filter: blur(6px);
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
    }
    .nav {
      border: 1px solid rgba(39, 33, 29, 0.12);
      border-radius: 10px;
      padding: 8px 12px;
      text-decoration: none;
      color: var(--ink);
      font-weight: 700;
      font-size: 13px;
      background: #ffffff;
    }
    .nav.active {
      border-color: rgba(249, 115, 22, 0.35);
      background: rgba(249, 115, 22, 0.14);
      color: #c2410c;
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
    .lan { background: #2563eb; color: #fff; }
    .settings { background: #0f766e; color: #fff; }
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
    <nav class="topbar">
      <a class="nav active" href="/setup">Setup</a>
      <a class="nav" href="/home">Home</a>
      <a class="nav" href="/effects">Effects</a>
      <a class="nav" href="/settings">Settings</a>
    </nav>

    <section class="card">
      <h1>ESP32 LED Controller</h1>
      <div class="meta">
        <div>Mode: <span id="mode">-</span></div>
        <div>AP IP: <span id="apIp">-</span></div>
        <div>STA IP: <span id="staIp">-</span></div>
        <div>Wi-Fi: <span id="ssid">-</span><span class="badge" id="conn">...</span></div>
        <div>Internet: <span id="internet">-</span></div>
        <div>Status LED: <span id="statusLed">-</span></div>
      </div>
      <div class="controls">
        <button class="reboot" onclick="reboot()">Restart</button>
        <button class="settings" onclick="openSettings()">Settings</button>
        <button id="lanBtn" class="lan" style="display:none;" onclick="copyLanUrl()">Copy LAN URL</button>
      </div>
    </section>

    <section class="card">
      <h2>Wi-Fi Setup</h2>
      <form id="deviceForm">
        <input id="deviceName" name="name" placeholder="Device name" maxlength="24" required />
        <div class="controls">
          <button class="save" type="submit">Save Device Name</button>
        </div>
      </form>

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
    let handoffAttempted = false;
    let lastStatusRaw = '';
    let statusInFlight = false;

    function setTextIfChanged(id, value) {
      const el = document.getElementById(id);
      if (el.textContent !== value) {
        el.textContent = value;
      }
    }

    function getHostFromUrl(url) {
      try {
        return new URL(url).hostname;
      } catch (_) {
        return '';
      }
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

        const lanBtn = document.getElementById('lanBtn');
        const lanUrl = s.lan_url || '';

        if (s.connected && lanUrl.length > 0) {
          if (lanBtn.style.display !== 'inline-block') {
            lanBtn.style.display = 'inline-block';
          }
          lanBtn.dataset.url = lanUrl;
          setTextIfChanged('lanBtn', 'Copy LAN URL');

          // If AP has shut down and station is active, attempt seamless handoff.
          if (s.ap_enabled === false && !handoffAttempted) {
            const currentHost = window.location.hostname;
            const targetHost = getHostFromUrl(lanUrl);
            const onApHost = currentHost === (s.ap_ip || '');

            // Only hand off if still on AP host and target is different.
            if (onApHost && targetHost.length > 0 && targetHost !== currentHost) {
              handoffAttempted = true;
              const m = document.getElementById('msg');
              m.textContent = `Switching to LAN page: ${lanUrl}`;
              window.location.href = lanUrl;
            }
          }
        } else {
          if (lanBtn.style.display !== 'none') {
            lanBtn.style.display = 'none';
          }
          lanBtn.dataset.url = '';
          handoffAttempted = false;
        }

        if (raw === lastStatusRaw) {
          return;
        }

        lastStatusRaw = raw;

        setTextIfChanged('mode', s.mode || '-');
        setTextIfChanged('apIp', s.ap_ip || '-');
        setTextIfChanged('staIp', s.sta_ip || '-');
        setTextIfChanged('ssid', s.ssid || '(not set)');
        setTextIfChanged('conn', s.connected ? 'connected' : 'offline');
        setTextIfChanged('internet', s.internet ? 'online' : 'offline');
        setTextIfChanged('statusLed', s.status_led || '-');

        const ssidInput = document.getElementById('wifiSsid');
        const passInput = document.getElementById('wifiPass');
        const deviceInput = document.getElementById('deviceName');
        const editingForm = document.activeElement === ssidInput
          || document.activeElement === passInput
          || document.activeElement === deviceInput;

        // Avoid clobbering user typing when status polling runs.
        if (!editingForm) {
          const nextSsid = s.ssid || '';
          if (ssidInput.value !== nextSsid) {
            ssidInput.value = nextSsid;
          }

          const nextDevice = s.device_name || 'Device 1';
          if (deviceInput.value !== nextDevice) {
            deviceInput.value = nextDevice;
          }
        }
      } finally {
        statusInFlight = false;
      }
    }

    async function reboot() {
      await fetch('/api/restart', { method: 'POST' });
      const m = document.getElementById('msg');
      m.textContent = 'Restarting device...';
    }

    async function copyLanUrl() {
      const lanBtn = document.getElementById('lanBtn');
      const lanUrl = lanBtn.dataset.url || '';
      const m = document.getElementById('msg');
      if (lanUrl.length > 0) {
        try {
          if (navigator.clipboard && navigator.clipboard.writeText) {
            await navigator.clipboard.writeText(lanUrl);
            m.textContent = `Copied LAN URL: ${lanUrl}`;
            return;
          }
        } catch (_) {
          // Ignore and fall back below.
        }

        const manual = prompt('Copy this LAN URL:', lanUrl);
        if (manual !== null) {
          m.textContent = `LAN URL ready to copy: ${lanUrl}`;
        }
      }
    }

    function openSettings() {
      window.location.href = '/settings';
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

    document.getElementById('deviceForm').addEventListener('submit', async (e) => {
      e.preventDefault();
      const name = document.getElementById('deviceName').value.trim();
      const m = document.getElementById('msg');
      const body = new URLSearchParams({ name });
      const res = await fetch('/api/device', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });
      const text = await res.text();
      m.textContent = text;
      fetchStatus();
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
