#include "effects_engine.h"

#include <Arduino.h>
#include <FastLED.h>
#include <NeoPixelBus.h>

#include "../Effects/EffectRegistry.h"
#include "../Effects/EffectPresetStore.h"

#ifndef LED_DATA_PIN
#define LED_DATA_PIN 18
#endif

#ifndef LED_STRIP_LENGTH
#define LED_STRIP_LENGTH 1600
#endif

#ifndef LED_TRANSPORT_MAX
#define LED_TRANSPORT_MAX LED_STRIP_LENGTH
#endif

#ifndef LED_COLOR_ORDER
#define LED_COLOR_ORDER GRB
#endif

#ifndef LED_TYPE
#define LED_TYPE WS2812B
#endif

namespace {

const uint16_t kTransportLedCount =
  (LED_TRANSPORT_MAX > LED_STRIP_LENGTH) ? LED_STRIP_LENGTH : LED_TRANSPORT_MAX;
const uint16_t kMinFps = 15;
const uint16_t kMaxFps = 600;
const uint8_t kDirectRgbPatternId = 0;
const uint8_t kSolidPatternId = 1;
const char *kDirectRgbPatternName = "Direct RGB";

CRGB g_leds[LED_STRIP_LENGTH];
NeoPixelBus<NeoGrbFeature, NeoEsp32BitBangWs2812Method> g_strip(kTransportLedCount, LED_DATA_PIN);
portMUX_TYPE g_effectMux = portMUX_INITIALIZER_UNLOCKED;

LedEffectState g_state = {
    true,
    kSolidPatternId,
    96,
    128,
    20,
    true,
    255,
    160,
    48,
};

uint32_t g_lastFrameMs = 0;
uint32_t g_frame = 0;
uint16_t g_phase = 0;
uint16_t g_activeLedCount = LED_STRIP_LENGTH;
bool g_effectsInitialized = false;
bool g_lastFrameWasOff = false;
bool g_lastOutputWasBlack = false;
uint8_t g_lastActivePattern = 0xFF;
EffectDescriptor g_directRgbFallback = {kDirectRgbPatternId, kDirectRgbPatternName, nullptr, nullptr};

uint16_t effectiveLedCount(uint16_t requestedCount) {
  return requestedCount > kTransportLedCount ? kTransportLedCount : requestedCount;
}

uint8_t scaleChannel(uint8_t value, uint8_t brightness) {
  if (brightness >= 255) {
    return value;
  }

  return static_cast<uint8_t>((static_cast<uint16_t>(value) * static_cast<uint16_t>(brightness)) / 255U);
}

bool isAllBlack(uint16_t activeLedCount) {
  const uint16_t clampedCount = effectiveLedCount(activeLedCount);
  for (uint16_t i = 0; i < clampedCount; ++i) {
    if (g_leds[i].r != 0 || g_leds[i].g != 0 || g_leds[i].b != 0) {
      return false;
    }
  }
  return true;
}

void flushStrip(uint16_t activeLedCount, uint8_t brightness) {
  const uint16_t clampedCount = effectiveLedCount(activeLedCount);
  const uint16_t transportCount = kTransportLedCount;

  for (uint16_t i = 0; i < clampedCount; ++i) {
    const CRGB &pixel = g_leds[i];
    g_strip.SetPixelColor(i,
                          RgbColor(scaleChannel(pixel.r, brightness),
                                   scaleChannel(pixel.g, brightness),
                                   scaleChannel(pixel.b, brightness)));
  }

  for (uint16_t i = clampedCount; i < transportCount; ++i) {
    g_strip.SetPixelColor(i, RgbColor(0, 0, 0));
  }

  g_strip.Show();
}

LedEffectState snapshotState() {
  portENTER_CRITICAL(&g_effectMux);
  const LedEffectState s = g_state;
  portEXIT_CRITICAL(&g_effectMux);
  return s;
}

void writeState(const LedEffectState &next) {
  portENTER_CRITICAL(&g_effectMux);
  g_state = next;
  portEXIT_CRITICAL(&g_effectMux);
}

uint16_t clampFrameDelayMs(uint16_t fps) {
  if (fps < kMinFps) {
    fps = kMinFps;
  }
  if (fps > kMaxFps) {
    fps = kMaxFps;
  }

  const uint16_t delayMs = static_cast<uint16_t>(1000U / fps);
  return delayMs == 0 ? 1 : delayMs;
}

uint16_t transportMinFrameDelayMs(uint16_t ledCount) {
  const uint32_t clampedCount = ledCount == 0 ? 1 : ledCount;
  // WS2812-class LEDs need about 30 us per pixel plus reset/latch time.
  const uint32_t frameTimeUs = (clampedCount * 30U) + 300U;
  const uint16_t delayMs = static_cast<uint16_t>((frameTimeUs + 999U) / 1000U);
  return delayMs == 0 ? 1 : static_cast<uint16_t>(delayMs + 1U);
}

uint16_t maxFpsForLedCount(uint16_t ledCount) {
  const uint16_t clampedCount = effectiveLedCount(ledCount);
  const uint16_t minFrameDelayMs = transportMinFrameDelayMs(clampedCount);
  uint16_t maxByTransport = static_cast<uint16_t>(1000U / minFrameDelayMs);
  if (maxByTransport < kMinFps) {
    maxByTransport = kMinFps;
  }
  return maxByTransport > kMaxFps ? kMaxFps : maxByTransport;
}

void initializeEffectsIfNeeded() {
  if (g_effectsInitialized) {
    return;
  }

  EffectDescriptor *it = effectsRegistryHead();
  while (it != nullptr) {
    it->effect->begin(g_leds, LED_STRIP_LENGTH);
    it = it->next;
  }

  g_effectsInitialized = true;
}

EffectDescriptor *activeEffectOrFallback(uint8_t requestedId) {
  if (effectsRegistryHead() == nullptr) {
    return &g_directRgbFallback;
  }

  EffectDescriptor *found = effectsFindById(requestedId);
  if (found != nullptr) {
    return found;
  }

  EffectDescriptor *fallback = effectsFindById(0);
  if (fallback != nullptr) {
    return fallback;
  }

  return effectsRegistryHead();
}

}  // namespace

