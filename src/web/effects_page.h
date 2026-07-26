#pragma once

const char EFFECTS_PAGE_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>LED Effects</title>
  <style>
    :root {
      --ink: #12212f;
      --card: #ffffff;
      --bg-a: #e9f3ff;
      --bg-b: #fff2de;
      --accent: #c2410c;
      --accent-2: #0e7490;
      --muted: #5b6776;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      font-family: "Trebuchet MS", "Segoe UI", sans-serif;
      color: var(--ink);
      background:
        radial-gradient(circle at 12% 12%, rgba(14,116,144,.18), transparent 36%),
        radial-gradient(circle at 88% 22%, rgba(194,65,12,.18), transparent 34%),
        linear-gradient(135deg, var(--bg-a), var(--bg-b));
      padding: 18px;
    }
    .shell { width: min(900px, 100%); margin: 0 auto; display: grid; gap: 14px; }
    .card {
      background: var(--card);
      border-radius: 14px;
      border: 1px solid rgba(18,33,47,.1);
      box-shadow: 0 12px 24px rgba(18,33,47,.12);
      padding: 16px;
    }
    h1 { margin: 0 0 6px; }
    .muted { color: var(--muted); margin: 0; }
    .grid {
      margin-top: 12px;
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 10px;
    }
    @media (max-width: 760px) { .grid { grid-template-columns: 1fr; } }
    label { display: block; font-weight: 700; margin-bottom: 6px; }
    select, input[type="range"], input[type="color"], input[type="number"], button {
      width: 100%;
      border-radius: 10px;
      border: 1px solid #d7deea;
      padding: 10px;
      font: inherit;
      background: #fff;
    }
    .row { display: grid; grid-template-columns: 1fr auto; gap: 8px; align-items: center; }
    .chip {
      display: inline-block;
      padding: 4px 10px;
      border-radius: 999px;
      font-size: 12px;
      font-weight: 700;
      background: rgba(14,116,144,.16);
      color: #155e75;
    }
    .switch {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 10px;
      border: 1px solid #d7deea;
      border-radius: 10px;
      background: #fbfdff;
    }
    .actions { display: flex; gap: 8px; flex-wrap: wrap; margin-top: 12px; }
    .btn {
      border: 0;
      border-radius: 10px;
      padding: 10px 14px;
      color: #fff;
      font-weight: 700;
      cursor: pointer;
      background: linear-gradient(120deg, var(--accent), #ea580c);
    }
    .btn.alt { background: linear-gradient(120deg, var(--accent-2), #0284c7); }
    .msg { min-height: 18px; margin-top: 8px; color: #1d4ed8; font-weight: 600; font-size: 13px; }
  </style>
</head>
<body>
  <main class="shell">
    <section class="card">
      <h1>Effects Control</h1>
      <p class="muted">Rendering runs on LED core. Web controls queue updates from the web core.</p>
      <div style="margin-top:10px"><span class="chip" id="frameChip">-</span></div>

      <div class="grid">
        <div>
          <label for="pattern">Pattern</label>
          <select id="pattern"></select>
        </div>

        <div>
          <label for="brightness">Brightness <span id="brightnessVal">0</span></label>
          <input id="brightness" type="range" min="0" max="1000" step="1" />
        </div>

        <div>
          <label for="speed">Speed <span id="speedVal">0</span></label>
          <input id="speed" type="range" min="1" max="255" step="1" />
        </div>

        <div>
          <label for="fps">Target FPS</label>
          <input id="fps" type="number" min="15" max="600" step="1" />
        </div>

        <div>
          <label for="solidColor">Solid Color</label>
          <input id="solidColor" type="color" value="#ffa030" />
        </div>

        <div class="switch">
          <span>Dither</span>
          <input id="dither" type="checkbox" />
        </div>

        <div class="switch">
          <span>Power</span>
          <input id="power" type="checkbox" />
        </div>
      </div>

      <div class="actions">
        <button class="btn" onclick="saveEffects()">Apply Effects Settings</button>
        <button class="btn alt" onclick="reloadState()">Refresh</button>
        <a class="btn alt" href="/home" style="text-decoration:none;display:inline-block">Back Home</a>
      </div>
      <p class="msg" id="msg"></p>
    </section>
  </main>

  <script>
    function hexToRgb(hex) {
      const v = hex.replace('#', '');
      return {
        r: parseInt(v.substring(0, 2), 16),
        g: parseInt(v.substring(2, 4), 16),
        b: parseInt(v.substring(4, 6), 16)
      };
    }

    function rgbToHex(r, g, b) {
      const p = (n) => n.toString(16).padStart(2, '0');
      return `#${p(r)}${p(g)}${p(b)}`;
    }

    function sliderToBrightness(sliderValue) {
      const s = Math.max(0, Math.min(1000, Number(sliderValue) || 0));
      if (s <= 350) {
        return Math.round((s / 350) * 32);
      }
      if (s <= 700) {
        return Math.round(32 + ((s - 350) / 350) * 192);
      }
      return Math.round(224 + ((s - 700) / 300) * 31);
    }

    function brightnessToSlider(brightnessValue) {
      const b = Math.max(0, Math.min(255, Number(brightnessValue) || 0));
      if (b <= 32) {
        return Math.round((b / 32) * 350);
      }
      if (b <= 224) {
        return Math.round(350 + ((b - 32) / 192) * 350);
      }
      return Math.round(700 + ((b - 224) / 31) * 300);
    }

    function updateBrightnessLabel() {
      const slider = document.getElementById('brightness');
      const brightness = sliderToBrightness(slider.value);
      document.getElementById('brightnessVal').textContent = String(brightness);
    }

    async function reloadState() {
      await ensurePatternCatalog();
      const r = await fetch('/api/effects/state');
      const s = await r.json();

      document.getElementById('pattern').value = String(s.pattern ?? 0);
      document.getElementById('brightness').value = String(brightnessToSlider(s.brightness ?? 96));
      document.getElementById('speed').value = String(s.speed ?? 128);
      document.getElementById('fps').value = String(s.fps ?? 120);
      document.getElementById('dither').checked = !!s.dither;
      document.getElementById('power').checked = !!s.power;
      document.getElementById('solidColor').value = rgbToHex(s.red ?? 255, s.green ?? 160, s.blue ?? 48);

      updateBrightnessLabel();
      document.getElementById('speedVal').textContent = String(s.speed ?? 0);
      document.getElementById('frameChip').textContent = `FPS ${s.fps ?? '-'} | Dither ${s.dither ? 'on' : 'off'} | Pattern ${s.pattern_name || s.pattern || '-'} | LEDs ${s.active_leds ?? '-'} / ${s.max_leds ?? '-'}`;
    }

    async function ensurePatternCatalog() {
      const select = document.getElementById('pattern');
      if (select.options.length > 0) {
        return;
      }

      const r = await fetch('/api/effects/catalog');
      const items = await r.json();

      select.innerHTML = '';
      for (const item of items) {
        const opt = document.createElement('option');
        opt.value = String(item.id);
        opt.textContent = item.name;
        select.appendChild(opt);
      }
    }

    async function saveEffects() {
      const color = hexToRgb(document.getElementById('solidColor').value);
      const brightness = sliderToBrightness(document.getElementById('brightness').value);
      const body = new URLSearchParams({
        pattern: document.getElementById('pattern').value,
        brightness: String(brightness),
        speed: document.getElementById('speed').value,
        fps: document.getElementById('fps').value,
        dither: document.getElementById('dither').checked ? '1' : '0',
        power: document.getElementById('power').checked ? '1' : '0',
        red: String(color.r),
        green: String(color.g),
        blue: String(color.b)
      });

      const res = await fetch('/api/effects/set', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });

      const msg = document.getElementById('msg');
      msg.textContent = await res.text();
      reloadState();
    }

    document.getElementById('brightness').addEventListener('input', (e) => {
      updateBrightnessLabel();
    });

    document.getElementById('speed').addEventListener('input', (e) => {
      document.getElementById('speedVal').textContent = e.target.value;
    });

    reloadState();
    setInterval(reloadState, 3000);
  </script>
</body>
</html>
)HTML";
