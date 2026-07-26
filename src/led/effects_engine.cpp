#include "effects_engine.h"

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>

#include "../Effects/EffectRegistry.h"

#ifndef LED_DATA_PIN
#define LED_DATA_PIN 18
#endif

#ifndef LED_STRIP_LENGTH
#define LED_STRIP_LENGTH 1600
#endif

#ifndef LED_COLOR_ORDER
#define LED_COLOR_ORDER GRB
#endif

#ifndef LED_TYPE
#define LED_TYPE WS2812B
#endif

#ifndef DEBUG_EFFECTS_OUTPUT_MAX
#define DEBUG_EFFECTS_OUTPUT_MAX 0
#endif

namespace {

CRGB g_leds[LED_STRIP_LENGTH];
Adafruit_NeoPixel g_strip(LED_STRIP_LENGTH, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
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

uint16_t effectiveLedCount(uint16_t requestedCount) {
  uint16_t clamped = requestedCount > LED_STRIP_LENGTH ? LED_STRIP_LENGTH : requestedCount;
#if defined(DEBUG_EFFECTS_OUTPUT_MAX) && (DEBUG_EFFECTS_OUTPUT_MAX > 0)
  if (clamped > DEBUG_EFFECTS_OUTPUT_MAX) {
    clamped = DEBUG_EFFECTS_OUTPUT_MAX;
  }
#endif
  return clamped;
}

uint8_t scaleChannel(uint8_t value, uint8_t brightness) {
  if (brightness >= 255) {
    return value;
  }

  return static_cast<uint8_t>((static_cast<uint16_t>(value) * static_cast<uint16_t>(brightness)) / 255U);
}

void flushStrip(uint16_t activeLedCount, uint8_t brightness) {
  const uint16_t clampedCount = effectiveLedCount(activeLedCount);

  for (uint16_t i = 0; i < clampedCount; ++i) {
    const CRGB &pixel = g_leds[i];
    g_strip.setPixelColor(i,
                          g_strip.Color(scaleChannel(pixel.r, brightness),
                                        scaleChannel(pixel.g, brightness),
                                        scaleChannel(pixel.b, brightness)));
  }

  for (uint16_t i = clampedCount; i < LED_STRIP_LENGTH; ++i) {
    g_strip.setPixelColor(i, 0);
  }

  g_strip.show();
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
  Serial.println("[effects] transport init begin");
  fill_solid(g_leds, LED_STRIP_LENGTH, CRGB::Black);
  g_strip.begin();
  g_strip.clear();
  g_strip.show();
  Serial.println("[effects] transport init ok");
  Serial.print("[effects] effective output max: ");
  Serial.println(effectiveLedCount(LED_STRIP_LENGTH));

#if !defined(DEBUG_DISABLE_EFFECTS_PLUGIN_INIT) || (DEBUG_DISABLE_EFFECTS_PLUGIN_INIT == 0)
  Serial.println("[effects] plugin init begin");
  initializeEffectsIfNeeded();
  Serial.println("[effects] plugin init ok");
#else
  Serial.println("[effects] plugin init skipped");
#endif
}

void effectsEngineTick() {
  const uint32_t now = millis();
  const LedEffectState s = snapshotState();
  const uint16_t activeLedCount = effectiveLedCount(g_activeLedCount);

  if (!s.powerOn || s.brightness == 0) {
    fill_solid(g_leds, LED_STRIP_LENGTH, CRGB::Black);
    flushStrip(activeLedCount, 0);
    return;
  }

  const uint16_t requestedFrameDelay = clampFrameDelayMs(s.fps);
  const uint16_t transportFrameDelay = transportMinFrameDelayMs(activeLedCount);
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
  g_activeLedCount = static_cast<uint16_t>(constrain(count, static_cast<uint16_t>(1), static_cast<uint16_t>(LED_STRIP_LENGTH)));
}

uint16_t effectsEngineGetActiveLedCount() {
  return g_activeLedCount;
}

uint16_t effectsEngineGetMaxLedCount() {
  return LED_STRIP_LENGTH;
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
  out += ",\"max_leds\":" + String(LED_STRIP_LENGTH);
  out += "}";
  return out;
}

String effectsEngineCatalogJson() {
  return effectsCatalogJson();
}
