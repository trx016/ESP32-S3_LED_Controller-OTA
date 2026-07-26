//
//   system working as of march 29 on the esp
//   connect LED data to pin D6 (12 - in software)
//   connect Ir sensor to pin D5(14 - in software)
//   Board - ESP32-WROOM-32.



#include "LEDConfig.h"
#include <FastLED.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "IR_RemoteHandler.h"
#include "LEDUtils.h"
#include "Classes.h"
#include "LightningStrike.h"
#include "LedPongChasers.h"
#include "Rain.h"
#include "Flame.h"
#include "StaticLights.h"
#include "Pac_Man.h"
#include "LightningStrike_Alt.h"
#include "Kaleidoscope.h"
#include "RainbowWaves.h"
#include "NeuroTrip.h"
#include "WallCountTest.h"




IRRemoteHandler remoteHandler(14);  // GPIO14 = D5
CRGB leds[NUM_LEDS];                // Actual LED array definition
WebServer server(80);
Preferences preferences;

// Optional compile-time fallback credentials.
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";

// Built-in AP credentials for direct phone/laptop control.
const char* AP_SSID = "ESP32_LED_Controller";
const char* AP_PASSWORD = "ledcontrol32";

char configuredWifiSsid[33] = {0};
char configuredWifiPassword[65] = {0};

const char WEB_UI[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>ESP32 LED Control</title>
  <style>
    :root {
      --bg-0: #09131f;
      --bg-1: #12253a;
      --card: #f4f1e8;
      --text: #17212d;
      --accent: #d04f2f;
      --accent-2: #ffb347;
      --muted: #516173;
      --ok: #1a8f5a;
    }

    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: "Space Grotesk", "Trebuchet MS", sans-serif;
      color: var(--text);
      background:
        radial-gradient(circle at 20% 20%, rgba(255,179,71,.28), transparent 40%),
        radial-gradient(circle at 85% 10%, rgba(208,79,47,.22), transparent 45%),
        linear-gradient(145deg, var(--bg-0), var(--bg-1));
      min-height: 100vh;
      padding: 20px;
      overflow-x: hidden;
    }

    input[type="color"],
    input[type="range"],
    input[type="text"],
    select,
    textarea {
      max-width: 100%;
      box-sizing: border-box;
    }

    .pill {
      display: inline-block;
      padding: 4px 10px;
      border-radius: 999px;
      background: #dde7ef;
      color: #243747;
      font-size: .82rem;
      font-weight: 700;
    }
    @keyframes rise {
      from { opacity: 0; transform: translateY(10px) scale(.99); }
      to { opacity: 1; transform: translateY(0) scale(1); }
    }

    .card {
      background: var(--card);
      border-radius: 16px;
      padding: 16px;
      box-shadow: 0 14px 28px rgba(0,0,0,.22);
      max-width: 100%;
      overflow: visible;
    }

    .top {
      display: grid;
      grid-template-columns: 1fr auto;
      gap: 12px;
      align-items: center;
    }

    h1 {
      margin: 0;
      letter-spacing: .02em;
      font-size: clamp(1.2rem, 2.8vw, 1.8rem);
    }

    .sub {
      margin-top: 4px;
      color: var(--muted);
      font-size: .92rem;
    }

    .grid {
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 12px;
    }

    @media (max-width: 760px) {
      .grid { grid-template-columns: 1fr; }
      .top { grid-template-columns: 1fr; }
    }

    label {
      display: block;
      font-weight: 700;
      font-size: .9rem;
      margin-bottom: 8px;
    }

    input[type="range"], input[type="text"], input[type="password"], select, button {
      width: 100%;
      border-radius: 10px;
      border: 1px solid #d8d1c2;
      padding: 10px;
      font: inherit;
    }

    .row {
      display: grid;
      grid-template-columns: 1fr auto;
      gap: 8px;
      align-items: center;
    }

    .btn {
      border: 0;
      background: linear-gradient(110deg, var(--accent), #b43318);
      color: #fff;
      font-weight: 700;
      cursor: pointer;
      transition: transform .12s ease, filter .12s ease;
    }

    .btn:active {
      transform: translateY(1px) scale(.99);
      filter: brightness(.95);
    }

    .btn.alt {
      background: linear-gradient(110deg, #2f5f87, #1d496d);
    }

    .pill {
      display: inline-block;
      padding: 4px 10px;
      border-radius: 999px;
      background: #dde7ef;
      color: #243747;
      font-size: .82rem;
      font-weight: 700;
    }

    .status-ok {
      color: var(--ok);
      font-weight: 700;
    }

    .color-grid {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(42px, 1fr));
      gap: 8px;
      width: 100%;
      max-width: 100%;
    }

    #solidPanel {
      max-width: 100%;
      overflow-x: visible;
    }

    .color-preset {
      width: 100%;
      min-width: 0;
      aspect-ratio: 1;
      border: none;
      border-radius: 8px;
      padding: 0;
      cursor: pointer;
      transition: transform .15s ease, box-shadow .15s ease, filter .15s ease;
      box-shadow: 0 2px 8px rgba(0,0,0,.15);
      position: relative;
    }

    .color-preset:hover {
      transform: scale(1.06);
      box-shadow: 0 4px 16px rgba(0,0,0,.25);
    }

    #presetsPanel {
      max-width: 100%;
    }

    #presetSlots {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
      gap: 8px;
      width: 100%;
      max-width: 100%;
    }

    .preset-slot {
      display: grid;
      grid-template-columns: minmax(0, 1fr) 32px;
      gap: 4px;
      min-width: 0;
      align-items: stretch;
    }

    .preset-slot.empty {
      grid-template-columns: minmax(0, 1fr);
    }

    .preset-btn {
      min-width: 0;
      white-space: normal;
      word-break: break-word;
      line-height: 1.2;
      padding: 8px;
    }

    .preset-delete {
      width: 32px;
      min-width: 32px;
      padding: 0;
      font-size: 14px;
      line-height: 1;
    }

    /* Mobile Responsive */
    @media (max-width: 760px) {
      body { padding: 10px; }
      .card { padding: 12px; }
      .color-grid {
        grid-template-columns: repeat(auto-fill, minmax(40px, 1fr));
        gap: 6px;
      }
    }

    @media (max-width: 600px) {
      body { padding: 6px; }
      .shell { gap: 10px; }
      .card {
        padding: 10px;
        border-radius: 12px;
      }
      .grid {
        grid-template-columns: 1fr !important;
        gap: 10px;
      }
      .color-grid {
        grid-template-columns: repeat(4, minmax(0, 1fr));
        gap: 5px;
      }
      #presetSlots {
        grid-template-columns: repeat(2, minmax(0, 1fr));
      }
      .color-preset:hover { transform: scale(1.02); }
      .color-preset:hover::after {
        opacity: 0;
        pointer-events: none;
      }
    }

    @media (max-width: 380px) {
      body { padding: 4px; }
      .card {
        padding: 8px;
        border-radius: 10px;
      }
      .color-grid {
        grid-template-columns: repeat(3, minmax(0, 1fr));
        gap: 4px;
      }
      #presetSlots {
        grid-template-columns: 1fr;
      }
      select, input[type="range"], input[type="color"] { font-size: 16px; }
    }
  </style>
