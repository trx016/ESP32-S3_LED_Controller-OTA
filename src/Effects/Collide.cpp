#include "EffectRegistry.h"
#include "EffectPresetStore.h"

#include <FastLED.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

struct Settings {
  uint8_t speedMin = 24;
  uint8_t speedMax = 68;
  uint8_t impactRadius = 16;
  uint16_t impactDurationMs = 260;
};

class CollideEffect : public IEffect {
 public:
  void begin(CRGB *leds, uint16_t count) override {
    (void)leds;
    active_ = false;
    exploding_ = false;
    cooldownMs_ = 0;
    explosionAgeMs_ = 0;
    explosionCenter_ = 0.0f;
    hueA_ = random8();
    hueB_ = random8();
    hueBoom_ = random8();
    huePhase_ = 0;
    energy_.assign(count, 0.0f);
  }

  void render(const EffectContext &ctx, CRGB *leds, uint16_t count) override {
    if (count == 0) {
      return;
    }

    if (energy_.size() != count) {
      energy_.assign(count, 0.0f);
      active_ = false;
      exploding_ = false;
      cooldownMs_ = 0;
      explosionAgeMs_ = 0;
      explosionCenter_ = 0.0f;
      huePhase_ = 0;
    }

    const float speedScale = clampf(static_cast<float>(ctx.state.speed) / 128.0f, 0.20f, 3.00f);
    const int32_t dtMs = std::max<int32_t>(1, ctx.deltaMs > 0 ? static_cast<int32_t>(ctx.deltaMs) : 33);
    const float dtSec = (static_cast<float>(dtMs) / 1000.0f) * speedScale;
    huePhase_ = static_cast<uint16_t>(huePhase_ + static_cast<uint16_t>(std::max(1.0f, static_cast<float>(dtMs) * 0.2f * speedScale)));

    std::vector<float> target(count, 0.0f);
    float boomRadius = 1.0f;
    float boomT = 1.0f;

    if (!active_ && !exploding_) {
      cooldownMs_ -= static_cast<int32_t>(std::max(1.0f, static_cast<float>(dtMs) * speedScale));
      if (cooldownMs_ <= 0) {
        spawnPair(count);
      }
    }

    if (active_) {
      posA_ = wrapPos(posA_ + (velA_ * dtSec), count);
      posB_ = wrapPos(posB_ + (velB_ * dtSec), count);

      addSpark(target, posA_, 1.0f, count);
      addSpark(target, posB_, 1.0f, count);

      const float dist = ringDistance(posA_, posB_, count);
      if (dist <= std::max(1.0f, (std::fabs(velA_) + std::fabs(velB_)) * dtSec * 0.8f)) {
        active_ = false;
        exploding_ = true;
        explosionAgeMs_ = 0;
        const float toward = signedRingDelta(posA_, posB_, count) * 0.5f;
        explosionCenter_ = wrapPos(posA_ + toward, count);
        hueBoom_ = static_cast<uint8_t>(((static_cast<uint16_t>(hueA_) + static_cast<uint16_t>(hueB_)) / 2U) + random8());
      }
    }

    if (exploding_) {
      explosionAgeMs_ += static_cast<int32_t>(std::max(1.0f, static_cast<float>(dtMs) * speedScale));
      const float t = clampf(static_cast<float>(explosionAgeMs_) / static_cast<float>(settings_.impactDurationMs), 0.0f, 1.0f);
      const float envelope = powf(std::max(0.0f, 1.0f - t), 1.2f);
      const float radius = std::max(2.0f, static_cast<float>(settings_.impactRadius) * (0.30f + (0.70f * t)));
      boomRadius = radius;
      boomT = t;

      for (uint16_t i = 0; i < count; ++i) {
        const float d = ringDistance(static_cast<float>(i), explosionCenter_, count);
        const float x = d / radius;
        const float glow = expf(-0.5f * x * x);
        target[i] = std::max(target[i], envelope * glow);
      }

      if (t >= 1.0f) {
        exploding_ = false;
        cooldownMs_ = random(120, 581);
      }
    }

    const float hold = 0.74f;
    for (uint16_t i = 0; i < count; ++i) {
      float e = (energy_[i] * hold) + (target[i] * (1.0f - hold));
      if (target[i] <= 0.0005f && e < 0.006f) {
        e = 0.0f;
      }
      energy_[i] = clampf(e, 0.0f, 1.0f);

      const float dA = ringDistance(static_cast<float>(i), posA_, count);
      const float dB = ringDistance(static_cast<float>(i), posB_, count);
      const float dBoom = ringDistance(static_cast<float>(i), explosionCenter_, count);

      float wA = active_ ? gaussianGlow(dA, 1.0f) : 0.0f;
      float wB = active_ ? gaussianGlow(dB, 1.0f) : 0.0f;
      float wBoom = exploding_ ? gaussianGlow(dBoom, boomRadius) * (0.75f + (0.25f * (1.0f - boomT))) : 0.0f;
      const float total = wA + wB + wBoom;

      const uint8_t value = static_cast<uint8_t>(std::min(255.0f, 255.0f * powf(energy_[i], 0.78f)));
      if (value == 0 || total <= 0.0001f) {
        leds[i] = CRGB::Black;
        continue;
      }

      const float hueBoomShifted = static_cast<float>(static_cast<uint8_t>(hueBoom_ + ((i * 5U) & 0xFFU) + static_cast<uint8_t>(huePhase_ >> 3)));
      const float hue = ((wA * static_cast<float>(hueA_)) +
                         (wB * static_cast<float>(hueB_)) +
                         (wBoom * hueBoomShifted)) /
                        total;
      leds[i] = CHSV(static_cast<uint8_t>(static_cast<int>(hue) & 0xFF), 245, value);
    }
  }

