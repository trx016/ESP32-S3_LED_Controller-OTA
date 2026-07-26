#include "effects_engine.h"

#include <Arduino.h>
#include <FastLED.h>
#include <NeoPixelBus.h>

#include "../Effects/EffectRegistry.h"

#ifndef LED_DATA_PIN
#define LED_DATA_PIN 18
#endif

#ifndef LED_STRIP_LENGTH
#define LED_STRIP_LENGTH 1600
#endif

#ifndef LED_TRANSPORT_MAX
#define LED_TRANSPORT_MAX 256
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

CRGB g_leds[LED_STRIP_LENGTH];
NeoPixelBus<NeoGrbFeature, NeoEsp32BitBangWs2812Method> g_strip(kTransportLedCount, LED_DATA_PIN);
portMUX_TYPE g_effectMux = portMUX_INITIALIZER_UNLOCKED;

LedEffectState g_state = {
    true,
    0,
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

uint16_t effectiveLedCount(uint16_t requestedCount) {
  return requestedCount > kTransportLedCount ? kTransportLedCount : requestedCount;
}

uint8_t scaleChannel(uint8_t value, uint8_t brightness) {
  if (brightness >= 255) {
    return value;
  }

  return static_cast<uint8_t>((static_cast<uint16_t>(value) * static_cast<uint16_t>(brightness)) / 255U);
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
  if (fps < 15) {
    fps = 15;
  }
  if (fps > 600) {
    fps = 600;
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

  g_lastFrameMs = now;
  ++g_frame;
  g_phase = static_cast<uint16_t>(g_phase + map(s.speed, 1, 255, 1, 7));

  EffectDescriptor *active = activeEffectOrFallback(s.pattern);
  if (active != nullptr && active->effect != nullptr) {
    EffectContext ctx = {s, now, g_phase, g_frame};
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

  flushStrip(activeLedCount, s.brightness);
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
  s.pattern = pattern;
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
  if (fps < 15) {
    fps = 15;
  }
  if (fps > 600) {
    fps = 600;
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
}

uint16_t effectsEngineGetActiveLedCount() {
  return g_activeLedCount;
}

uint16_t effectsEngineGetMaxLedCount() {
  return effectiveLedCount(LED_STRIP_LENGTH);
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
  return effectsCatalogJson();
}