</head>
<body>
  <main class="shell">
    <section class="card top">
      <div>
        <h1>ESP32 LED Controller</h1>
        <div class="sub">Live control for patterns, brightness, and effect tuning</div>
      </div>
      <div class="pill" id="net">Loading network...</div>
    </section>

    <section class="card grid">
      <div>
        <label for="pattern">Pattern</label>
        <select id="pattern"></select>
      </div>
      <div>
        <label for="brightness">Brightness <span id="brightnessVal">0</span></label>
        <input id="brightness" type="range" min="0" max="255" step="1" />
      </div>
      <div>
        <button class="btn" id="powerBtn">Toggle Power</button>
      </div>
      <div>
        <button class="btn alt" id="refreshBtn">Refresh State</button>
      </div>
      <div>
        <button class="btn alt" id="wifiToggleBtn">Wi-Fi Settings</button>
      </div>
    </section>

    <section class="card" id="solidPanel">
      <h3 style="margin-top:0;margin-bottom:16px;">Solid Color</h3>
      
      <!-- Red & Pink Family -->
      <div style="margin-bottom:16px;">
        <div style="font-size:0.85em;color:#516173;margin-bottom:8px;font-weight:600;">Reds & Pinks</div>
        <div class="color-grid">
          <button class="color-preset" data-color="#FF0000" style="background:#FF0000;" title="Red"></button>
          <button class="color-preset" data-color="#DC143C" style="background:#DC143C;" title="Crimson"></button>
          <button class="color-preset" data-color="#FF1493" style="background:#FF1493;" title="Deep Pink"></button>
          <button class="color-preset" data-color="#FF69B4" style="background:#FF69B4;" title="Hot Pink"></button>
          <button class="color-preset" data-color="#FFB6C1" style="background:#FFB6C1;" title="Light Pink"></button>
          <button class="color-preset" data-color="#C71585" style="background:#C71585;" title="Medium Violet Red"></button>
        </div>
      </div>

      <!-- Orange & Yellow Family -->
      <div style="margin-bottom:16px;">
        <div style="font-size:0.85em;color:#516173;margin-bottom:8px;font-weight:600;">Oranges & Yellows</div>
        <div class="color-grid">
          <button class="color-preset" data-color="#FFA500" style="background:#FFA500;" title="Orange"></button>
          <button class="color-preset" data-color="#FF8C00" style="background:#FF8C00;" title="Dark Orange"></button>
          <button class="color-preset" data-color="#FFD700" style="background:#FFD700;" title="Gold"></button>
          <button class="color-preset" data-color="#FFFF00" style="background:#FFFF00;" title="Yellow"></button>
          <button class="color-preset" data-color="#ADFF2F" style="background:#ADFF2F;" title="Green Yellow"></button>
          <button class="color-preset" data-color="#FFCC00" style="background:#FFCC00;" title="Amber"></button>
        </div>
      </div>

      <!-- Green & Cyan Family -->
      <div style="margin-bottom:16px;">
        <div style="font-size:0.85em;color:#516173;margin-bottom:8px;font-weight:600;">Greens & Cyans</div>
        <div class="color-grid">
          <button class="color-preset" data-color="#00FF00" style="background:#00FF00;" title="Green"></button>
          <button class="color-preset" data-color="#008000" style="background:#008000;" title="Dark Green"></button>
          <button class="color-preset" data-color="#32CD32" style="background:#32CD32;" title="Lime Green"></button>
          <button class="color-preset" data-color="#00CED1" style="background:#00CED1;" title="Dark Turquoise"></button>
          <button class="color-preset" data-color="#00FFFF" style="background:#00FFFF;" title="Cyan"></button>
          <button class="color-preset" data-color="#20B2AA" style="background:#20B2AA;" title="Light Sea Green"></button>
        </div>
      </div>

      <!-- Blue & Purple Family -->
      <div style="margin-bottom:16px;">
        <div style="font-size:0.85em;color:#516173;margin-bottom:8px;font-weight:600;">Blues & Purples</div>
        <div class="color-grid">
          <button class="color-preset" data-color="#0000FF" style="background:#0000FF;" title="Blue"></button>
          <button class="color-preset" data-color="#000080" style="background:#000080;" title="Navy"></button>
          <button class="color-preset" data-color="#4169E1" style="background:#4169E1;" title="Royal Blue"></button>
          <button class="color-preset" data-color="#9370DB" style="background:#9370DB;" title="Medium Purple"></button>
          <button class="color-preset" data-color="#800080" style="background:#800080;" title="Purple"></button>
          <button class="color-preset" data-color="#4B0082" style="background:#4B0082;" title="Indigo"></button>
        </div>
      </div>

      <!-- Magenta & Violet Family -->
      <div style="margin-bottom:16px;">
        <div style="font-size:0.85em;color:#516173;margin-bottom:8px;font-weight:600;">Magentas & Violets</div>
        <div class="color-grid">
          <button class="color-preset" data-color="#FF00FF" style="background:#FF00FF;" title="Magenta"></button>
          <button class="color-preset" data-color="#EE82EE" style="background:#EE82EE;" title="Violet"></button>
          <button class="color-preset" data-color="#DA70D6" style="background:#DA70D6;" title="Orchid"></button>
          <button class="color-preset" data-color="#BA55D3" style="background:#BA55D3;" title="Medium Orchid"></button>
          <button class="color-preset" data-color="#DDA0DD" style="background:#DDA0DD;" title="Plum"></button>
          <button class="color-preset" data-color="#D8BFD8" style="background:#D8BFD8;" title="Thistle"></button>
        </div>
      </div>

      <!-- Neutral & Whites Family -->
      <div style="margin-bottom:16px;">
        <div style="font-size:0.85em;color:#516173;margin-bottom:8px;font-weight:600;">Neutrals & Whites</div>
        <div class="color-grid">
          <button class="color-preset" data-color="#FFFFFF" style="background:#FFFFFF;border:1px solid #999;" title="White"></button>
          <button class="color-preset" data-color="#F5F5F5" style="background:#F5F5F5;border:1px solid #999;" title="White Smoke"></button>
          <button class="color-preset" data-color="#D3D3D3" style="background:#D3D3D3;border:1px solid #999;" title="Light Gray"></button>
          <button class="color-preset" data-color="#A9A9A9" style="background:#A9A9A9;" title="Dark Gray"></button>
          <button class="color-preset" data-color="#808080" style="background:#808080;" title="Gray"></button>
          <button class="color-preset" data-color="#000000" style="background:#000000;border:1px solid #fff;" title="Black"></button>
        </div>
      </div>
      
      <!-- Custom Color Picker (Advanced) -->
      <div style="border-top:1px solid #ddd;padding-top:12px;">
        <label for="solidColor" style="font-size:0.9em;color:#516173;margin-bottom:8px;display:block;font-weight:600;">Custom Color:</label>
        <div style="display:grid;grid-template-columns:1fr auto;gap:8px;align-items:center;">
          <input id="solidColor" type="color" value="#0000ff" style="height:44px;border-radius:8px;cursor:pointer;" />
          <button class="btn" id="solidApplyBtn">Apply</button>
        </div>
      </div>
    </section>

    <section class="card grid" id="rainPanel">
      <div>
        <label for="rainChance">Rain Drop Chance <span id="rainChanceVal"></span></label>
        <input id="rainChance" type="range" min="0" max="29" step="1" />
      </div>
      <div>
        <label for="rainFade">Rain Fade Amount <span id="rainFadeVal"></span></label>
        <input id="rainFade" type="range" min="0" max="99" step="1" />
      </div>
      <div>
        <label for="rainDelay">Rain Update Delay (ms) <span id="rainDelayVal"></span></label>
        <input id="rainDelay" type="range" min="25" max="250" step="25" />
      </div>
      <div>
        <button class="btn" id="rainReset">Reset Rain Defaults</button>
      </div>
    </section>

    <section class="card grid" id="flamePanel">
      <div>
        <label for="flameCool">Flame Cool <span id="flameCoolVal"></span></label>
        <input id="flameCool" type="range" min="1" max="50" step="1" />
      </div>
      <div>
        <label for="flameSparking">Flame Sparking <span id="flameSparkingVal"></span></label>
        <input id="flameSparking" type="range" min="0" max="255" step="1" />
      </div>
      <div>
        <label for="flameDelay">Flame Update Delay (ms) <span id="flameDelayVal"></span></label>
        <input id="flameDelay" type="range" min="5" max="200" step="5" />
      </div>
      <div>
        <label for="flamePalette">Flame Palette</label>
        <select id="flamePalette">
          <option value="0">Classic Ember</option>
          <option value="1">Neon Blend</option>
          <option value="2">Cool Blue</option>
          <option value="3">Arctic White</option>
          <option value="4">Warm White</option>
        </select>
      </div>
      <div>
        <label for="flameBgLevel">Background Ember Level <span id="flameBgLevelVal"></span></label>
        <input id="flameBgLevel" type="range" min="0" max="30" step="1" />
      </div>
      <div>
        <button class="btn" id="flameReset">Reset Flame Defaults</button>
      </div>
    </section>

    <section class="card grid" id="lightningAltPanel" style="display:none;">
      <h3 style="grid-column:1/-1">Lightning Strike Settings</h3>
      <div>
        <label for="lightningMinInterval">Min Strike Interval (s) <span id="lightningMinIntervalVal"></span></label>
        <input id="lightningMinInterval" type="range" min="5" max="300" step="1" />
      </div>
      <div>
        <label for="lightningMaxInterval">Max Strike Interval (s) <span id="lightningMaxIntervalVal"></span></label>
        <input id="lightningMaxInterval" type="range" min="5" max="300" step="1" />
      </div>
      <div>
        <label for="lightningFlashBrightness">Flash Brightness <span id="lightningFlashBrightnessVal"></span></label>
        <input id="lightningFlashBrightness" type="range" min="0" max="255" step="1" />
      </div>
      <div>
        <label for="lightningSpeed">Lightning Speed % <span id="lightningSpeedVal"></span></label>
        <input id="lightningSpeed" type="range" min="50" max="220" step="5" />
      </div>
      <div>
        <label for="lightningMinDuration">Min Strike Duration (ms) <span id="lightningMinDurationVal"></span></label>
        <input id="lightningMinDuration" type="range" min="10" max="2000" step="10" />
      </div>
      <div>
        <label for="lightningMaxDuration">Max Strike Duration (ms) <span id="lightningMaxDurationVal"></span></label>
        <input id="lightningMaxDuration" type="range" min="10" max="2000" step="10" />
      </div>
      <div style="display:flex;align-items:center;gap:8px;">
        <input id="lightningStrikePin" type="checkbox" />
        <label for="lightningStrikePin" style="margin:0;">Enable strike-pin lead-in</label>
      </div>
      <div>
        <label for="lightningStrikePinChance">Strike Pin Chance % <span id="lightningStrikePinChanceVal"></span></label>
        <input id="lightningStrikePinChance" type="range" min="0" max="100" step="1" />
      </div>
      <div>
        <label for="lightningStrikePinSpeed">Strike Pin Speed Multiplier <span id="lightningStrikePinSpeedVal"></span>x</label>
        <input id="lightningStrikePinSpeed" type="range" min="4" max="50" step="1" />
      </div>
      <div style="display:flex;align-items:center;gap:8px;">
        <input id="lightningTestMode" type="checkbox" />
        <label for="lightningTestMode" style="margin:0;">Enable test strike at LED 50</label>
      </div>
      <div>
        <label for="lightningTestDelay">Test Strike Delay (ms) <span id="lightningTestDelayVal"></span></label>
        <input id="lightningTestDelay" type="range" min="50" max="5000" step="10" />
      </div>
      <h3 style="grid-column:1/-1">Rain Settings</h3>
      <div>
        <label for="rainSpawnChance">Rain Spawn Chance <span id="rainSpawnChanceVal"></span></label>
        <input id="rainSpawnChance" type="range" min="0" max="255" step="1" />
      </div>
      <div>
        <label for="rainFadeSpeed">Rain Fade Speed <span id="rainFadeSpeedVal"></span></label>
        <input id="rainFadeSpeed" type="range" min="0" max="120" step="1" />
      </div>
      <div>
        <label for="rainUpdateSpeed">Rain Update Delay (ms) <span id="rainUpdateSpeedVal"></span></label>
        <input id="rainUpdateSpeed" type="range" min="1" max="200" step="1" />
      </div>
      <div>
        <label for="rainDropBrightness">Rain Brightness <span id="rainDropBrightnessVal"></span></label>
        <input id="rainDropBrightness" type="range" min="0" max="255" step="1" />
      </div>
      <div>
        <label for="rainTrailChance">Trail Chance % <span id="rainTrailChanceVal"></span></label>
        <input id="rainTrailChance" type="range" min="0" max="100" step="5" />
      </div>
      <div>
        <label for="rainPeakDuration">Rain Hold Duration (ms) <span id="rainPeakDurationVal"></span></label>
        <input id="rainPeakDuration" type="range" min="50" max="1000" step="10" />
      </div>
      <div>
        <label for="rainColorMode">Rain Color Mode</label>
        <select id="rainColorMode" style="width:100%;padding:8px;border-radius:4px;border:1px solid #ccc;">
          <option value="0">Multi-Tone Blue (Default)</option>
          <option value="1">Solid Blue</option>
          <option value="2">White to Blue</option>
        </select>
      </div>
      <div>
        <button class="btn" id="lightningAltReset">Reset Lightning Alt Defaults</button>
      </div>
    </section>

    <section class="card grid" id="kaleidoscopePanel" style="display:none;">
      <h3 style="grid-column:1/-1">Kaleidoscope Settings</h3>
      <div>
        <label for="kaleidoscopeSpeed">Kaleidoscope Speed % <span id="kaleidoscopeSpeedVal"></span></label>
        <input id="kaleidoscopeSpeed" type="range" min="50" max="220" step="5" />
      </div>
      <div>
        <button class="btn" id="kaleidoscopeReset">Reset Kaleidoscope Defaults</button>
      </div>
    </section>

    <section class="card grid" id="particleMayhemPanel" style="display:none;">
      <h3 style="grid-column:1/-1">Particle Mayhem Settings</h3>
      <div>
        <label for="particleMayhemSpeed">Particle Speed % <span id="particleMayhemSpeedVal"></span></label>
        <input id="particleMayhemSpeed" type="range" min="50" max="220" step="5" />
      </div>
      <div>
        <label for="particleMayhemSpeedBoost">Speed Boost <span id="particleMayhemSpeedBoostVal"></span></label>
        <input id="particleMayhemSpeedBoost" type="range" min="0" max="100" step="5" />
      </div>
      <div>
        <label for="particleMayhemRandomSpeed">Random Speed % <span id="particleMayhemRandomSpeedVal"></span></label>
        <input id="particleMayhemRandomSpeed" type="range" min="0" max="100" step="5" />
      </div>
      <div>
        <label for="particleMayhemParticleCount">Particle Count <span id="particleMayhemParticleCountVal"></span></label>
        <input id="particleMayhemParticleCount" type="range" min="1" max="10" step="1" />
      </div>
      <div>
        <label for="particleMayhemDensity">Density <span id="particleMayhemDensityVal"></span></label>
        <input id="particleMayhemDensity" type="range" min="1" max="5" step="1" />
      </div>
      <div>
        <label for="particleMayhemExplosionSize">Explosion Size <span id="particleMayhemExplosionSizeVal"></span></label>
        <input id="particleMayhemExplosionSize" type="range" min="2" max="10" step="1" />
      </div>
      <div>
        <label for="particleMayhemExplosionSpeed">Explosion Speed <span id="particleMayhemExplosionSpeedVal"></span></label>
        <input id="particleMayhemExplosionSpeed" type="range" min="1" max="10" step="1" />
      </div>
      <div>
        <button class="btn" id="particleMayhemReset">Reset Particle Mayhem</button>
      </div>
    </section>

    <section class="card grid" id="neuroTripPanel" style="display:none;">
      <h3 style="grid-column:1/-1">Neuro Prism Settings</h3>
      <div>
        <label for="neuroTripSpeed">Flow Speed % <span id="neuroTripSpeedVal"></span></label>
        <input id="neuroTripSpeed" type="range" min="30" max="240" step="5" />
      </div>
      <div>
        <label for="neuroTripDepth">Pattern Depth <span id="neuroTripDepthVal"></span></label>
        <input id="neuroTripDepth" type="range" min="40" max="255" step="1" />
      </div>
      <div>
        <label for="neuroTripPulse">Pulse Strength <span id="neuroTripPulseVal"></span></label>
        <input id="neuroTripPulse" type="range" min="0" max="255" step="1" />
      </div>
      <div>
        <button class="btn" id="neuroTripReset">Reset Neuro Prism</button>
      </div>
    </section>

    <section class="card grid" id="presetsPanel" style="display:none;">
      <h3 style="grid-column:1/-1">Presets</h3>
      <div id="presetSlots" style="grid-column:1/-1;"></div>
      <div style="grid-column:1/-1;">
        <input id="presetName" type="text" placeholder="Preset name" maxlength="31" />
        <button class="btn" id="savePresetBtn">Save as Preset</button>
      </div>
    </section>

    <section class="card grid" id="wallCountPanel" style="display:none;">
      <h3 style="grid-column:1/-1">Wall Count Test (4 x 400)</h3>
      <div>
        <label for="wall1Slider">Wall 1 Count</label>
        <input id="wall1Slider" type="range" min="0" max="400" step="1" />
        <input id="wall1Count" type="number" min="0" max="400" step="1" />
      </div>
      <div>
        <label for="wall2Slider">Wall 2 Count</label>
        <input id="wall2Slider" type="range" min="0" max="400" step="1" />
        <input id="wall2Count" type="number" min="0" max="400" step="1" />
      </div>
      <div>
        <label for="wall3Slider">Wall 3 Count</label>
        <input id="wall3Slider" type="range" min="0" max="400" step="1" />
        <input id="wall3Count" type="number" min="0" max="400" step="1" />
      </div>
      <div>
        <label for="wall4Slider">Wall 4 Count</label>
        <input id="wall4Slider" type="range" min="0" max="400" step="1" />
        <input id="wall4Count" type="number" min="0" max="400" step="1" />
      </div>
    </section>

    <section class="card" id="wifiPanel" style="display:none;">
      <div>
        <label for="wifiSsid">Home Wi-Fi SSID</label>
        <input id="wifiSsid" type="text" placeholder="Your router SSID" />
      </div>
      <div>
        <label for="wifiPassword">Home Wi-Fi Password</label>
        <input id="wifiPassword" type="password" placeholder="Your router password" />
      </div>
      <div>
        <button class="btn" id="wifiSaveBtn">Save and Connect</button>
      </div>
      <div>
        <button class="btn alt" id="wifiClearBtn">Clear Saved Wi-Fi</button>
      </div>
    </section>

    <section class="card">
      <div id="status" class="status-ok">Ready.</div>
    </section>
  </main>

  <script>
    const patterns = [
      [1, "Solid"],
        [2, "Lightning + Rain"],
        [3, "Lightning + Rain Alt"],
        [4, "Pong Chaser"],
        [5, "Flame"],
        [6, "Pac-Man"],
        [7, "Kaleidoscope"],
        [8, "Particle Mayhem"],
        [9, "Wall Count Test"],
        [10, "Neuro Prism"]
    ];

    const el = (id) => document.getElementById(id);
    const status = (msg, ok = true) => {
      const s = el("status");
      s.textContent = msg;
      s.className = ok ? "status-ok" : "";
    };

    function fillPatternSelect() {
      const sel = el("pattern");
      sel.innerHTML = "";
      patterns.forEach(([value, label]) => {
        const o = document.createElement("option");
        o.value = value;
        o.textContent = label;
        sel.appendChild(o);
      });
    }

    function setText(id, value) {
      el(id).textContent = value;
    }

    function updateBrightnessSliderLimit(pattern) {
      const brightness = el("brightness");
      const isAltPattern = Number(pattern) === 3;
      const maxValue = isAltPattern ? 80 : 255;
      brightness.max = String(maxValue);

      const current = Number(brightness.value || 0);
      if (current > maxValue) {
        brightness.value = String(maxValue);
        setText("brightnessVal", maxValue);
      }
    }

    function showPanels(pattern) {
      updateBrightnessSliderLimit(pattern);
      el("solidPanel").style.display = pattern === 1 ? "grid" : "none";
      // Classic rain controls only for original lightning pattern.
      el("rainPanel").style.display = pattern === 2 ? "grid" : "none";
      el("lightningAltPanel").style.display = pattern === 3 ? "grid" : "none";
      el("kaleidoscopePanel").style.display = pattern === 7 ? "grid" : "none";
      el("particleMayhemPanel").style.display = pattern === 8 ? "grid" : "none";
      el("flamePanel").style.display = pattern === 5 ? "grid" : "none";
      el("wallCountPanel").style.display = pattern === 9 ? "grid" : "none";
      el("neuroTripPanel").style.display = pattern === 10 ? "grid" : "none";
      el("presetsPanel").style.display = "grid";  // Always show presets
      refreshPresets();  // Refresh preset list for current pattern
    }

    async function api(path, method = "GET", body = null, contentType = null) {
      const options = { method };
      if (body !== null) {
        options.body = body;
        if (contentType) {
          options.headers = { "Content-Type": contentType };
        }
      }
      const res = await fetch(path, options);
      if (!res.ok) throw new Error(`${res.status} ${res.statusText}`);
      return res.json();
    }

    // Preset functions - defined here for scope access
    async function refreshPresets() {
      const pattern = parseInt(el("pattern").value);
      try {
        const data = await api(`/api/presets/list?pattern=${pattern}`);
        const container = el("presetSlots");
        container.innerHTML = "";
        
        data.presets.forEach(preset => {
          // Wrapper for preset and delete button
          const wrapper = document.createElement("div");
          wrapper.className = "preset-slot";
          
          const btn = document.createElement("button");
          btn.className = "btn preset-btn";
          btn.textContent = preset.name || `Empty ${preset.slot}`;
          btn.style.opacity = preset.exists ? "1" : "0.5";
          btn.style.cursor = preset.exists ? "pointer" : "default";
          
          if (preset.exists) {
            btn.addEventListener("click", async () => {
              try {
                await api(`/api/presets/load?slot=${preset.slot}&pattern=${pattern}`, "POST");
                await refresh();
                status(`Loaded: ${preset.name}`, true);
              } catch (err) {
                status(`Load failed: ${err.message}`, false);
              }
            });
            
            // Delete button
            const deleteBtn = document.createElement("button");
            deleteBtn.className = "btn alt preset-delete";
            deleteBtn.textContent = "✕";
            deleteBtn.title = "Delete this preset";
            deleteBtn.addEventListener("click", async (e) => {
              e.stopPropagation();
              if (confirm(`Delete "${preset.name}"?`)) {
                try {
                  await api(`/api/presets/delete?slot=${preset.slot}&pattern=${pattern}`, "POST");
                  status(`Deleted: ${preset.name}`, true);
                  await refreshPresets();
                } catch (err) {
                  status(`Delete failed: ${err.message}`, false);
                }
              }
            });
            
            wrapper.appendChild(btn);
            wrapper.appendChild(deleteBtn);
          } else {
            wrapper.classList.add("empty");
            wrapper.appendChild(btn);
          }
          container.appendChild(wrapper);
        });
      } catch (err) {
        status(`Presets failed: ${err.message}`, false);
      }
    }

    async function refresh() {
      try {
        const data = await api("/api/state");
        el("net").textContent = `${data.networkMode} @ ${data.ip}`;
        if (data.configuredSsid) {
          el("wifiSsid").value = data.configuredSsid;
        }

        el("pattern").value = data.currentPattern;
        el("solidColor").value = data.solidHex;
        el("brightness").value = data.brightness;
        setText("brightnessVal", data.brightness);

        el("rainChance").value = data.rainDropChance;
        el("rainFade").value = data.rainFadeAmount;
        el("rainDelay").value = data.rainUpdateDelay;
        setText("rainChanceVal", data.rainDropChance);
        setText("rainFadeVal", data.rainFadeAmount);
        setText("rainDelayVal", data.rainUpdateDelay);

        el("flameCool").value = data.flameCool;
        el("flameSparking").value = data.flameSparking;
        el("flameDelay").value = data.flameUpdateDelay;
        el("flamePalette").value = data.flamePaletteMode;
        el("flameBgLevel").value = data.flameBgEmberLevel;
        setText("flameCoolVal", data.flameCool);
        setText("flameSparkingVal", data.flameSparking);
        setText("flameDelayVal", data.flameUpdateDelay);
        setText("flameBgLevelVal", data.flameBgEmberLevel);

        el("lightningMinInterval").value = Math.round(data.lightningMinInterval / 1000);
        el("lightningMaxInterval").value = Math.round(data.lightningMaxInterval / 1000);
        el("lightningFlashBrightness").value = data.lightningFlashBrightness;
        el("lightningSpeed").value = data.lightningSpeed;
        el("lightningMinDuration").value = data.lightningMinDuration;
        el("lightningMaxDuration").value = data.lightningMaxDuration;
        el("lightningStrikePin").checked = !!data.lightningStrikePin;
        el("lightningStrikePinChance").value = data.lightningStrikePinChance;
        el("lightningStrikePinSpeed").value = data.lightningStrikePinSpeed;
        el("lightningTestMode").checked = !!data.lightningTestMode;
        el("lightningTestDelay").value = data.lightningTestDelay;
        el("rainSpawnChance").value = data.rainSpawnChance;
        el("rainFadeSpeed").value = data.rainFadeSpeed;
        el("rainUpdateSpeed").value = data.rainUpdateSpeed;
        el("rainDropBrightness").value = data.rainDropBrightness;
        el("rainTrailChance").value = data.rainTrailChance;
        el("rainPeakDuration").value = data.rainPeakDuration;
        el("rainColorMode").value = data.rainColorMode;
        el("kaleidoscopeSpeed").value = data.kaleidoscopeSpeed;
        el("particleMayhemSpeed").value = data.particleMayhemSpeed;
        el("particleMayhemSpeedBoost").value = data.particleMayhemSpeedBoost;
        el("particleMayhemRandomSpeed").value = data.particleMayhemRandomSpeed;
        el("particleMayhemParticleCount").value = data.particleMayhemParticleCount;
        el("particleMayhemDensity").value = data.particleMayhemDensity;
        el("particleMayhemExplosionSize").value = data.particleMayhemExplosionSize;
        el("particleMayhemExplosionSpeed").value = data.particleMayhemExplosionSpeed;
        el("neuroTripSpeed").value = data.neuroTripSpeed;
        el("neuroTripDepth").value = data.neuroTripDepth;
        el("neuroTripPulse").value = data.neuroTripPulse;

        el("wall1Slider").value = data.wall1Count;
        el("wall2Slider").value = data.wall2Count;
        el("wall3Slider").value = data.wall3Count;
        el("wall4Slider").value = data.wall4Count;
        el("wall1Count").value = data.wall1Count;
        el("wall2Count").value = data.wall2Count;
        el("wall3Count").value = data.wall3Count;
        el("wall4Count").value = data.wall4Count;

        setText("lightningMinIntervalVal", Math.round(data.lightningMinInterval / 1000));
        setText("lightningMaxIntervalVal", Math.round(data.lightningMaxInterval / 1000));
        setText("lightningFlashBrightnessVal", data.lightningFlashBrightness);
        setText("lightningSpeedVal", data.lightningSpeed);
        setText("lightningMinDurationVal", data.lightningMinDuration);
        setText("lightningMaxDurationVal", data.lightningMaxDuration);
        setText("lightningStrikePinChanceVal", data.lightningStrikePinChance);
        setText("lightningStrikePinSpeedVal", data.lightningStrikePinSpeed);
        setText("lightningTestDelayVal", data.lightningTestDelay);
        setText("rainSpawnChanceVal", data.rainSpawnChance);
        setText("rainFadeSpeedVal", data.rainFadeSpeed);
        setText("rainUpdateSpeedVal", data.rainUpdateSpeed);
        setText("rainDropBrightnessVal", data.rainDropBrightness);
        setText("rainTrailChanceVal", data.rainTrailChance);
        setText("rainPeakDurationVal", data.rainPeakDuration);
        setText("kaleidoscopeSpeedVal", data.kaleidoscopeSpeed);
        setText("particleMayhemSpeedVal", data.particleMayhemSpeed);
        setText("particleMayhemSpeedBoostVal", data.particleMayhemSpeedBoost);
        setText("particleMayhemRandomSpeedVal", data.particleMayhemRandomSpeed);
        setText("particleMayhemParticleCountVal", data.particleMayhemParticleCount);
        setText("particleMayhemDensityVal", data.particleMayhemDensity);
        setText("particleMayhemExplosionSizeVal", data.particleMayhemExplosionSize);
        setText("particleMayhemExplosionSpeedVal", data.particleMayhemExplosionSpeed);
        setText("neuroTripSpeedVal", data.neuroTripSpeed);
        setText("neuroTripDepthVal", data.neuroTripDepth);
        setText("neuroTripPulseVal", data.neuroTripPulse);

        showPanels(Number(data.currentPattern));
        if (data.networkMode !== "STA" && data.wifiLastError && data.wifiLastError !== "NONE") {
          status(`Wi-Fi: ${data.wifiLastError}`, false);
        } else {
          status("State synced.");
        }
      } catch (err) {
        status(`Refresh failed: ${err.message}`, false);
      }
    }

    async function bindControls() {
      el("pattern").addEventListener("change", async (e) => {
        try {
          await api(`/api/pattern?value=${encodeURIComponent(e.target.value)}`, "POST");
          showPanels(Number(e.target.value));
          status("Pattern updated.");
        } catch (err) {
          status(`Pattern update failed: ${err.message}`, false);
        }
      });

      el("brightness").addEventListener("input", (e) => {
        setText("brightnessVal", e.target.value);
      });
      el("brightness").addEventListener("change", async (e) => {
        try {
          await api(`/api/brightness?value=${encodeURIComponent(e.target.value)}`, "POST");
          status("Brightness updated.");
        } catch (err) {
          status(`Brightness update failed: ${err.message}`, false);
        }
      });

      el("powerBtn").addEventListener("click", async () => {
        try {
          await api("/api/power", "POST");
          await refresh();
          status("Power toggled.");
        } catch (err) {
          status(`Power toggle failed: ${err.message}`, false);
        }
      });

      el("solidApplyBtn").addEventListener("click", async () => {
        try {
          const hex = el("solidColor").value.replace("#", "");
          await api(`/api/solid?hex=${encodeURIComponent(hex)}`, "POST");
          status("Solid color updated.");
        } catch (err) {
          status(`Solid color update failed: ${err.message}`, false);
        }
      });

      // Add event listeners for color presets
      document.querySelectorAll(".color-preset").forEach((btn) => {
        btn.addEventListener("click", async () => {
          try {
            const hex = btn.getAttribute("data-color").replace("#", "");
            el("solidColor").value = "#" + hex;
            await api(`/api/solid?hex=${encodeURIComponent(hex)}`, "POST");
            status(`Color ${btn.getAttribute("title")} applied.`);
          } catch (err) {
            status(`Solid color update failed: ${err.message}`, false);
          }
        });
      });

      const rainPush = async () => {
        const chance = el("rainChance").value;
        const fade = el("rainFade").value;
        const delay = el("rainDelay").value;
        setText("rainChanceVal", chance);
        setText("rainFadeVal", fade);
        setText("rainDelayVal", delay);
        await api(`/api/rain?chance=${chance}&fade=${fade}&delay=${delay}`, "POST");
      };

      ["rainChance", "rainFade", "rainDelay"].forEach((id) => {
        el(id).addEventListener("change", async () => {
          try {
            await rainPush();
            status("Rain settings updated.");
          } catch (err) {
            status(`Rain update failed: ${err.message}`, false);
          }
        });
      });

      const flamePush = async () => {
        const cool = el("flameCool").value;
        const sparking = el("flameSparking").value;
        const delay = el("flameDelay").value;
        const palette = el("flamePalette").value;
        const bg = el("flameBgLevel").value;
        setText("flameCoolVal", cool);
        setText("flameSparkingVal", sparking);
        setText("flameDelayVal", delay);
        setText("flameBgLevelVal", bg);
        await api(`/api/flame?cool=${cool}&sparking=${sparking}&delay=${delay}&palette=${palette}&bg=${bg}`, "POST");
      };

      ["flameCool", "flameSparking", "flameDelay", "flamePalette", "flameBgLevel"].forEach((id) => {
        el(id).addEventListener("change", async () => {
          try {
            await flamePush();
            status("Flame settings updated.");
          } catch (err) {
            status(`Flame update failed: ${err.message}`, false);
          }
        });
      });

      el("rainReset").addEventListener("click", async () => {
        try {
          await api("/api/rain/reset", "POST");
          await refresh();
        } catch (err) {
          status(`Rain reset failed: ${err.message}`, false);
        }
      });

      el("flameReset").addEventListener("click", async () => {
        try {
          await api("/api/flame/reset", "POST");
          await refresh();
        } catch (err) {
          status(`Flame reset failed: ${err.message}`, false);
        }
      });

      // Lightning Alt settings
      const lightningAltPush = async () => {
        const minIntervalSec = parseInt(el("lightningMinInterval").value, 10);
        const maxIntervalSec = parseInt(el("lightningMaxInterval").value, 10);
        const minInterval = minIntervalSec * 1000;
        const maxInterval = maxIntervalSec * 1000;
        const flashBrightness = el("lightningFlashBrightness").value;
        const speed = el("lightningSpeed").value;
        const minDuration = el("lightningMinDuration").value;
        const maxDuration = el("lightningMaxDuration").value;
        const strikePin = el("lightningStrikePin").checked ? 1 : 0;
        const strikePinChance = el("lightningStrikePinChance").value;
        const strikePinSpeed = el("lightningStrikePinSpeed").value;
        const testMode = el("lightningTestMode").checked ? 1 : 0;
        const testDelay = el("lightningTestDelay").value;
        const spawnChance = el("rainSpawnChance").value;
        const fadeSpeed = el("rainFadeSpeed").value;
        const updateSpeed = el("rainUpdateSpeed").value;
        const brightness = el("rainDropBrightness").value;
        const trailChance = el("rainTrailChance").value;
        const peakDuration = el("rainPeakDuration").value;

        setText("lightningMinIntervalVal", minIntervalSec);
        setText("lightningMaxIntervalVal", maxIntervalSec);
        setText("lightningFlashBrightnessVal", flashBrightness);
        setText("lightningSpeedVal", speed);
        setText("lightningMinDurationVal", minDuration);
        setText("lightningMaxDurationVal", maxDuration);
        setText("lightningStrikePinChanceVal", strikePinChance);
        setText("lightningStrikePinSpeedVal", strikePinSpeed);
        setText("lightningTestDelayVal", testDelay);
        setText("rainSpawnChanceVal", spawnChance);
        setText("rainFadeSpeedVal", fadeSpeed);
        setText("rainUpdateSpeedVal", updateSpeed);
        setText("rainDropBrightnessVal", brightness);
        setText("rainTrailChanceVal", trailChance);
        setText("rainPeakDurationVal", peakDuration);

        await api(`/api/lightning-alt?minInterval=${minInterval}&maxInterval=${maxInterval}&flashBrightness=${flashBrightness}&speed=${speed}&minDuration=${minDuration}&maxDuration=${maxDuration}&strikePin=${strikePin}&strikePinChance=${strikePinChance}&strikePinSpeed=${strikePinSpeed}&testMode=${testMode}&testDelay=${testDelay}&spawnChance=${spawnChance}&fadeSpeed=${fadeSpeed}&updateSpeed=${updateSpeed}&brightness=${brightness}&trailChance=${trailChance}&peakDuration=${peakDuration}`, "POST");
      };

      ["lightningMinInterval", "lightningMaxInterval", "lightningFlashBrightness", "lightningSpeed", "lightningMinDuration", "lightningMaxDuration", "lightningStrikePinChance", "lightningStrikePinSpeed", "lightningTestDelay", "rainSpawnChance", "rainFadeSpeed", "rainUpdateSpeed", "rainDropBrightness", "rainTrailChance", "rainPeakDuration"].forEach((id) => {
        el(id).addEventListener("input", (e) => {
          setText(id + "Val", e.target.value);
        });
        el(id).addEventListener("change", async () => {
          try {
            await lightningAltPush();
            status("Lightning Alt settings updated.");
          } catch (err) {
            status(`Lightning Alt update failed: ${err.message}`, false);
          }
        });
      });

      el("lightningTestMode").addEventListener("change", async () => {
        try {
          await lightningAltPush();
          status("Lightning Alt settings updated.");
        } catch (err) {
          status(`Lightning Alt update failed: ${err.message}`, false);
        }
      });

      el("lightningStrikePin").addEventListener("change", async () => {
        try {
          await lightningAltPush();
          status("Lightning Alt settings updated.");
        } catch (err) {
          status(`Lightning Alt update failed: ${err.message}`, false);
        }
      });

      el("lightningAltReset").addEventListener("click", async () => {
        try {
          await api("/api/lightning-alt/reset", "POST");
          await refresh();
        } catch (err) {
          status(`Lightning Alt reset failed: ${err.message}`, false);
        }
      });

      el("rainColorMode").addEventListener("change", async (e) => {
        try {
          await api(`/api/rain-color-mode?mode=${e.target.value}`, "POST");
          status(`Rain color mode changed to: ${e.target.options[e.target.selectedIndex].text}`);
        } catch (err) {
          status(`Rain color mode change failed: ${err.message}`, false);
        }
      });

      el("kaleidoscopeSpeed").addEventListener("input", (e) => {
        setText("kaleidoscopeSpeedVal", e.target.value);
      });

      el("kaleidoscopeSpeed").addEventListener("change", async (e) => {
        try {
          await api(`/api/kaleidoscope?speed=${encodeURIComponent(e.target.value)}`, "POST");
          status("Kaleidoscope speed updated.");
        } catch (err) {
          status(`Kaleidoscope update failed: ${err.message}`, false);
        }
      });

      el("kaleidoscopeReset").addEventListener("click", async () => {
        try {
          await api("/api/kaleidoscope/reset", "POST");
          await refresh();
        } catch (err) {
          status(`Kaleidoscope reset failed: ${err.message}`, false);
        }
      });

      el("particleMayhemSpeed").addEventListener("input", (e) => {
        setText("particleMayhemSpeedVal", e.target.value);
      });

      el("particleMayhemSpeedBoost").addEventListener("input", (e) => {
        setText("particleMayhemSpeedBoostVal", e.target.value);
      });

      el("particleMayhemRandomSpeed").addEventListener("input", (e) => {
        setText("particleMayhemRandomSpeedVal", e.target.value);
      });

      el("particleMayhemParticleCount").addEventListener("input", (e) => {
        setText("particleMayhemParticleCountVal", e.target.value);
      });

      el("particleMayhemDensity").addEventListener("input", (e) => {
        setText("particleMayhemDensityVal", e.target.value);
      });

      el("particleMayhemExplosionSize").addEventListener("input", (e) => {
        setText("particleMayhemExplosionSizeVal", e.target.value);
      });

      el("particleMayhemExplosionSpeed").addEventListener("input", (e) => {
        setText("particleMayhemExplosionSpeedVal", e.target.value);
      });

      const particleMayhemPush = async () => {
        const speed = el("particleMayhemSpeed").value;
        const speedBoost = el("particleMayhemSpeedBoost").value;
        const randomSpeed = el("particleMayhemRandomSpeed").value;
        const particleCount = el("particleMayhemParticleCount").value;
        const density = el("particleMayhemDensity").value;
        const explosion = el("particleMayhemExplosionSize").value;
        const explosionSpeed = el("particleMayhemExplosionSpeed").value;

        setText("particleMayhemSpeedVal", speed);
        setText("particleMayhemSpeedBoostVal", speedBoost);
        setText("particleMayhemRandomSpeedVal", randomSpeed);
        setText("particleMayhemParticleCountVal", particleCount);
        setText("particleMayhemDensityVal", density);
        setText("particleMayhemExplosionSizeVal", explosion);
        setText("particleMayhemExplosionSpeedVal", explosionSpeed);

        await api(`/api/particle-mayhem?speed=${encodeURIComponent(speed)}&speedBoost=${encodeURIComponent(speedBoost)}&randomSpeed=${encodeURIComponent(randomSpeed)}&particleCount=${encodeURIComponent(particleCount)}&density=${encodeURIComponent(density)}&explosion=${encodeURIComponent(explosion)}&explosionSpeed=${encodeURIComponent(explosionSpeed)}`, "POST");
      };

      el("particleMayhemSpeed").addEventListener("change", async (e) => {
        try {
          await particleMayhemPush();
          status("Particle Mayhem settings updated.");
        } catch (err) {
          status(`Particle Mayhem update failed: ${err.message}`, false);
        }
      });

      el("particleMayhemSpeedBoost").addEventListener("change", async () => {
        try {
          await particleMayhemPush();
          status("Particle Mayhem settings updated.");
        } catch (err) {
          status(`Particle Mayhem update failed: ${err.message}`, false);
        }
      });

      el("particleMayhemDensity").addEventListener("change", async () => {
        try {
          await particleMayhemPush();
          status("Particle Mayhem settings updated.");
        } catch (err) {
          status(`Particle Mayhem update failed: ${err.message}`, false);
        }
      });

      el("particleMayhemRandomSpeed").addEventListener("change", async () => {
        try {
          await particleMayhemPush();
          status("Particle Mayhem settings updated.");
        } catch (err) {
          status(`Particle Mayhem update failed: ${err.message}`, false);
        }
      });

      el("particleMayhemParticleCount").addEventListener("change", async () => {
        try {
          await particleMayhemPush();
          status("Particle Mayhem settings updated.");
        } catch (err) {
          status(`Particle Mayhem update failed: ${err.message}`, false);
        }
      });

      el("particleMayhemExplosionSize").addEventListener("change", async () => {
        try {
          await particleMayhemPush();
          status("Particle Mayhem settings updated.");
        } catch (err) {
          status(`Particle Mayhem update failed: ${err.message}`, false);
        }
      });

      el("particleMayhemExplosionSpeed").addEventListener("change", async () => {
        try {
          await particleMayhemPush();
          status("Particle Mayhem settings updated.");
        } catch (err) {
          status(`Particle Mayhem update failed: ${err.message}`, false);
        }
      });

      el("particleMayhemReset").addEventListener("click", async () => {
        try {
          await api("/api/particle-mayhem/reset", "POST");
          await refresh();
        } catch (err) {
          status(`Particle Mayhem reset failed: ${err.message}`, false);
        }
      });

      el("neuroTripSpeed").addEventListener("input", (e) => {
        setText("neuroTripSpeedVal", e.target.value);
      });

      el("neuroTripDepth").addEventListener("input", (e) => {
        setText("neuroTripDepthVal", e.target.value);
      });

      el("neuroTripPulse").addEventListener("input", (e) => {
        setText("neuroTripPulseVal", e.target.value);
      });

      const neuroTripPush = async () => {
        const speed = el("neuroTripSpeed").value;
        const depth = el("neuroTripDepth").value;
        const pulse = el("neuroTripPulse").value;

        setText("neuroTripSpeedVal", speed);
        setText("neuroTripDepthVal", depth);
        setText("neuroTripPulseVal", pulse);

        await api(`/api/neuro-trip?speed=${encodeURIComponent(speed)}&depth=${encodeURIComponent(depth)}&pulse=${encodeURIComponent(pulse)}`, "POST");
      };

      ["neuroTripSpeed", "neuroTripDepth", "neuroTripPulse"].forEach((id) => {
        el(id).addEventListener("change", async () => {
          try {
            await neuroTripPush();
            status("Neuro Prism settings updated.");
          } catch (err) {
            status(`Neuro Prism update failed: ${err.message}`, false);
          }
        });
      });

      el("neuroTripReset").addEventListener("click", async () => {
        try {
          await api("/api/neuro-trip/reset", "POST");
          await refresh();
        } catch (err) {
          status(`Neuro Prism reset failed: ${err.message}`, false);
        }
      });

      const syncWallPair = (sliderId, inputId) => {
        const slider = el(sliderId);
        const input = el(inputId);
        slider.addEventListener("input", () => {
          input.value = slider.value;
        });
        input.addEventListener("input", () => {
          let v = Number(input.value);
          if (Number.isNaN(v)) v = 0;
          v = Math.max(0, Math.min(400, v));
          input.value = v;
          slider.value = v;
        });
      };

      syncWallPair("wall1Slider", "wall1Count");
      syncWallPair("wall2Slider", "wall2Count");
      syncWallPair("wall3Slider", "wall3Count");
      syncWallPair("wall4Slider", "wall4Count");

      const wallCountPush = async () => {
        const w1 = el("wall1Slider").value;
        const w2 = el("wall2Slider").value;
        const w3 = el("wall3Slider").value;
        const w4 = el("wall4Slider").value;
        await api(`/api/wall-count?wall1=${w1}&wall2=${w2}&wall3=${w3}&wall4=${w4}`, "POST");
      };

      ["wall1Slider", "wall2Slider", "wall3Slider", "wall4Slider", "wall1Count", "wall2Count", "wall3Count", "wall4Count"].forEach((id) => {
        el(id).addEventListener("change", async () => {
          try {
            await wallCountPush();
            status("Wall counts updated.");
          } catch (err) {
            status(`Wall count update failed: ${err.message}`, false);
          }
        });
      });

      el("savePresetBtn").addEventListener("click", async () => {
        const name = el("presetName").value || "Preset";
        const pattern = parseInt(el("pattern").value);
        
        // Find first empty slot
        let slot = 0;
        const data = await api(`/api/presets/list?pattern=${pattern}`);
        for (let i = 0; i < data.presets.length; i++) {
          if (!data.presets[i].exists) {
            slot = i;
            break;
          }
        }
        
        try {
          const formData = new FormData();
          formData.append("slot", slot);
          formData.append("name", name);
          
          await fetch(`/api/presets/save?slot=${slot}&pattern=${pattern}`, {
            method: "POST",
            body: formData
          });
          
          el("presetName").value = "";
          await refreshPresets();
          status(`Saved: ${name}`, true);
        } catch (err) {
          status(`Save failed: ${err.message}`, false);
        }
      });

      el("refreshBtn").addEventListener("click", refresh);

      el("wifiToggleBtn").addEventListener("click", () => {
        const panel = el("wifiPanel");
        panel.style.display = panel.style.display === "none" ? "grid" : "none";
      });

      el("wifiSaveBtn").addEventListener("click", async () => {
        try {
          const ssid = el("wifiSsid").value.trim();
          const password = el("wifiPassword").value;
          if (!ssid) {
            status("Enter an SSID first.", false);
            return;
          }
          const body = new URLSearchParams({ ssid, password }).toString();
          const data = await api("/api/wifi/set", "POST", body, "application/x-www-form-urlencoded");
          if (data.networkMode === "STA") {
            status("Wi-Fi connected. You can switch to your home network now.");
          } else {
            status(`Wi-Fi connect failed: ${data.wifiLastError || "unknown"}`, false);
          }
          setTimeout(refresh, 1800);
          setTimeout(refresh, 5000);
        } catch (err) {
          status(`Wi-Fi save failed: ${err.message}`, false);
        }
      });

      el("wifiClearBtn").addEventListener("click", async () => {
        try {
          await api("/api/wifi/clear", "POST");
          el("wifiSsid").value = "";
          el("wifiPassword").value = "";
          status("Saved Wi-Fi cleared.");
          setTimeout(refresh, 1000);
        } catch (err) {
          status(`Wi-Fi clear failed: ${err.message}`, false);
        }
      });
    }

    fillPatternSelect();
    bindControls();
    refresh();
    setInterval(refresh, 4000);
  </script>