void effectsEngineBegin() {
  fill_solid(g_leds, LED_STRIP_LENGTH, CRGB::Black);
  g_strip.Begin();
  g_strip.ClearTo(RgbColor(0, 0, 0));
  g_strip.Show();
  initializeEffectsIfNeeded();
}

void effectsEngineTick() {
  const uint32_t now = millis();
  const LedEffectState s = snapshotState();
  const uint16_t activeLedCount = effectiveLedCount(g_activeLedCount);
  const uint16_t transportFrameDelay = transportMinFrameDelayMs(activeLedCount);

  if (!s.powerOn || s.brightness == 0) {
    if (g_lastFrameWasOff) {
      return;
    }

    if (now - g_lastFrameMs < transportFrameDelay) {
      return;
    }

    g_lastFrameMs = now;
    g_lastFrameWasOff = true;
    g_lastOutputWasBlack = true;
    fill_solid(g_leds, LED_STRIP_LENGTH, CRGB::Black);
    flushStrip(activeLedCount, 0);
    return;
  }

  g_lastFrameWasOff = false;

  const uint16_t requestedFrameDelay = clampFrameDelayMs(s.fps);
  const uint16_t frameDelay = requestedFrameDelay > transportFrameDelay ? requestedFrameDelay : transportFrameDelay;
  if (now - g_lastFrameMs < frameDelay) {
    return;
  }

  const uint32_t previousFrameMs = g_lastFrameMs;
  g_lastFrameMs = now;
  ++g_frame;
  const uint32_t deltaMs = previousFrameMs == 0 ? 0 : (now - previousFrameMs);
  g_phase = static_cast<uint16_t>(g_phase + map(s.speed, 1, 255, 1, 7));

  EffectDescriptor *active = activeEffectOrFallback(s.pattern);
  if (active != nullptr && active->effect != nullptr && g_lastActivePattern != active->id) {
    if (g_lastActivePattern != 0xFF) {
      EffectDescriptor *previous = activeEffectOrFallback(g_lastActivePattern);
      if (previous != nullptr && previous->effect != nullptr) {
        previous->effect->onDeactivate();
      }
    }
    active->effect->onActivate(s);
    g_lastActivePattern = active->id;
  }

  if (active != nullptr && active->effect != nullptr) {
    EffectContext ctx = {s, now, deltaMs, g_phase, g_frame, activeLedCount};
    active->effect->render(ctx, g_leds, activeLedCount);

    if (activeLedCount < LED_STRIP_LENGTH) {
      fill_solid(g_leds + activeLedCount, LED_STRIP_LENGTH - activeLedCount, CRGB::Black);
    }
  } else {
    fill_solid(g_leds, activeLedCount, CRGB(s.red, s.green, s.blue));
    if (activeLedCount < LED_STRIP_LENGTH) {
      fill_solid(g_leds + activeLedCount, LED_STRIP_LENGTH - activeLedCount, CRGB::Black);
    }
  }

  const bool frameIsBlack = isAllBlack(activeLedCount);
  if (frameIsBlack && g_lastOutputWasBlack) {
    return;
  }

  flushStrip(activeLedCount, s.brightness);
  g_lastOutputWasBlack = frameIsBlack;
}