  String settingsSchemaJson() const override {
    return "["
           "{\"key\":\"speedMin\",\"label\":\"Speed Min\",\"type\":\"slider\",\"min\":4,\"max\":120,\"step\":1},"
           "{\"key\":\"speedMax\",\"label\":\"Speed Max\",\"type\":\"slider\",\"min\":4,\"max\":160,\"step\":1},"
           "{\"key\":\"impactRadius\",\"label\":\"Impact Radius\",\"type\":\"slider\",\"min\":4,\"max\":80,\"step\":1},"
           "{\"key\":\"impactDurationMs\",\"label\":\"Impact Hold ms\",\"type\":\"slider\",\"min\":120,\"max\":900,\"step\":10}"
           "]";
  }

  String settingsStateJson() const override {
    String out = "{";
    out += "\"speedMin\":" + String(settings_.speedMin);
    out += ",\"speedMax\":" + String(settings_.speedMax);
    out += ",\"impactRadius\":" + String(settings_.impactRadius);
    out += ",\"impactDurationMs\":" + String(settings_.impactDurationMs);
    out += "}";
    return out;
  }

  bool setSetting(const String &key, const String &value) override {
    const int intVal = value.toInt();
    if (key == "speedMin") settings_.speedMin = static_cast<uint8_t>(constrain(intVal, 4, 120));
    else if (key == "speedMax") settings_.speedMax = static_cast<uint8_t>(constrain(intVal, 4, 160));
    else if (key == "impactRadius") settings_.impactRadius = static_cast<uint8_t>(constrain(intVal, 4, 80));
    else if (key == "impactDurationMs") settings_.impactDurationMs = static_cast<uint16_t>(constrain(intVal, 120, 900));
    else return false;

    if (settings_.speedMax < settings_.speedMin) {
      settings_.speedMax = settings_.speedMin;
    }
    return true;
  }

  void resetSettings() override {
    settings_ = Settings();
  }

  bool savePreset(uint8_t slot) override {
    const PresetData preset = {
        settings_.speedMin,
        settings_.speedMax,
        settings_.impactRadius,
        settings_.impactDurationMs,
    };
    return saveEffectPresetBytes(54, slot, reinterpret_cast<const uint8_t *>(&preset), sizeof(preset));
  }