</body>
</html>
)rawliteral";

enum mode{
  IDLE,
  RUNNING,
};
enum Current_Pattern{
  NONE,
  SOLID,
  LIGHTNING_W_RAIN,
  LIGHTNING_W_RAIN_ALT,
  PONG_CHASER,
  FLAME,
  PAC_MAN,
  KALEIDOSCOPE,
  RAINBOW_WAVES,
  WALL_COUNT_TEST,
  NEURO_TRIP
};

uint8_t mode = 0;
const uint8_t ALT_PATTERN_BRIGHTNESS_MAX = 80;
uint8_t BRIGHTNESS = 10;
uint8_t LastBrightness = 0;
uint8_t Current_Pattern = SOLID;
CRGB solidColor = CRGB::Blue;
// Lightning effect parameters
unsigned long lastStrikeTime = 0;
unsigned long nextStrikeInterval = 10000;  // Initial delay between lightning
// Rain effect paramaters
uint8_t rainDropChance = 0; // % chance per frame to spawn new drop
uint8_t rainFadeAmount = 10; // how fast drops fade
uint16_t rainUpdateDelay = 50;
// Flame effect parameters
uint8_t flameCool = 2;           // cooling: lower = more fire-like
uint8_t flameSparking = 200;      // sparking: higher = more sparkly
uint16_t flameUpdateDelay = 25;   // ms between updates
uint8_t flameClumpMinSize = 3;   // minimum clump width
uint8_t flameClumpMaxSize = 16;  // maximum clump width
uint8_t flamePaletteMode = 0;    // 0=classic,1=neon,2=cool blue,3=arctic white,4=warm white
uint8_t flameBgEmberLevel = 7;   // ember floor intensity (0-30)
uint8_t kaleidoscopeSpeedPercent = 100;
uint8_t rainbowWavesSpeedPercent = 100;
uint8_t particleMayhemSpeedBoost = 35;
uint8_t particleMayhemDensity = 4;
uint8_t particleMayhemExplosionSize = 7;
uint8_t particleMayhemExplosionSpeed = 6;
uint8_t particleMayhemRandomSpeed = 55;
uint8_t particleMayhemParticleCount = 5;
uint8_t neuroTripSpeedPercent = 105;
uint8_t neuroTripDepth = 180;
uint8_t neuroTripPulse = 150;
// Static Light parameters