LedEffectState effectsEngineGetState() {
  return snapshotState();
}

void effectsEngineSetPower(bool on) {
  LedEffectState s = snapshotState();
  s.powerOn = on;
  writeState(s);
}

void effectsEngineSetPattern(uint8_t pattern) {
  LedEffectState s = snapshotState();
  if (effectsRegistryHead() == nullptr) {
    s.pattern = kDirectRgbPatternId;
  } else {
    EffectDescriptor *found = effectsFindById(pattern);
    s.pattern = (found != nullptr) ? pattern : effectsRegistryHead()->id;
  }
  writeState(s);
}

void effectsEngineSetBrightness(uint8_t brightness) {
  LedEffectState s = snapshotState();
  s.brightness = brightness;
  writeState(s);
}

void effectsEngineSetSpeed(uint8_t speed) {
  LedEffectState s = snapshotState();
  s.speed = speed == 0 ? 1 : speed;
  writeState(s);
}

void effectsEngineSetFps(uint16_t fps) {
  LedEffectState s = snapshotState();
  const uint16_t currentMax = maxFpsForLedCount(g_activeLedCount);
  if (fps < kMinFps) {
    fps = kMinFps;
  }
  if (fps > currentMax) {
    fps = currentMax;
  }
  s.fps = fps;
  writeState(s);
}

void effectsEngineSetDither(bool enabled) {
  LedEffectState s = snapshotState();
  s.dither = enabled;
  writeState(s);
}

void effectsEngineSetColor(uint8_t red, uint8_t green, uint8_t blue) {
  LedEffectState s = snapshotState();
  s.red = red;
  s.green = green;
  s.blue = blue;
  writeState(s);
}

void effectsEngineSetActiveLedCount(uint16_t count) {
  g_activeLedCount = static_cast<uint16_t>(constrain(count, static_cast<uint16_t>(1), effectsEngineGetMaxLedCount()));

  LedEffectState s = snapshotState();
  const uint16_t newMaxFps = maxFpsForLedCount(g_activeLedCount);
  if (s.fps > newMaxFps) {
    s.fps = newMaxFps;
    writeState(s);
  }
}

uint16_t effectsEngineGetActiveLedCount() {
  return g_activeLedCount;
}

uint16_t effectsEngineGetMaxLedCount() {
  return effectiveLedCount(LED_STRIP_LENGTH);
}

uint16_t effectsEngineGetMaxFps() {
  return maxFpsForLedCount(g_activeLedCount);
}

uint16_t effectsEngineGetMaxFpsForLedCount(uint16_t ledCount) {
  return maxFpsForLedCount(ledCount);
}

String effectsEngineActiveSettingsSchemaJson() {
  const LedEffectState s = snapshotState();
  EffectDescriptor *active = activeEffectOrFallback(s.pattern);
  if (active == nullptr || active->effect == nullptr) {
    return "[]";
  }
  return active->effect->settingsSchemaJson();
}