  bool loadPreset(uint8_t slot) override {
    PresetData preset = {};
    if (!loadEffectPresetBytes(54, slot, reinterpret_cast<uint8_t *>(&preset), sizeof(preset))) {
      return false;
    }

    settings_.speedMin = preset.speedMin;
    settings_.speedMax = std::max(preset.speedMin, preset.speedMax);
    settings_.impactRadius = preset.impactRadius;
    settings_.impactDurationMs = preset.impactDurationMs;
    return true;
  }

 private:
  struct PresetData {
    uint8_t speedMin;
    uint8_t speedMax;
    uint8_t impactRadius;
    uint16_t impactDurationMs;
  };

  Settings settings_;
  bool active_ = false;
  bool exploding_ = false;
  float posA_ = 0.0f;
  float posB_ = 0.0f;
  float velA_ = 0.0f;
  float velB_ = 0.0f;
  float explosionCenter_ = 0.0f;
  int32_t cooldownMs_ = 0;
  int32_t explosionAgeMs_ = 0;
  uint8_t hueA_ = 0;
  uint8_t hueB_ = 0;
  uint8_t hueBoom_ = 0;
  uint16_t huePhase_ = 0;
  std::vector<float> energy_;

  static float clampf(float value, float low, float high) {
    return std::max(low, std::min(high, value));
  }

  static float wrapPos(float p, uint16_t count) {
    if (count == 0) {
      return 0.0f;
    }
    const float c = static_cast<float>(count);
    float out = fmodf(p, c);
    if (out < 0.0f) {
      out += c;
    }
    return out;
  }

  static float ringDistance(float a, float b, uint16_t count) {
    if (count == 0) {
      return fabsf(a - b);
    }
    const float c = static_cast<float>(count);
    const float d = fmodf(fabsf(a - b), c);
    return std::min(d, c - d);
  }

  static float signedRingDelta(float src, float dst, uint16_t count) {
    if (count == 0) {
      return dst - src;
    }
    const float c = static_cast<float>(count);
    float delta = fmodf(dst - src, c);
    if (delta < 0.0f) {
      delta += c;
    }
    if (delta > (c * 0.5f)) {
      delta -= c;
    }
    return delta;
  }

  float randomSpeedLedsPerSec() const {
    const float minS = static_cast<float>(settings_.speedMin);
    const float maxS = static_cast<float>(settings_.speedMax);
    return minS + (maxS - minS) * (static_cast<float>(random(0, 10001)) / 10000.0f);
  }

  static float gaussianGlow(float distance, float sigma) {
    const float x = distance / std::max(0.3f, sigma);
    return expf(-0.5f * x * x);
  }

  void addSpark(std::vector<float> &target, float center, float strength, uint16_t count) {
    const float sigma = 0.9f;
    for (uint16_t i = 0; i < count; ++i) {
      const float d = ringDistance(static_cast<float>(i), center, count);
      const float glow = gaussianGlow(d, sigma);
      target[i] = std::max(target[i], strength * glow);
    }
  }

  void spawnPair(uint16_t count) {
    if (count < 2) {
      return;
    }

    const int16_t base = random(count);
    const int16_t minSep = std::max<int16_t>(2, static_cast<int16_t>(count / 5));
    const int16_t maxSep = std::max<int16_t>(minSep + 1, static_cast<int16_t>((count * 4) / 5));
    const int16_t sep = random(minSep, maxSep + 1);

    const float c = static_cast<float>(count);
    posA_ = wrapPos(static_cast<float>(base) - (static_cast<float>(sep) * 0.5f), count);
    posB_ = wrapPos(static_cast<float>(base) + (static_cast<float>(sep) * 0.5f), count);

    const float towardB = signedRingDelta(posA_, posB_, count);
    const float dirA = towardB >= 0.0f ? 1.0f : -1.0f;
    const float dirB = -dirA;

    velA_ = dirA * randomSpeedLedsPerSec();
    velB_ = dirB * randomSpeedLedsPerSec();
    hueA_ = random8();
    hueB_ = static_cast<uint8_t>(hueA_ + random(70, 191));

    active_ = true;
    exploding_ = false;
    explosionAgeMs_ = 0;
  }
};

REGISTER_EFFECT(CollideEffect, 54, "Collide");

}  // namespace