// ============================================================
// PRESET SYSTEM - Save/Load effect configurations
// ============================================================
#define MAX_PRESETS 5

struct EffectPreset {
  char name[32];
  uint8_t patternType;
  // Lightning Alt parameters
  uint32_t lightningAltMinInterval;
  uint32_t lightningAltMaxInterval;
  uint8_t lightningAltFlashBrightness;
  uint8_t lightningAltSpeedPercent;
  uint16_t lightningAltMinFlashDuration;
  uint16_t lightningAltMaxFlashDuration;
  uint8_t lightningAltStrikePinEnabled;
  uint8_t lightningAltTestMode;
  uint16_t lightningAltTestInterval;
  uint8_t rainAltSpawnChance;
  uint8_t rainAltFadeAmount;
  uint16_t rainAltUpdateDelay;
  uint8_t rainAltBrightness;
  uint8_t rainAltTrailChance;
  uint16_t rainAltPeakDuration;
  uint8_t rainAltColorMode;
  // Flame parameters
  uint8_t flameCool;
  uint8_t flameSparking;
  uint16_t flameUpdateDelay;
  uint8_t flameClumpMinSize;
  uint8_t flameClumpMaxSize;
  uint8_t flamePaletteMode;
  uint8_t flameBgEmberLevel;
  // Solid color
  uint8_t solidColorR;
  uint8_t solidColorG;
  uint8_t solidColorB;
};