String effectsEngineActiveSettingsStateJson() {
  const LedEffectState s = snapshotState();
  EffectDescriptor *active = activeEffectOrFallback(s.pattern);
  if (active == nullptr || active->effect == nullptr) {
    return "{}";
  }
  return active->effect->settingsStateJson();
}

bool effectsEngineSetActiveSetting(const String &key, const String &value) {
  const LedEffectState s = snapshotState();
  EffectDescriptor *active = activeEffectOrFallback(s.pattern);
  if (active == nullptr || active->effect == nullptr) {
    return false;
  }
  return active->effect->setSetting(key, value);
}

void effectsEngineResetActiveSettings() {
  const LedEffectState s = snapshotState();
  EffectDescriptor *active = activeEffectOrFallback(s.pattern);
  if (active == nullptr || active->effect == nullptr) {
    return;
  }
  active->effect->resetSettings();
}

bool effectsEngineSaveActivePreset(uint8_t slot) {
  const LedEffectState s = snapshotState();
  EffectDescriptor *active = activeEffectOrFallback(s.pattern);
  if (active == nullptr || active->effect == nullptr) {
    return false;
  }
  return active->effect->savePreset(slot);
}

bool effectsEngineLoadActivePreset(uint8_t slot) {
  const LedEffectState s = snapshotState();
  EffectDescriptor *active = activeEffectOrFallback(s.pattern);
  if (active == nullptr || active->effect == nullptr) {
    return false;
  }
  const bool loaded = active->effect->loadPreset(slot);
  if (loaded) {
    active->effect->onActivate(snapshotState());
  }
  return loaded;
}

String effectsEngineActivePresetNamesJson() {
  const LedEffectState s = snapshotState();
  EffectDescriptor *active = activeEffectOrFallback(s.pattern);
  if (active == nullptr) {
    return "[]";
  }

  String out = "[";
  for (uint8_t slot = 1; slot <= 5; ++slot) {
    if (slot > 1) {
      out += ",";
    }
    String name = loadEffectPresetName(active->id, slot);
    if (name.length() == 0) {
      name = "Preset " + String(slot);
    }
    name.replace("\\", "\\\\");
    name.replace("\"", "\\\"");

    out += "{";
    out += "\"slot\":" + String(slot);
    out += ",\"name\":\"" + name + "\"";
    out += "}";
  }
  out += "]";
  return out;
}

bool effectsEngineSetActivePresetName(uint8_t slot, const String &name) {
  const LedEffectState s = snapshotState();
  EffectDescriptor *active = activeEffectOrFallback(s.pattern);
  if (active == nullptr) {
    return false;
  }
  return saveEffectPresetName(active->id, slot, name);
}

String effectsEngineStateJson() {
  const LedEffectState s = snapshotState();
  String out = "{";
  out += "\"power\":";
  out += (s.powerOn ? "true" : "false");
  out += ",\"pattern\":" + String(s.pattern);

  EffectDescriptor *active = activeEffectOrFallback(s.pattern);
  out += ",\"pattern_name\":\"";
  out += (active != nullptr && active->name != nullptr) ? String(active->name) : String("Unknown");
  out += "\"";

  out += ",\"brightness\":" + String(s.brightness);
  out += ",\"speed\":" + String(s.speed);
  out += ",\"fps\":" + String(s.fps);
  out += ",\"fps_max\":" + String(effectsEngineGetMaxFps());
  out += ",\"dither\":";
  out += (s.dither ? "true" : "false");
  out += ",\"red\":" + String(s.red);
  out += ",\"green\":" + String(s.green);
  out += ",\"blue\":" + String(s.blue);
  out += ",\"active_leds\":" + String(g_activeLedCount);
  out += ",\"max_leds\":" + String(effectiveLedCount(LED_STRIP_LENGTH));
  out += "}";
  return out;
}

String effectsEngineCatalogJson() {
  if (effectsCount() == 0) {
    return "[{\"id\":0,\"name\":\"Direct RGB\"}]";
  }
  return effectsCatalogJson();
}
