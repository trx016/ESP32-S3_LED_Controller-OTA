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
    .topbar {
      background: rgba(255, 255, 255, 0.72);
      border: 1px solid rgba(18, 33, 47, 0.12);
      border-radius: 14px;
      padding: 10px;
      backdrop-filter: blur(6px);
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
    }
    .nav {
      border: 1px solid rgba(18, 33, 47, 0.12);
      border-radius: 10px;
      padding: 8px 12px;
      text-decoration: none;
      color: var(--ink);
      font-weight: 700;
      font-size: 13px;
      background: #ffffff;
    }
    .nav.active {
      border-color: rgba(194, 65, 12, 0.35);
      background: rgba(194, 65, 12, 0.14);
      color: #9a3412;
    }
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
    .preset-row {
      display: grid;
      grid-template-columns: 150px 1fr 1fr;
      gap: 8px;
      align-items: center;
      margin-top: 12px;
    }
    @media (max-width: 760px) { .preset-row { grid-template-columns: 1fr; } }
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
    .effect-settings {
      margin-top: 14px;
      border-top: 1px solid rgba(18,33,47,.12);
      padding-top: 14px;
      display: grid;
      gap: 10px;
    }
    .toast {
      position: fixed;
      right: 16px;
      bottom: 16px;
      background: #0f172a;
      color: #f8fafc;
      border-radius: 10px;
      padding: 10px 12px;
      font-size: 13px;
      font-weight: 600;
      box-shadow: 0 10px 24px rgba(2, 6, 23, 0.35);
      opacity: 0;
      transform: translateY(8px);
      pointer-events: none;
      transition: opacity .18s ease, transform .18s ease;
      z-index: 40;
    }
    .toast.show {
      opacity: 1;
      transform: translateY(0);
    }
    .msg { min-height: 18px; margin-top: 8px; color: #1d4ed8; font-weight: 600; font-size: 13px; }
  </style>
</head>
<body>
  <main class="shell">
    <nav class="topbar">
      <a class="nav" href="/setup">Setup</a>
      <a class="nav" href="/home">Home</a>
      <a class="nav active" href="/effects">Effects</a>
      <a class="nav" href="/settings">Settings</a>
    </nav>

    <section class="card">
      <h1>Effects Control</h1>
      <p class="muted">Global controls stay fixed. Effect-specific controls are generated from each effect module.</p>
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

        <div class="switch">
          <span>Dither</span>
          <input id="dither" type="checkbox" />
        </div>

        <div class="switch">
          <span>Power</span>
          <input id="power" type="checkbox" />
        </div>
      </div>

      <div class="effect-settings" id="effectSettingsWrap">
        <h3 style="margin:0">Effect Settings</h3>
        <div id="effectSettingsGrid" class="grid"></div>
      </div>

      <div class="preset-row">
        <select id="presetSlot"></select>
        <button class="btn alt" onclick="loadPreset()">Load Preset</button>
        <button class="btn alt" onclick="savePreset()">Save Preset</button>
      </div>
      <div class="preset-row" style="margin-top:8px">
        <input id="presetName" type="text" maxlength="24" placeholder="Preset name" />
        <button class="btn alt" onclick="renamePreset()">Rename Preset</button>
        <button class="btn" onclick="resetEffectSettings()">Reset Effect Settings</button>
      </div>

      <div class="actions">
        <button class="btn alt" onclick="reloadState()">Refresh</button>
        <a class="btn alt" href="/home" style="text-decoration:none;display:inline-block">Back Home</a>
      </div>
      <p class="msg" id="msg"></p>
    </section>
  </main>
  <div id="toast" class="toast"></div>

  <script>
    let lastStateRaw = '';
    let lastSchemaRaw = '';
    let lastEffectSettingsRaw = '';
    let activePattern = -1;
    let stateInFlight = false;
    let settingsInFlight = false;
    let catalogReady = false;
    let globalApplyTimer = null;
    let toastTimer = null;
    const effectSettingTimers = {};

    function setTextIfChanged(id, value) {
      const el = document.getElementById(id);
      if (el.textContent !== value) {
        el.textContent = value;
      }
    }

    function setValueIfChanged(id, value) {
      const el = document.getElementById(id);
      if (el.value !== value) {
        el.value = value;
      }
    }

    function setCheckedIfChanged(id, checked) {
      const el = document.getElementById(id);
      if (el.checked !== checked) {
        el.checked = checked;
      }
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

    function setMsg(text) {
      document.getElementById('msg').textContent = text;
    }

    function showToast(text) {
      const toast = document.getElementById('toast');
      toast.textContent = text;
      toast.classList.add('show');
      if (toastTimer !== null) {
        clearTimeout(toastTimer);
      }
      toastTimer = setTimeout(() => {
        toast.classList.remove('show');
      }, 1400);
    }

    function getDynamicControlIds() {
      return Array.from(document.querySelectorAll('[data-setting-key]')).map((el) => el.id);
    }

    function scheduleGlobalApply(delayMs) {
      if (globalApplyTimer !== null) {
        clearTimeout(globalApplyTimer);
      }
      globalApplyTimer = setTimeout(() => {
        globalApplyTimer = null;
        saveGlobalEffects(false, true);
      }, delayMs);
    }

    function scheduleEffectSetting(key, value, delayMs) {
      if (effectSettingTimers[key]) {
        clearTimeout(effectSettingTimers[key]);
      }
      effectSettingTimers[key] = setTimeout(() => {
        delete effectSettingTimers[key];
        applyEffectSetting(key, value, true);
      }, delayMs);
    }

    async function reloadPresetNames() {
      const res = await fetch('/api/effects/active/preset/names');
      const raw = await res.text();
      if (!res.ok) {
        return;
      }

      const names = JSON.parse(raw || '[]');
      const select = document.getElementById('presetSlot');
      const current = select.value || '1';
      select.innerHTML = '';

      for (const item of names) {
        const opt = document.createElement('option');
        opt.value = String(item.slot);
        opt.textContent = String(item.name || ('Preset ' + item.slot));
        select.appendChild(opt);
      }

      if (Array.from(select.options).some((o) => o.value === current)) {
        select.value = current;
      }
      updatePresetNameInput();
    }

    function updatePresetNameInput() {
      const select = document.getElementById('presetSlot');
      const nameInput = document.getElementById('presetName');
      const selected = select.options[select.selectedIndex];
      nameInput.value = selected ? selected.textContent : '';
    }

    async function reloadState() {
      if (stateInFlight) {
        return;
      }

      stateInFlight = true;
      try {
        await ensurePatternCatalog();
        const r = await fetch('/api/effects/state');
        const raw = await r.text();
        if (!r.ok) {
          setMsg(raw || 'Failed to read effects state.');
          return;
        }

        if (raw === lastStateRaw) {
          await reloadEffectSettingsState(false);
          return;
        }

        const s = JSON.parse(raw);
        const dynamicIds = getDynamicControlIds();
        const editingControls =
          document.activeElement === document.getElementById('pattern') ||
          document.activeElement === document.getElementById('brightness') ||
          document.activeElement === document.getElementById('speed') ||
          document.activeElement === document.getElementById('dither') ||
          document.activeElement === document.getElementById('power') ||
          dynamicIds.includes(document.activeElement ? document.activeElement.id : '');

        if (!editingControls) {
          setValueIfChanged('pattern', String(s.pattern ?? 0));
          setValueIfChanged('brightness', String(brightnessToSlider(s.brightness ?? 96)));
          setValueIfChanged('speed', String(s.speed ?? 128));
          setCheckedIfChanged('dither', !!s.dither);
          setCheckedIfChanged('power', !!s.power);

          updateBrightnessLabel();
          setTextIfChanged('speedVal', String(s.speed ?? 0));
        }

        setTextIfChanged('frameChip', `FPS ${s.fps ?? '-'} | Dither ${s.dither ? 'on' : 'off'} | Pattern ${s.pattern_name || s.pattern || '-'} | LEDs ${s.active_leds ?? '-'} / ${s.max_leds ?? '-'}`);

        if (activePattern !== Number(s.pattern ?? 0)) {
          activePattern = Number(s.pattern ?? 0);
          await reloadEffectSchema(true);
          await reloadPresetNames();
        }

        lastStateRaw = raw;
        await reloadEffectSettingsState(false);
      } finally {
        stateInFlight = false;
      }
    }

    async function ensurePatternCatalog() {
      if (catalogReady) {
        return;
      }

      const select = document.getElementById('pattern');
      if (select.options.length > 0) {
        catalogReady = true;
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

      catalogReady = true;
    }

    function renderSettingControl(def, value) {
      const key = String(def.key || '');
      const type = String(def.type || 'slider').toLowerCase();

      const wrap = document.createElement('div');
      const label = document.createElement('label');
      label.setAttribute('for', `efx_${key}`);
      label.textContent = String(def.label || key);
      wrap.appendChild(label);

      if (type === 'toggle') {
        const holder = document.createElement('div');
        holder.className = 'switch';
        const left = document.createElement('span');
        left.textContent = String(def.label || key);
        const input = document.createElement('input');
        input.type = 'checkbox';
        input.id = `efx_${key}`;
        input.dataset.settingKey = key;
        input.checked = !!value;
        input.addEventListener('change', () => applyEffectSetting(key, input.checked ? '1' : '0'));
        holder.innerHTML = '';
        holder.appendChild(left);
        holder.appendChild(input);
        wrap.innerHTML = '';
        wrap.appendChild(holder);
        return wrap;
      }

      if (type === 'dropdown' || type === 'dlist' || type === 'select') {
        const select = document.createElement('select');
        select.id = `efx_${key}`;
        select.dataset.settingKey = key;
        const opts = Array.isArray(def.options) ? def.options : [];
        for (const o of opts) {
          const opt = document.createElement('option');
          if (typeof o === 'object' && o !== null) {
            opt.value = String(o.value ?? '');
            opt.textContent = String(o.label ?? o.value ?? '');
          } else {
            opt.value = String(o);
            opt.textContent = String(o);
          }
          select.appendChild(opt);
        }
        select.value = String(value ?? '');
        select.addEventListener('change', () => applyEffectSetting(key, select.value));
        wrap.appendChild(select);
        return wrap;
      }

      const min = Number(def.min ?? 0);
      const max = Number(def.max ?? 255);
      const step = Number(def.step ?? 1);
      const row = document.createElement('div');
      row.className = 'row';

      const input = document.createElement('input');
      input.type = 'range';
      input.id = `efx_${key}`;
      input.dataset.settingKey = key;
      input.min = String(min);
      input.max = String(max);
      input.step = String(step);
      input.value = String(value ?? min);

      const out = document.createElement('span');
      out.id = `efx_${key}_val`;
      out.textContent = String(input.value);

      input.addEventListener('input', () => {
        out.textContent = input.value;
        scheduleEffectSetting(key, input.value, 80);
      });
      input.addEventListener('change', () => {
        scheduleEffectSetting(key, input.value, 0);
      });

      row.appendChild(input);
      row.appendChild(out);
      wrap.appendChild(row);
      return wrap;
    }

    async function reloadEffectSchema(force) {
      if (settingsInFlight) {
        return;
      }

      settingsInFlight = true;
      try {
        const r = await fetch('/api/effects/active/settings/schema');
        const raw = await r.text();
        if (!r.ok) {
          setMsg(raw || 'Failed to load effect schema.');
          return;
        }

        if (!force && raw === lastSchemaRaw) {
          return;
        }

        const schema = JSON.parse(raw);
        const stateRes = await fetch('/api/effects/active/settings/state');
        const stateRaw = await stateRes.text();
        const state = stateRes.ok ? JSON.parse(stateRaw || '{}') : {};

        const grid = document.getElementById('effectSettingsGrid');
        grid.innerHTML = '';

        for (const def of schema) {
          const key = String(def.key || '');
          if (!key) {
            continue;
          }
          grid.appendChild(renderSettingControl(def, state[key]));
        }

        lastSchemaRaw = raw;
        lastEffectSettingsRaw = stateRaw;
      } finally {
        settingsInFlight = false;
      }
    }

    async function reloadEffectSettingsState(force) {
      const r = await fetch('/api/effects/active/settings/state');
      const raw = await r.text();
      if (!r.ok) {
        return;
      }

      if (!force && raw === lastEffectSettingsRaw) {
        return;
      }

      const state = JSON.parse(raw || '{}');
      const controls = document.querySelectorAll('[data-setting-key]');
      for (const el of controls) {
        const key = el.dataset.settingKey;
        if (!(key in state)) {
          continue;
        }

        if (document.activeElement === el) {
          continue;
        }

        const v = state[key];
        if (el.type === 'checkbox') {
          if (el.checked !== !!v) {
            el.checked = !!v;
          }
        } else if (String(el.value) !== String(v)) {
          el.value = String(v);
          const out = document.getElementById(el.id + '_val');
          if (out) {
            out.textContent = String(v);
          }
        }
      }

      lastEffectSettingsRaw = raw;
    }

    async function applyEffectSetting(key, value, silent) {
      const body = new URLSearchParams({ key: String(key), value: String(value) });
      const res = await fetch('/api/effects/active/settings/set', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });

      const message = await res.text();
      if (!silent) {
        setMsg(message);
        await reloadEffectSettingsState(true);
        await reloadState();
        showToast(message);
      }
    }

    async function saveGlobalEffects(refreshSchema, silent) {
      const brightness = sliderToBrightness(document.getElementById('brightness').value);
      const body = new URLSearchParams({
        pattern: document.getElementById('pattern').value,
        brightness: String(brightness),
        speed: document.getElementById('speed').value,
        dither: document.getElementById('dither').checked ? '1' : '0',
        power: document.getElementById('power').checked ? '1' : '0'
      });

      const res = await fetch('/api/effects/set', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });

      const message = await res.text();
      if (!silent) {
        setMsg(message);
        showToast(message);
      }
      await reloadState();
      if (refreshSchema) {
        await reloadEffectSchema(true);
        await reloadPresetNames();
      }
    }

    async function savePreset() {
      const body = new URLSearchParams({ slot: document.getElementById('presetSlot').value });
      const res = await fetch('/api/effects/active/preset/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });
      const message = await res.text();
      setMsg(message);
      showToast(message);
    }

    async function loadPreset() {
      const body = new URLSearchParams({ slot: document.getElementById('presetSlot').value });
      const res = await fetch('/api/effects/active/preset/load', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });
      const message = await res.text();
      setMsg(message);
      showToast(message);
      await reloadEffectSettingsState(true);
      await reloadState();
    }

    async function renamePreset() {
      const slot = document.getElementById('presetSlot').value;
      const name = document.getElementById('presetName').value.trim();
      if (!name) {
        setMsg('Preset name cannot be empty.');
        showToast('Preset name cannot be empty.');
        return;
      }

      const body = new URLSearchParams({ slot, name });
      const res = await fetch('/api/effects/active/preset/rename', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });
      const message = await res.text();
      setMsg(message);
      showToast(message);
      await reloadPresetNames();
      document.getElementById('presetSlot').value = String(slot);
      updatePresetNameInput();
    }

    async function resetEffectSettings() {
      const res = await fetch('/api/effects/active/settings/reset', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: ''
      });
      const message = await res.text();
      setMsg(message);
      showToast(message);
      await reloadEffectSettingsState(true);
      await reloadState();
    }

    document.getElementById('brightness').addEventListener('input', (e) => {
      updateBrightnessLabel();
      scheduleGlobalApply(80);
    });

    document.getElementById('speed').addEventListener('input', (e) => {
      document.getElementById('speedVal').textContent = e.target.value;
      scheduleGlobalApply(80);
    });

    document.getElementById('pattern').addEventListener('change', async () => {
      await saveGlobalEffects(true, false);
    });

    document.getElementById('presetSlot').addEventListener('change', () => {
      updatePresetNameInput();
    });

    document.getElementById('dither').addEventListener('change', async () => {
      await saveGlobalEffects(false, true);
    });

    document.getElementById('power').addEventListener('change', async () => {
      await saveGlobalEffects(false, true);
    });

    reloadState();
    reloadEffectSchema(true);
    reloadPresetNames();
    setInterval(reloadState, 3000);
  </script>
</body>
</html>
)HTML";