// Save preset to NVS
void savePreset(uint8_t slotNum, const String& presetName, uint8_t patternType) {
  if (slotNum >= MAX_PRESETS) return;

  EffectPreset preset;
  memset(&preset, 0, sizeof(preset));
  
  strncpy(preset.name, presetName.c_str(), sizeof(preset.name) - 1);
  preset.patternType = patternType;
  
  // Capture current parameters based on pattern
  if (patternType == LIGHTNING_W_RAIN_ALT) {
    preset.lightningAltMinInterval = lightningAltMinInterval;
    preset.lightningAltMaxInterval = lightningAltMaxInterval;
    preset.lightningAltFlashBrightness = lightningAltFlashBrightness;
    preset.lightningAltSpeedPercent = lightningAltSpeedPercent;
    preset.lightningAltMinFlashDuration = lightningAltMinFlashDuration;
    preset.lightningAltMaxFlashDuration = lightningAltMaxFlashDuration;
    preset.lightningAltStrikePinEnabled = lightningAltStrikePinEnabled ? 1 : 0;
    preset.lightningAltTestMode = lightningAltTestMode ? 1 : 0;
    preset.lightningAltTestInterval = lightningAltTestInterval;
    preset.rainAltSpawnChance = rainAltSpawnChance;
    preset.rainAltFadeAmount = rainAltFadeAmount;
    preset.rainAltUpdateDelay = rainAltUpdateDelay;
    preset.rainAltBrightness = rainAltBrightness;
    preset.rainAltTrailChance = rainAltTrailChance;
    preset.rainAltPeakDuration = rainAltPeakDuration;
    preset.rainAltColorMode = rainAltColorMode;
  } else if (patternType == FLAME) {
    preset.flameCool = flameCool;
    preset.flameSparking = flameSparking;
    preset.flameUpdateDelay = flameUpdateDelay;
    preset.flameClumpMinSize = flameClumpMinSize;
    preset.flameClumpMaxSize = flameClumpMaxSize;
    preset.flamePaletteMode = flamePaletteMode;
    preset.flameBgEmberLevel = flameBgEmberLevel;
  } else if (patternType == SOLID) {
    preset.solidColorR = solidColor.r;
    preset.solidColorG = solidColor.g;
    preset.solidColorB = solidColor.b;
  }
  
  // Save to NVS
  char keyName[20];
  snprintf(keyName, sizeof(keyName), "prst_%d_%d", patternType, slotNum);
  
  preferences.begin("presets", false);
  preferences.putBytes(keyName, (uint8_t *)&preset, sizeof(preset));
  preferences.end();
  
  Serial.printf("Preset saved: %s\n", presetName.c_str());
}

// Load preset from NVS
bool loadPreset(uint8_t slotNum, uint8_t patternType) {
  if (slotNum >= MAX_PRESETS) return false;

  char keyName[20];
  snprintf(keyName, sizeof(keyName), "prst_%d_%d", patternType, slotNum);
  
  preferences.begin("presets", true);
  size_t size = preferences.getBytesLength(keyName);
  preferences.end();
  
  if (size == 0 || size > sizeof(EffectPreset)) return false;

  EffectPreset preset;
  memset(&preset, 0, sizeof(preset));
  preferences.begin("presets", true);
  preferences.getBytes(keyName, (uint8_t *)&preset, size);
  preferences.end();

  // Apply preset parameters
  if (preset.patternType == LIGHTNING_W_RAIN_ALT) {
    lightningAltMinInterval = preset.lightningAltMinInterval;
    lightningAltMaxInterval = preset.lightningAltMaxInterval;
    lightningAltFlashBrightness = preset.lightningAltFlashBrightness;
    lightningAltSpeedPercent = preset.lightningAltSpeedPercent == 0 ? 100 : preset.lightningAltSpeedPercent;
    lightningAltMinFlashDuration = preset.lightningAltMinFlashDuration;
    lightningAltMaxFlashDuration = preset.lightningAltMaxFlashDuration;
    lightningAltStrikePinEnabled = preset.lightningAltStrikePinEnabled != 0;
    lightningAltTestMode = preset.lightningAltTestMode != 0;
    lightningAltTestInterval = preset.lightningAltTestInterval;
    rainAltSpawnChance = preset.rainAltSpawnChance;
    rainAltFadeAmount = preset.rainAltFadeAmount;
    rainAltUpdateDelay = preset.rainAltUpdateDelay;
    rainAltBrightness = preset.rainAltBrightness;
    rainAltTrailChance = preset.rainAltTrailChance;
    rainAltPeakDuration = preset.rainAltPeakDuration;
    rainAltColorMode = preset.rainAltColorMode;
  } else if (preset.patternType == FLAME) {
    flameCool = preset.flameCool;
    flameSparking = preset.flameSparking;
    flameUpdateDelay = preset.flameUpdateDelay;
    flameClumpMinSize = preset.flameClumpMinSize;
    flameClumpMaxSize = (uint8_t)constrain((int)preset.flameClumpMaxSize, flameClumpMinSize, 16);
    if (size < sizeof(EffectPreset)) {
      flamePaletteMode = 0;
      flameBgEmberLevel = 7;
    } else {
      flamePaletteMode = (uint8_t)constrain((int)preset.flamePaletteMode, 0, 4);
      flameBgEmberLevel = (uint8_t)constrain((int)preset.flameBgEmberLevel, 0, 30);
    }
  } else if (preset.patternType == SOLID) {
    solidColor = CRGB(preset.solidColorR, preset.solidColorG, preset.solidColorB);
  }
  
  Serial.printf("Preset loaded: %s\n", preset.name);
  return true;
}

