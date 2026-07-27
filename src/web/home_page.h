#pragma once

const char HOME_PAGE_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>LED Controller Home</title>
  <style>
    :root {
      --sky-a: #d7f0ff;
      --sky-b: #f2fbff;
      --card: #ffffff;
      --ink: #132131;
      --brand: #0284c7;
      --ok: #16a34a;
      --warn: #f59e0b;
      --muted: #4b5563;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      font-family: "Segoe UI", "Trebuchet MS", sans-serif;
      color: var(--ink);
      background:
        radial-gradient(circle at 20% 10%, rgba(2, 132, 199, 0.22), transparent 38%),
        radial-gradient(circle at 88% 78%, rgba(22, 163, 74, 0.16), transparent 34%),
        linear-gradient(145deg, var(--sky-a), var(--sky-b));
      display: grid;
      place-items: center;
      padding: 18px;
    }
    .shell {
      width: min(760px, 100%);
      display: grid;
      gap: 14px;
    }
    .topbar {
      background: rgba(255, 255, 255, 0.72);
      border: 1px solid rgba(19, 33, 49, 0.10);
      border-radius: 14px;
      padding: 10px;
      backdrop-filter: blur(6px);
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
    }
    .nav {
      border: 1px solid rgba(19, 33, 49, 0.10);
      border-radius: 10px;
      padding: 8px 12px;
      text-decoration: none;
      color: var(--ink);
      font-weight: 700;
      font-size: 13px;
      background: #ffffff;
    }
    .nav.active {
      border-color: rgba(2, 132, 199, 0.35);
      background: rgba(2, 132, 199, 0.14);
      color: #075985;
    }
    .card {
      background: var(--card);
      border-radius: 16px;
      border: 1px solid rgba(19, 33, 49, 0.09);
      padding: 18px;
      box-shadow: 0 14px 28px rgba(19, 33, 49, 0.12);
      animation: fade-up .35s ease both;
    }
    .card:nth-child(2) { animation-delay: .08s; }
    @keyframes fade-up {
      from { opacity: 0; transform: translateY(10px); }
      to { opacity: 1; transform: translateY(0); }
    }
    h1 { margin: 0 0 8px; }
    p { margin: 0; color: var(--muted); }
    .grid {
      margin-top: 14px;
      display: grid;
      gap: 8px;
      grid-template-columns: repeat(auto-fit, minmax(210px, 1fr));
    }
    .tile {
      border: 1px solid rgba(19, 33, 49, 0.09);
      border-radius: 12px;
      padding: 10px;
      background: #f9fdff;
    }
    .label {
      font-size: 12px;
      text-transform: uppercase;
      letter-spacing: 0.06em;
      color: var(--muted);
      font-weight: 700;
    }
    .value {
      margin-top: 4px;
      font-size: 16px;
      font-weight: 700;
      word-break: break-word;
    }
    .row {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      margin-top: 14px;
      align-items: center;
    }
    .pill {
      display: inline-block;
      padding: 4px 10px;
      border-radius: 999px;
      font-size: 12px;
      font-weight: 700;
      background: rgba(2, 132, 199, 0.12);
      color: #075985;
    }
    .ok { background: rgba(22, 163, 74, 0.18); color: #166534; }
    .warn { background: rgba(245, 158, 11, 0.18); color: #92400e; }
    button, a.btn {
      border: 0;
      border-radius: 12px;
      padding: 10px 14px;
      font-size: 14px;
      font-weight: 700;
      cursor: pointer;
      text-decoration: none;
      color: #fff;
      background: var(--brand);
      transition: transform .12s ease, filter .12s ease;
    }
    button:hover, a.btn:hover { transform: translateY(-1px); filter: brightness(1.03); }
    button:active, a.btn:active { transform: translateY(0); }
    .subtle { background: #475569; }
    .msg {
      min-height: 20px;
      margin-top: 8px;
      font-size: 13px;
      font-weight: 600;
      color: #1d4ed8;
    }
  </style>
</head>
<body>
  <main class="shell">
    <nav class="topbar">
      <a class="nav" href="/setup">Setup</a>
      <a class="nav active" href="/home">Home</a>
      <a class="nav" href="/effects">Effects</a>
      <a class="nav" href="/settings">Settings</a>
    </nav>

    <section class="card">
      <h1>LED Controller Home</h1>
      <p>Device is connected. This page is shown during normal operation.</p>
      <div class="row">
        <span class="pill" id="wifiPill">Wi-Fi: checking</span>
        <span class="pill" id="netPill">Internet: checking</span>
      </div>
      <div class="grid">
        <div class="tile"><div class="label">Device</div><div class="value" id="deviceName">-</div></div>
        <div class="tile"><div class="label">Mode</div><div class="value" id="mode">-</div></div>
        <div class="tile"><div class="label">Wi-Fi SSID</div><div class="value" id="ssid">-</div></div>
        <div class="tile"><div class="label">LAN IP</div><div class="value" id="lanIp">-</div></div>
        <div class="tile"><div class="label">Status LED</div><div class="value" id="statusLed">-</div></div>
      </div>
      <p class="msg" id="msg"></p>
    </section>

    <section class="card">
      <div class="row">
        <a class="btn" href="/setup">Open Wi-Fi Setup</a>
        <a class="btn" href="/effects">Effects</a>
        <a class="btn subtle" href="/settings">Settings</a>
        <button class="subtle" onclick="reboot()">Restart Device</button>
      </div>
    </section>
  </main>

  <script>
    let lastStatusRaw = '';
    let refreshInFlight = false;

    function setTextIfChanged(id, value) {
      const el = document.getElementById(id);
      if (el.textContent !== value) {
        el.textContent = value;
      }
    }

    function setPill(id, label, ok) {
      const el = document.getElementById(id);
      const nextClass = ok ? 'pill ok' : 'pill warn';
      if (el.textContent !== label) {
        el.textContent = label;
      }
      if (el.className !== nextClass) {
        el.className = nextClass;
      }
    }

    async function refreshStatus() {
      if (refreshInFlight) {
        return;
      }

      refreshInFlight = true;
      try {
        const res = await fetch('/api/status');
        const raw = await res.text();
        if (!res.ok) {
          return;
        }

        const s = JSON.parse(raw);

        if (!s.connected) {
          window.location.href = '/setup';
          return;
        }

        if (raw === lastStatusRaw) {
          return;
        }

        lastStatusRaw = raw;

        setTextIfChanged('deviceName', s.device_name || 'Device');
        setTextIfChanged('mode', s.mode || '-');
        setTextIfChanged('ssid', s.ssid || '(not set)');
        setTextIfChanged('lanIp', s.sta_ip || '-');
        setTextIfChanged('statusLed', s.status_led || '-');

        setPill('wifiPill', s.connected ? 'Wi-Fi: connected' : 'Wi-Fi: offline', !!s.connected);
        setPill('netPill', s.internet ? 'Internet: online' : 'Internet: offline', !!s.internet);
      } finally {
        refreshInFlight = false;
      }
    }

    async function reboot() {
      await fetch('/api/restart', { method: 'POST' });
      document.getElementById('msg').textContent = 'Restarting device...';
    }

    refreshStatus();
    setInterval(refreshStatus, 3000);
  </script>
</body>
</html>
)HTML";