// Get preset name, returns empty string if slot empty
String getPresetName(uint8_t slotNum, uint8_t patternType) {
  if (slotNum >= MAX_PRESETS) return "";

  char keyName[20];
  snprintf(keyName, sizeof(keyName), "prst_%d_%d", patternType, slotNum);
  
  preferences.begin("presets", true);
  size_t size = preferences.getBytesLength(keyName);
  preferences.end();
  
  if (size != sizeof(EffectPreset)) return "";

  EffectPreset preset;
  preferences.begin("presets", true);
  preferences.getBytes(keyName, (uint8_t *)&preset, sizeof(preset));
  preferences.end();

  return String(preset.name);
}

// Delete preset from NVS
void deletePreset(uint8_t slotNum, uint8_t patternType) {
  char keyName[20];
  snprintf(keyName, sizeof(keyName), "prst_%d_%d", patternType, slotNum);
  
  preferences.begin("presets", false);
  preferences.remove(keyName);
  preferences.end();
  
  Serial.printf("Preset deleted: slot %d\n", slotNum);
}

String networkMode = "BOOT";
String wifiLastError = "NONE";
bool usingStationMode = false;
unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL_MS = 10000;

void writeBounded(char* dst, size_t dstSize, const String& src) {
  if (dstSize == 0) {
    return;
  }
  strncpy(dst, src.c_str(), dstSize - 1);
  dst[dstSize - 1] = '\0';
}

bool hasUsableCompileTimeCredentials() {
  return strlen(WIFI_SSID) > 0;
}

String colorToHex(const CRGB& c) {
  char hex[8];
  snprintf(hex, sizeof(hex), "#%02X%02X%02X", c.r, c.g, c.b);
  return String(hex);
}

CRGB parseHexColor(String hex) {
  hex.trim();
  if (hex.startsWith("#")) {
    hex = hex.substring(1);
  }

  if (hex.length() != 6) {
    return solidColor;
  }

  long value = strtol(hex.c_str(), nullptr, 16);
  return CRGB((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
}

void loadStoredWifiCredentials() {
  preferences.begin("wifi", true);
  String savedSsid = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");
  preferences.end();

  if (savedSsid.length() > 0) {
    writeBounded(configuredWifiSsid, sizeof(configuredWifiSsid), savedSsid);
    writeBounded(configuredWifiPassword, sizeof(configuredWifiPassword), savedPassword);
    return;
  }

  if (hasUsableCompileTimeCredentials()) {
    writeBounded(configuredWifiSsid, sizeof(configuredWifiSsid), String(WIFI_SSID));
    writeBounded(configuredWifiPassword, sizeof(configuredWifiPassword), String(WIFI_PASSWORD));
  }
}

void persistWifiCredentials(const String& ssid, const String& password) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
  writeBounded(configuredWifiSsid, sizeof(configuredWifiSsid), ssid);
  writeBounded(configuredWifiPassword, sizeof(configuredWifiPassword), password);
}

void clearWifiCredentials() {
  preferences.begin("wifi", false);
  preferences.remove("ssid");
  preferences.remove("password");
  preferences.end();
  configuredWifiSsid[0] = '\0';
  configuredWifiPassword[0] = '\0';
}

int getRequestParamInt(const char* name, int fallback) {
  if (server.hasArg(name)) {
    return server.arg(name).toInt();
  }
  return fallback;
}

void setPatternValue(uint8_t nextPattern) {
  Current_Pattern = nextPattern;
  if (Current_Pattern == LIGHTNING_W_RAIN || Current_Pattern == LIGHTNING_W_RAIN_ALT) {
    clearLeds();
  }
}

void togglePowerMode() {
  if (mode == RUNNING) {
    LastBrightness = BRIGHTNESS;
    BRIGHTNESS = 0;
    mode = IDLE;
  } else {
    BRIGHTNESS = LastBrightness;
    mode = RUNNING;
  }
}

String activeIpAddress() {
  if (usingStationMode) {
    return WiFi.localIP().toString();
  }
  return WiFi.softAPIP().toString();
}

const char* wifiStatusToText(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS: return "IDLE";
    case WL_NO_SSID_AVAIL: return "SSID_NOT_FOUND";
    case WL_SCAN_COMPLETED: return "SCAN_COMPLETED";
    case WL_CONNECTED: return "CONNECTED";
    case WL_CONNECT_FAILED: return "AUTH_FAILED";
    case WL_CONNECTION_LOST: return "CONNECTION_LOST";
    case WL_DISCONNECTED: return "DISCONNECTED";
    default: return "UNKNOWN";
  }
}

void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  networkMode = "AP";
  usingStationMode = false;
  Serial.println();
  Serial.print("Access Point started. SSID: ");
  Serial.println(AP_SSID);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

bool connectStation(const char* ssid, const char* password, unsigned long timeoutMs = 10000) {
  if (strlen(ssid) == 0) {
    wifiLastError = "SSID_EMPTY";
    return false;
  }

  WiFi.disconnect(true, true);
  delay(150);
  WiFi.mode(WIFI_STA);
  wifiLastError = "CONNECTING";
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < timeoutMs) {
    delay(250);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    networkMode = "STA";
    usingStationMode = true;
    wifiLastError = "NONE";
    Serial.println();
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Connected to SSID: ");
    Serial.println(ssid);
    return true;
  }

  wl_status_t finalStatus = WiFi.status();
  wifiLastError = wifiStatusToText(finalStatus);
  Serial.println();
  Serial.println("WiFi connect attempt failed.");
  Serial.print("Reason: ");
  Serial.println(wifiLastError);
  return false;
}

void startNetwork() {
  loadStoredWifiCredentials();
  WiFi.setAutoReconnect(true);

  if (strlen(configuredWifiSsid) > 0 && connectStation(configuredWifiSsid, configuredWifiPassword)) {
    return;
  }

  startAccessPoint();
}

void maintainNetwork() {
  if (!usingStationMode) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiLastError = "NONE";
    return;
  }

  unsigned long now = millis();
  if (now - lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
    lastReconnectAttempt = now;
    Serial.println("WiFi disconnected. Attempting reconnect...");
    wifiLastError = wifiStatusToText(WiFi.status());
    WiFi.reconnect();
  }
}

void sendState() {
  StaticJsonDocument<1536> doc;
  doc["mode"] = mode;
  uint8_t effectiveBrightness = BRIGHTNESS;
  if (Current_Pattern == LIGHTNING_W_RAIN_ALT) {
    effectiveBrightness = min(BRIGHTNESS, ALT_PATTERN_BRIGHTNESS_MAX);
  }
  doc["brightness"] = effectiveBrightness;
  doc["lastBrightness"] = LastBrightness;
  doc["currentPattern"] = Current_Pattern;
  doc["networkMode"] = networkMode;
  doc["wifiLastError"] = wifiLastError;
  doc["ip"] = activeIpAddress();
  doc["configuredSsid"] = configuredWifiSsid;
  doc["hasSavedWifi"] = strlen(configuredWifiSsid) > 0;
  doc["solidHex"] = colorToHex(solidColor);

  if (Current_Pattern == LIGHTNING_W_RAIN_ALT) {
    doc["rainDropChance"] = rainAltSpawnChance;
    doc["rainFadeAmount"] = rainAltFadeAmount;
    doc["rainUpdateDelay"] = rainAltUpdateDelay;
  } else {
    doc["rainDropChance"] = rainDropChance;
    doc["rainFadeAmount"] = rainFadeAmount;
    doc["rainUpdateDelay"] = rainUpdateDelay;
  }

  doc["flameCool"] = flameCool;
  doc["flameSparking"] = flameSparking;
  doc["flameUpdateDelay"] = flameUpdateDelay;
  doc["flameClumpMinSize"] = flameClumpMinSize;
  doc["flameClumpMaxSize"] = flameClumpMaxSize;
  doc["flamePaletteMode"] = flamePaletteMode;
  doc["flameBgEmberLevel"] = flameBgEmberLevel;

  // Lightning Alt parameters
  doc["lightningMinInterval"] = lightningAltMinInterval;
  doc["lightningMaxInterval"] = lightningAltMaxInterval;
  doc["lightningFlashBrightness"] = lightningAltFlashBrightness;
  doc["lightningSpeed"] = lightningAltSpeedPercent;
  doc["lightningMinDuration"] = lightningAltMinFlashDuration;
  doc["lightningMaxDuration"] = lightningAltMaxFlashDuration;
  doc["lightningStrikePin"] = lightningAltStrikePinEnabled;
  doc["lightningStrikePinChance"] = lightningAltStrikePinChance;
  doc["lightningStrikePinSpeed"] = lightningAltPinSpeedMultiplier;
  doc["lightningTestMode"] = lightningAltTestMode;
  doc["lightningTestDelay"] = lightningAltTestInterval;
  doc["rainSpawnChance"] = rainAltSpawnChance;
  doc["rainFadeSpeed"] = rainAltFadeAmount;
  doc["rainUpdateSpeed"] = rainAltUpdateDelay;
  doc["rainDropBrightness"] = rainAltBrightness;
  doc["rainTrailChance"] = rainAltTrailChance;
  doc["rainPeakDuration"] = rainAltPeakDuration;
  doc["rainColorMode"] = rainAltColorMode;
  doc["kaleidoscopeSpeed"] = kaleidoscopeSpeedPercent;
  doc["particleMayhemSpeed"] = rainbowWavesSpeedPercent;
  doc["particleMayhemSpeedBoost"] = particleMayhemSpeedBoost;
  doc["particleMayhemRandomSpeed"] = particleMayhemRandomSpeed;
  doc["particleMayhemParticleCount"] = particleMayhemParticleCount;
  doc["particleMayhemDensity"] = particleMayhemDensity;
  doc["particleMayhemExplosionSize"] = particleMayhemExplosionSize;
  doc["particleMayhemExplosionSpeed"] = particleMayhemExplosionSpeed;
  doc["neuroTripSpeed"] = neuroTripSpeedPercent;
  doc["neuroTripDepth"] = neuroTripDepth;
  doc["neuroTripPulse"] = neuroTripPulse;
  doc["wall1Count"] = wall1Count;
  doc["wall2Count"] = wall2Count;
  doc["wall3Count"] = wall3Count;
  doc["wall4Count"] = wall4Count;

  String payload;
  serializeJson(doc, payload);
  server.send(200, "application/json", payload);
}

void configureWebServer() {
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.send(200, "text/html", WEB_UI);
  });

  server.on("/api/state", HTTP_GET, []() {
    sendState();
  });

  server.on("/api/power", HTTP_POST, []() {
    togglePowerMode();
    sendState();
  });

  server.on("/api/brightness", HTTP_POST, []() {
    int value = getRequestParamInt("value", BRIGHTNESS);
    int maxBrightness = (Current_Pattern == LIGHTNING_W_RAIN_ALT) ? ALT_PATTERN_BRIGHTNESS_MAX : 255;
    value = constrain(value, 0, maxBrightness);
    BRIGHTNESS = (uint8_t)value;
    if (BRIGHTNESS > 0 && mode == IDLE) {
      mode = RUNNING;
    }
    sendState();
  });

  server.on("/api/pattern", HTTP_POST, []() {
    int value = getRequestParamInt("value", Current_Pattern);
    value = constrain(value, SOLID, NEURO_TRIP);
    setPatternValue((uint8_t)value);
    sendState();
  });

  server.on("/api/wall-count", HTTP_POST, []() {
    setWallCountValues(
      getRequestParamInt("wall1", wall1Count),
      getRequestParamInt("wall2", wall2Count),
      getRequestParamInt("wall3", wall3Count),
      getRequestParamInt("wall4", wall4Count)
    );
    sendState();
  });

  server.on("/api/solid", HTTP_POST, []() {
    String hex = server.hasArg("hex") ? server.arg("hex") : "";
    solidColor = parseHexColor(hex);
    setPatternValue(SOLID);
    sendState();
  });

  server.on("/api/rain", HTTP_POST, []() {
    if (Current_Pattern == LIGHTNING_W_RAIN_ALT) {
      rainAltSpawnChance = (uint8_t)constrain(getRequestParamInt("chance", rainAltSpawnChance), 0, 255);
      rainAltFadeAmount = (uint8_t)constrain(getRequestParamInt("fade", rainAltFadeAmount), 0, 120);
      rainAltUpdateDelay = (uint16_t)constrain(getRequestParamInt("delay", rainAltUpdateDelay), 1, 250);
    } else {
      rainDropChance = (uint8_t)constrain(getRequestParamInt("chance", rainDropChance), 0, 29);
      rainFadeAmount = (uint8_t)constrain(getRequestParamInt("fade", rainFadeAmount), 0, 99);
      rainUpdateDelay = (uint16_t)constrain(getRequestParamInt("delay", rainUpdateDelay), 25, 250);
    }
    sendState();
  });

  server.on("/api/rain/reset", HTTP_POST, []() {
    if (Current_Pattern == LIGHTNING_W_RAIN_ALT) {
      rainAltSpawnChance = 2;
      rainAltFadeAmount = 8;
      rainAltUpdateDelay = 60;
    } else {
      rainDropChance = 1;
      rainFadeAmount = 10;
      rainUpdateDelay = 50;
    }
    sendState();
  });

  server.on("/api/flame", HTTP_POST, []() {
    flameCool = (uint8_t)constrain(getRequestParamInt("cool", flameCool), 1, 50);
    flameSparking = (uint8_t)constrain(getRequestParamInt("sparking", flameSparking), 0, 255);
    flameUpdateDelay = (uint16_t)constrain(getRequestParamInt("delay", flameUpdateDelay), 5, 200);
    flameClumpMinSize = (uint8_t)constrain(getRequestParamInt("clumpMin", flameClumpMinSize), 1, 30);
    flameClumpMaxSize = (uint8_t)constrain(getRequestParamInt("clumpMax", flameClumpMaxSize), flameClumpMinSize, 16);
    flamePaletteMode = (uint8_t)constrain(getRequestParamInt("palette", flamePaletteMode), 0, 4);
    flameBgEmberLevel = (uint8_t)constrain(getRequestParamInt("bg", flameBgEmberLevel), 0, 30);
    sendState();
  });

  server.on("/api/flame/reset", HTTP_POST, []() {
    flameCool = 2;
    flameSparking = 200;
    flameUpdateDelay = 25;
    flameClumpMinSize = 3;
    flameClumpMaxSize = 16;
    flamePaletteMode = 0;
    flameBgEmberLevel = 7;
    sendState();
  });

  server.on("/api/lightning-alt", HTTP_POST, []() {
    lightningAltMinInterval = (uint32_t)constrain(getRequestParamInt("minInterval", (int)lightningAltMinInterval), 5000, 300000);
    lightningAltMaxInterval = (uint32_t)constrain(getRequestParamInt("maxInterval", (int)lightningAltMaxInterval), 5000, 300000);
    if (lightningAltMinInterval > lightningAltMaxInterval) {
      uint32_t t = lightningAltMinInterval;
      lightningAltMinInterval = lightningAltMaxInterval;
      lightningAltMaxInterval = t;
    }
    lightningAltFlashBrightness = (uint8_t)constrain(getRequestParamInt("flashBrightness", lightningAltFlashBrightness), 0, 255);
    lightningAltSpeedPercent = (uint8_t)constrain(getRequestParamInt("speed", lightningAltSpeedPercent), 50, 220);
    lightningAltMinFlashDuration = (uint16_t)constrain(getRequestParamInt("minDuration", lightningAltMinFlashDuration), 10, 2000);
    lightningAltMaxFlashDuration = (uint16_t)constrain(getRequestParamInt("maxDuration", lightningAltMaxFlashDuration), 10, 2000);
    if (lightningAltMinFlashDuration > lightningAltMaxFlashDuration) {
      uint16_t t = lightningAltMinFlashDuration;
      lightningAltMinFlashDuration = lightningAltMaxFlashDuration;
      lightningAltMaxFlashDuration = t;
    }
    lightningAltStrikePinEnabled = getRequestParamInt("strikePin", lightningAltStrikePinEnabled ? 1 : 0) != 0;
    lightningAltStrikePinChance = (uint8_t)constrain(getRequestParamInt("strikePinChance", lightningAltStrikePinChance), 0, 100);
    lightningAltPinSpeedMultiplier = (uint8_t)constrain(getRequestParamInt("strikePinSpeed", lightningAltPinSpeedMultiplier), 4, 50);
    lightningAltTestMode = getRequestParamInt("testMode", lightningAltTestMode ? 1 : 0) != 0;
    lightningAltTestInterval = (uint16_t)constrain(getRequestParamInt("testDelay", lightningAltTestInterval), 50, 5000);
    rainAltSpawnChance = (uint8_t)constrain(getRequestParamInt("spawnChance", rainAltSpawnChance), 0, 255);
    rainAltFadeAmount = (uint8_t)constrain(getRequestParamInt("fadeSpeed", rainAltFadeAmount), 0, 120);
    rainAltUpdateDelay = (uint16_t)constrain(getRequestParamInt("updateSpeed", rainAltUpdateDelay), 1, 200);
    rainAltBrightness = (uint8_t)constrain(getRequestParamInt("brightness", rainAltBrightness), 0, 255);
    rainAltTrailChance = (uint8_t)constrain(getRequestParamInt("trailChance", rainAltTrailChance), 0, 100);
    rainAltPeakDuration = (uint16_t)constrain(getRequestParamInt("peakDuration", rainAltPeakDuration), 50, 1000);
    
    // Trigger immediate flash so user can see the change in real-time
    forceImmediateLightningAltStrike();
    
    sendState();
  });

  server.on("/api/lightning-alt/reset", HTTP_POST, []() {
    lightningAltMinInterval = 5000;
    lightningAltMaxInterval = 30000;
    lightningAltFlashBrightness = 185;
    lightningAltSpeedPercent = 100;
    lightningAltMinFlashDuration = 25;
    lightningAltMaxFlashDuration = 140;
    lightningAltStrikePinEnabled = false;
    lightningAltStrikePinChance = 60;
    lightningAltPinSpeedMultiplier = 4;
    lightningAltTestMode = false;
    lightningAltTestInterval = 1000;
    rainAltSpawnChance = 12;
    rainAltFadeAmount = 14;
    rainAltUpdateDelay = 14;
    rainAltBrightness = 200;
    rainAltTrailChance = 30;
    rainAltPeakDuration = 110;
    // Trigger immediate flash so user can see the reset take effect
    forceImmediateLightningAltStrike();
    
    sendState();
  });

  server.on("/api/rain-color-mode", HTTP_POST, []() {
    uint8_t mode = (uint8_t)constrain(getRequestParamInt("mode", rainAltColorMode), 0, 2);
    rainAltColorMode = mode;
    sendState();
  });

  server.on("/api/kaleidoscope", HTTP_POST, []() {
    kaleidoscopeSpeedPercent = (uint8_t)constrain(getRequestParamInt("speed", kaleidoscopeSpeedPercent), 50, 220);
    sendState();
  });

  server.on("/api/kaleidoscope/reset", HTTP_POST, []() {
    kaleidoscopeSpeedPercent = 100;
    initKaleidoscope();
    sendState();
  });

  server.on("/api/particle-mayhem", HTTP_POST, []() {
    rainbowWavesSpeedPercent = (uint8_t)constrain(getRequestParamInt("speed", rainbowWavesSpeedPercent), 50, 220);
    particleMayhemSpeedBoost = (uint8_t)constrain(getRequestParamInt("speedBoost", particleMayhemSpeedBoost), 0, 100);
    particleMayhemRandomSpeed = (uint8_t)constrain(getRequestParamInt("randomSpeed", particleMayhemRandomSpeed), 0, 100);
    particleMayhemParticleCount = (uint8_t)constrain(getRequestParamInt("particleCount", particleMayhemParticleCount), 1, 10);
    particleMayhemDensity = (uint8_t)constrain(getRequestParamInt("density", particleMayhemDensity), 1, 5);
    particleMayhemExplosionSize = (uint8_t)constrain(getRequestParamInt("explosion", particleMayhemExplosionSize), 2, 10);
    particleMayhemExplosionSpeed = (uint8_t)constrain(getRequestParamInt("explosionSpeed", particleMayhemExplosionSpeed), 1, 10);
    sendState();
  });

  server.on("/api/particle-mayhem/reset", HTTP_POST, []() {
    rainbowWavesSpeedPercent = 100;
    particleMayhemSpeedBoost = 35;
    particleMayhemRandomSpeed = 55;
    particleMayhemParticleCount = 5;
    particleMayhemDensity = 4;
    particleMayhemExplosionSize = 7;
    particleMayhemExplosionSpeed = 6;
    initRainbowWaves();
    sendState();
  });

  server.on("/api/rainbow-waves", HTTP_POST, []() {
    rainbowWavesSpeedPercent = (uint8_t)constrain(getRequestParamInt("speed", rainbowWavesSpeedPercent), 50, 220);
    particleMayhemSpeedBoost = (uint8_t)constrain(getRequestParamInt("speedBoost", particleMayhemSpeedBoost), 0, 100);
    particleMayhemRandomSpeed = (uint8_t)constrain(getRequestParamInt("randomSpeed", particleMayhemRandomSpeed), 0, 100);
    particleMayhemParticleCount = (uint8_t)constrain(getRequestParamInt("particleCount", particleMayhemParticleCount), 1, 10);
    particleMayhemDensity = (uint8_t)constrain(getRequestParamInt("density", particleMayhemDensity), 1, 5);
    particleMayhemExplosionSize = (uint8_t)constrain(getRequestParamInt("explosion", particleMayhemExplosionSize), 2, 10);
    particleMayhemExplosionSpeed = (uint8_t)constrain(getRequestParamInt("explosionSpeed", particleMayhemExplosionSpeed), 1, 10);
    sendState();
  });

  server.on("/api/rainbow-waves/reset", HTTP_POST, []() {
    rainbowWavesSpeedPercent = 100;
    particleMayhemSpeedBoost = 35;
    particleMayhemRandomSpeed = 55;
    particleMayhemParticleCount = 5;
    particleMayhemDensity = 4;
    particleMayhemExplosionSize = 7;
    particleMayhemExplosionSpeed = 6;
    initRainbowWaves();
    sendState();
  });

  server.on("/api/neuro-trip", HTTP_POST, []() {
    neuroTripSpeedPercent = (uint8_t)constrain(getRequestParamInt("speed", neuroTripSpeedPercent), 30, 240);
    neuroTripDepth = (uint8_t)constrain(getRequestParamInt("depth", neuroTripDepth), 40, 255);
    neuroTripPulse = (uint8_t)constrain(getRequestParamInt("pulse", neuroTripPulse), 0, 255);
    sendState();
  });

  server.on("/api/neuro-trip/reset", HTTP_POST, []() {
    neuroTripSpeedPercent = 105;
    neuroTripDepth = 180;
    neuroTripPulse = 150;
    initNeuroTrip();
    sendState();
  });

  server.on("/api/wifi/set", HTTP_POST, []() {
    String ssid = server.hasArg("ssid") ? server.arg("ssid") : "";
    String password = server.hasArg("password") ? server.arg("password") : "";

    ssid.trim();
    if (ssid.length() == 0) {
      server.send(400, "application/json", "{\"error\":\"ssid_required\"}");
      return;
    }

    persistWifiCredentials(ssid, password);
    if (!connectStation(configuredWifiSsid, configuredWifiPassword, 12000)) {
      startAccessPoint();
    }
    sendState();
  });

  server.on("/api/wifi/clear", HTTP_POST, []() {
    clearWifiCredentials();
    startAccessPoint();
    sendState();
  });

  // ============================================================
  // PRESET MANAGEMENT ENDPOINTS
  // ============================================================
  
  // List all presets for a pattern
  server.on("/api/presets/list", HTTP_GET, []() {
    uint8_t pattern = (uint8_t)getRequestParamInt("pattern", LIGHTNING_W_RAIN_ALT);
    StaticJsonDocument<1024> doc;
    JsonArray presets = doc.createNestedArray("presets");
    
    for (int i = 0; i < MAX_PRESETS; i++) {
      String name = getPresetName(i, pattern);
      JsonObject preset = presets.createNestedObject();
      preset["slot"] = i;
      preset["name"] = name.length() > 0 ? name : "";
      preset["exists"] = name.length() > 0;
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });

  // Save current effect as preset
  server.on("/api/presets/save", HTTP_POST, []() {
    uint8_t slot = (uint8_t)constrain(getRequestParamInt("slot", 0), 0, MAX_PRESETS - 1);
    uint8_t pattern = (uint8_t)getRequestParamInt("pattern", Current_Pattern);
    String name = server.arg("name");
    if (name.length() == 0) name = "Preset";
    
    savePreset(slot, name, pattern);
    server.send(200, "application/json", "{\"status\":\"saved\"}");
  });

  // Load preset
  server.on("/api/presets/load", HTTP_POST, []() {
    uint8_t slot = (uint8_t)constrain(getRequestParamInt("slot", 0), 0, MAX_PRESETS - 1);
    uint8_t pattern = (uint8_t)getRequestParamInt("pattern", LIGHTNING_W_RAIN_ALT);
    
    if (loadPreset(slot, pattern)) {
      Current_Pattern = pattern;
      server.send(200, "application/json", "{\"status\":\"loaded\"}");
    } else {
      server.send(400, "application/json", "{\"error\":\"preset_not_found\"}");
    }
  });

  // Delete preset
  server.on("/api/presets/delete", HTTP_POST, []() {
    uint8_t slot = (uint8_t)constrain(getRequestParamInt("slot", 0), 0, MAX_PRESETS - 1);
    uint8_t pattern = (uint8_t)getRequestParamInt("pattern", LIGHTNING_W_RAIN_ALT);
    
    deletePreset(slot, pattern);
    server.send(200, "application/json", "{\"status\":\"deleted\"}");
  });

  server.onNotFound([]() {
    server.send(404, "application/json", "{\"error\":\"not_found\"}");
  });

  server.begin();
  Serial.println("Web server online on port 80");
}


// ------------------------------
// Handle IR Remote button presses
// ------------------------------
void handleIRCode(uint32_t code) {
  switch (code) {
    case 0xF00FFF00: Serial.println("Button: AUTO"); break;

    case 0xA25DFF00: Serial.println("Button: Brightness Down"); if (BRIGHTNESS > 1) BRIGHTNESS--; break;
    case 0xA35CFF00: Serial.println("Button: Brightness UP"); if (BRIGHTNESS < 255) BRIGHTNESS++; break;

    case 0xBA45FF00: Serial.println("Button: BLUE"); setPatternValue(SOLID); solidColor = CRGB::Blue; break;
    case 0xB649FF00: Serial.println("Button: B1"); setPatternValue(SOLID); solidColor = CRGB::Cyan; break;
    case 0xB24DFF00: Serial.println("Button: B2"); setPatternValue(SOLID); solidColor = CRGB(75, 0, 130); break;
    case 0xE11EFF00: Serial.println("Button: B3"); setPatternValue(SOLID); solidColor = CRGB(221, 160, 221); break;
    case 0xE51AFF00: Serial.println("Button: B4"); setPatternValue(SOLID); solidColor = CRGB(255, 102, 204); break;

    case 0xA659FF00: Serial.println("Button: GREEN"); setPatternValue(SOLID); solidColor = CRGB::Green; break;
    case 0xAA55FF00: Serial.println("Button: G1"); setPatternValue(SOLID); solidColor = CRGB(50, 205, 50); break;
    case 0xAE51FF00: Serial.println("Button: G2"); setPatternValue(SOLID); solidColor = CRGB(0, 255, 255); break;
    case 0xE21DFF00: Serial.println("Button: G3"); setPatternValue(SOLID); solidColor = CRGB(0, 128, 128); break;
    case 0xE619FF00: Serial.println("Button: G4"); setPatternValue(SOLID); solidColor = CRGB(255, 0, 255); break;

    case 0xA758FF00: Serial.println("Button: RED"); setPatternValue(SOLID); solidColor = CRGB::Red; break;
    case 0xAB54FF00: Serial.println("Button: R1"); setPatternValue(SOLID); solidColor = CRGB::Orange; break;
    case 0xAF50FF00: Serial.println("Button: R2"); setPatternValue(SOLID); solidColor = CRGB(255, 191, 0); break;
    case 0xE31CFF00: Serial.println("Button: R3"); setPatternValue(SOLID); solidColor = CRGB(218, 165, 32); break;
    case 0xE718FF00: Serial.println("Button: R4"); setPatternValue(SOLID); solidColor = CRGB::Yellow; break;

    case 0xBB44FF00: Serial.println("Button: WHITE"); setPatternValue(SOLID); solidColor = CRGB::White; break;
    case 0xB748FF00: Serial.println("Button: W1"); setPatternValue(SOLID); solidColor = CRGB(255, 182, 193); break;
    case 0xB34CFF00: Serial.println("Button: W2"); setPatternValue(SOLID); solidColor = CRGB(255, 209, 223); break;
    case 0xE01FFF00: Serial.println("Button: W3"); setPatternValue(SOLID); solidColor = CRGB(173, 216, 230); break;
    case 0xE41BFF00: Serial.println("Button: W4"); setPatternValue(SOLID); solidColor = CRGB(135, 206, 235); break;

    case 0xF30CFF00: Serial.println("Button: DIY1"); Current_Pattern = LIGHTNING_W_RAIN; clearLeds(); break;
    case 0xF20DFF00: Serial.println("Button: DIY2"); Current_Pattern = PONG_CHASER; break;
    case 0xF10EFF00: Serial.println("Button: DIY3"); Current_Pattern = FLAME; break;
    case 0xF708FF00: Serial.println("Button: DIY4"); Current_Pattern = PAC_MAN; break;
    case 0xF609FF00: Serial.println("Button: DIY5"); Current_Pattern = LIGHTNING_W_RAIN_ALT; clearLeds(); break;
    case 0xF50AFF00: Serial.println("Button: DIY6"); break;

    case 0xEC13FF00: Serial.println("Button: SLOW"); break;
    case 0xE817FF00: Serial.println("Button: Quick"); break;

    case 0xF40BFF00: Serial.println("Button: FLASH"); break;
    case 0xF807FF00: Serial.println("Button: FADE 7"); break;
    case 0xF906FF00: Serial.println("Button: FADE 3"); break;

    case 0xFA05FF00: Serial.println("Button: JUMP 7"); break;
    case 0xFB04FF00: Serial.println("Button: JUMP 3"); break;

    case 0xEB14FF00:
      Serial.println("Button: RED UP");
      if (Current_Pattern == LIGHTNING_W_RAIN) { rainDropChance = (rainDropChance + 1) % 30; }
      if (Current_Pattern == LIGHTNING_W_RAIN_ALT) { rainAltSpawnChance = min(255, rainAltSpawnChance + 5); }
      if (Current_Pattern == FLAME) { flameCool += 1; }
      Serial.print("flameCool "); Serial.println(flameCool);
      break;

    case 0xEF10FF00:
      Serial.println("Button: RED DOWN");
      if (Current_Pattern == LIGHTNING_W_RAIN) { rainDropChance = (rainDropChance - 1) % 30; }
      if (Current_Pattern == LIGHTNING_W_RAIN_ALT) { rainAltSpawnChance = (rainAltSpawnChance >= 5) ? rainAltSpawnChance - 5 : 0; }
      if (Current_Pattern == FLAME) { flameCool -= 1; }
      break;

    case 0xEA15FF00:
      Serial.println("Button: GREEN UP");
      if (Current_Pattern == LIGHTNING_W_RAIN) { rainFadeAmount = (rainFadeAmount + 10) % 100; }
      if (Current_Pattern == LIGHTNING_W_RAIN_ALT) { rainAltFadeAmount = min(120, rainAltFadeAmount + 4); }
      if (Current_Pattern == FLAME) { flameSparking += 10; }
      break;

    case 0xEE11FF00:
      Serial.println("Button: GREEN DOWN");
      if (Current_Pattern == LIGHTNING_W_RAIN) { rainFadeAmount = (rainFadeAmount - 10) % 100; }
      if (Current_Pattern == LIGHTNING_W_RAIN_ALT) { rainAltFadeAmount = max(0, (int)rainAltFadeAmount - 2); }
      if (Current_Pattern == FLAME) { flameSparking -= 10; }
      break;

    case 0xE916FF00:
      Serial.println("Button: BLUE UP");
      if ((Current_Pattern == LIGHTNING_W_RAIN) && (rainUpdateDelay < 250)) { rainUpdateDelay += 25; }
      if ((Current_Pattern == LIGHTNING_W_RAIN_ALT) && (rainAltUpdateDelay < 250)) { rainAltUpdateDelay += 5; }
      if (Current_Pattern == FLAME) { flameUpdateDelay += 5; }
      break;

    case 0xED12FF00:
      Serial.println("Button: BLUE DOWN");
      if ((Current_Pattern == LIGHTNING_W_RAIN) && (rainUpdateDelay > 25)) { rainUpdateDelay -= 25; }
      if (Current_Pattern == LIGHTNING_W_RAIN_ALT) { rainAltUpdateDelay = (rainAltUpdateDelay > 5) ? rainAltUpdateDelay - 5 : 1; }
      if (Current_Pattern == FLAME) { flameUpdateDelay -= 5; }
      break;

    case 0xBE41FF00:
      Serial.println("Button: NEXT");
      if (Current_Pattern == LIGHTNING_W_RAIN) { rainDropChance = 1; rainFadeAmount = 10; rainUpdateDelay = 50; }
      if (Current_Pattern == LIGHTNING_W_RAIN_ALT) { rainAltSpawnChance = 2; rainAltFadeAmount = 8; rainAltUpdateDelay = 60; }
      if (Current_Pattern == FLAME) { flameCool = 2; flameSparking = 200; flameUpdateDelay = 25; flamePaletteMode = 0; flameBgEmberLevel = 7; }
      break;

    case 0xBF40FF00:
      Serial.println("Button: POWER");
      togglePowerMode();
      Serial.print("Mode: ");
      Serial.println(mode == RUNNING ? "RUNNING" : "IDLE");
      break;

    default:
      if (code != 0x00000000) {  // Ignore "repeat" code
        Serial.print("Unknown IR code: 0x");
        Serial.println(code, HEX);
      }
      break;
  }

  if ((mode == IDLE) && (BRIGHTNESS > 0)) {
    mode = RUNNING;
  }
}



void applyPattern(unsigned long currentTime) {
  switch (Current_Pattern) {
    case SOLID:
      fill_solid(leds, NUM_LEDS, solidColor);
      break;
    case LIGHTNING_W_RAIN:
      debugBypass = false;
      if (strike.phase == STRIKE_IDLE && currentTime - lastStrikeTime > nextStrikeInterval) {
        beginLightningStrike();
      }
      if (strike.phase != STRIKE_IDLE) {
        updateLightningStrike();
      }
      updateRainEffect(currentTime);
      break;
    case LIGHTNING_W_RAIN_ALT:
      updateLightningAltStrike(currentTime);
      updateRainEffectAlt(currentTime);
      break;
    case PONG_CHASER:
      updatePongChasers(leds, NUM_LEDS);
      break;
    case FLAME:
       updateFlameEffect(currentTime);
       break;
    case PAC_MAN:
      updatePacmanEffect();
      break;
    case KALEIDOSCOPE:
      updateKaleidoscope(currentTime);
      break;
    case RAINBOW_WAVES:
      updateRainbowWaves(currentTime);
      break;
    case WALL_COUNT_TEST: {
      updateWallCountTest(leds);
      break;
    }
    case NEURO_TRIP:
      updateNeuroTrip(currentTime);
      break;
    default:
        break;
  }
}

void setup() {
  // delay(1000);
  Serial.begin(115200);
  delay(1000);
  Serial.println("---------LED CONTROLLER ONLINE---------");
  mode = RUNNING;
  remoteHandler.begin();
  remoteHandler.setPressCallback(handleIRCode);
  remoteHandler.setHoldCallback(handleIRCode);
  initPongChasers(NUM_LEDS);
  setupPacmanEffect();
  initKaleidoscope();
  initRainbowWaves();
  initNeuroTrip();
  // Initialize rain alt drops
  for (int i = 0; i < MAX_RAIN_DROPS_ALT; i++) {
    rainAltDrops[i].position = -1;
  }
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  startNetwork();
  configureWebServer();
}

void loop() {
  remoteHandler.update();
  server.handleClient();
  maintainNetwork();
  unsigned long currentTime = millis();

  // Alt pattern has a hard power-safety cap while still allowing full dimming down to 0.
  uint8_t outputBrightness = BRIGHTNESS;
  if (Current_Pattern == LIGHTNING_W_RAIN_ALT && BRIGHTNESS > 0) {
    outputBrightness = min(BRIGHTNESS, ALT_PATTERN_BRIGHTNESS_MAX);
  }
  FastLED.setBrightness(outputBrightness);

  applyPattern(currentTime);
  FastLED.show();
}