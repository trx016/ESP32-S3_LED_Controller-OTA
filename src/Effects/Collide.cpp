#include "EffectRegistry.h"
#include "EffectPresetStore.h"

#include <FastLED.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

struct Settings {
  uint8_t speedMin = 28;
  uint8_t speedMax = 86;
  uint16_t impactRadius = 24;
  uint16_t impactDurationMs = 320;
  uint8_t particleCount = 18;
  uint8_t burstSpeedSpread = 160;
  uint8_t saturation = 255;
  uint8_t hueDrift = 26;
  uint8_t trailHold = 68;
  uint8_t glowWidth = 11;
  uint8_t burstBoost = 145;
};

struct BurstParticle {
  float pos;
  float vel;
  float ageMs;
  float lifeMs;
  uint8_t hue;
  float size;
  float intensity;
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
    huePhase_ = 0.0f;
    burstParticles_.clear();
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
      huePhase_ = 0.0f;
      burstParticles_.clear();
    }

    const float speedScale = clampf(static_cast<float>(ctx.state.speed) / 128.0f, 0.20f, 3.00f);
    const int32_t dtMs = std::max<int32_t>(1, ctx.deltaMs > 0 ? static_cast<int32_t>(ctx.deltaMs) : 33);
    const float dtSec = (static_cast<float>(dtMs) / 1000.0f) * speedScale;
    const float hueDrift = clampf(static_cast<float>(settings_.hueDrift), 0.0f, 80.0f);
    huePhase_ = fmodf(huePhase_ + (static_cast<float>(dtMs) * (0.08f + (hueDrift / 220.0f)) * speedScale), 256.0f);

    std::vector<float> target(count, 0.0f);
    std::vector<float> hueSum(count, 0.0f);
    std::vector<float> hueWeight(count, 0.0f);

    if (!active_ && !exploding_) {
      cooldownMs_ -= static_cast<int32_t>(std::max(1.0f, static_cast<float>(dtMs) * speedScale));
      if (cooldownMs_ <= 0) {
        spawnPair(count);
      }
    }

    if (active_) {
      posA_ = wrapPos(posA_ + (velA_ * dtSec), count);
      posB_ = wrapPos(posB_ + (velB_ * dtSec), count);

      const float glowSigma = clampf(static_cast<float>(settings_.glowWidth) / 10.0f, 0.6f, 2.2f);
      addColoredSpark(target, hueSum, hueWeight, posA_, hueA_, 1.0f, glowSigma, count);
      addColoredSpark(target, hueSum, hueWeight, posB_, hueB_, 1.0f, glowSigma, count);

      const float dist = ringDistance(posA_, posB_, count);
      if (dist <= std::max(1.0f, (std::fabs(velA_) + std::fabs(velB_)) * dtSec * 0.8f)) {
        active_ = false;
        exploding_ = true;
        explosionAgeMs_ = 0;
        const float toward = signedRingDelta(posA_, posB_, count) * 0.5f;
        explosionCenter_ = wrapPos(posA_ + toward, count);
        hueBoom_ = static_cast<uint8_t>(((static_cast<uint16_t>(hueA_) + static_cast<uint16_t>(hueB_)) / 2U) + random8());
        spawnBurst(explosionCenter_, count);
      }
    }

    if (exploding_) {
      const int32_t impactDuration = std::max<int32_t>(80, settings_.impactDurationMs);
      const float burstBoost = clampf(static_cast<float>(settings_.burstBoost) / 100.0f, 0.8f, 2.2f);
      const float baseSigma = clampf(static_cast<float>(settings_.glowWidth) / 10.0f, 0.6f, 2.2f);
      const float impactRadius = clampf(static_cast<float>(settings_.impactRadius), 4.0f, static_cast<float>(count));

      std::vector<BurstParticle> survivors;
      survivors.reserve(burstParticles_.size());
      for (BurstParticle &particle : burstParticles_) {
        particle.ageMs += std::max(1.0f, static_cast<float>(dtMs) * speedScale);
        const float lifeMs = std::max(60.0f, particle.lifeMs);
        if (particle.ageMs >= lifeMs) {
          continue;
        }

        const float t = clampf(particle.ageMs / lifeMs, 0.0f, 1.0f);
        particle.pos = wrapPos(particle.pos + (particle.vel * dtSec), count);

        const float envelope = powf(std::max(0.0f, 1.0f - t), 0.72f) * burstBoost;
        float sigma = baseSigma * particle.size * (0.8f + (0.9f * (1.0f - t)));
        sigma = clampf(sigma, 0.35f, std::max(0.35f, impactRadius * 0.25f));

        addColoredSpark(target,
                        hueSum,
                        hueWeight,
                        particle.pos,
                        particle.hue,
                        envelope * particle.intensity,
                        sigma,
                        count);
        survivors.push_back(particle);
      }
      burstParticles_.swap(survivors);

      explosionAgeMs_ += static_cast<int32_t>(std::max(1.0f, static_cast<float>(dtMs) * speedScale));
      if (burstParticles_.empty() || explosionAgeMs_ > std::max<int32_t>(5000, impactDuration * 6)) {
        exploding_ = false;
        cooldownMs_ = random(120, 581);
        burstParticles_.clear();
      }
    }

    const float hold = clampf(static_cast<float>(settings_.trailHold) / 100.0f, 0.45f, 0.92f);
    const uint8_t sat = static_cast<uint8_t>(constrain(static_cast<int>(settings_.saturation), 120, 255));
    for (uint16_t i = 0; i < count; ++i) {
      float e = (energy_[i] * hold) + (target[i] * (1.0f - hold));
      if (target[i] <= 0.0005f && e < 0.006f) {
        e = 0.0f;
      }
      energy_[i] = clampf(e, 0.0f, 1.0f);

      const uint8_t value = static_cast<uint8_t>(std::min(255.0f, 255.0f * powf(energy_[i], 0.78f)));
      const float total = hueWeight[i];
      if (value == 0 || total <= 0.0001f) {
        leds[i] = CRGB::Black;
        continue;
      }

      const uint8_t fallbackHue = static_cast<uint8_t>((((static_cast<uint16_t>(hueA_) + static_cast<uint16_t>(hueB_)) / 2U) +
                                                        static_cast<uint8_t>(huePhase_ * 0.5f)) &
                                                       0xFFU);
      const float hue = (total > 0.0f) ? (hueSum[i] / total) : static_cast<float>(fallbackHue);
      leds[i] = CHSV(static_cast<uint8_t>(static_cast<int>(hue) & 0xFF), sat, value);
    }
  }

  String settingsSchemaJson() const override {
    return "["
           "{\"key\":\"speedMin\",\"label\":\"Speed Min\",\"type\":\"slider\",\"min\":4,\"max\":120,\"step\":1},"
           "{\"key\":\"speedMax\",\"label\":\"Speed Max\",\"type\":\"slider\",\"min\":4,\"max\":180,\"step\":1},"
           "{\"key\":\"impactRadius\",\"label\":\"Impact Radius\",\"type\":\"slider\",\"min\":4,\"max\":2000,\"step\":1},"
           "{\"key\":\"impactDurationMs\",\"label\":\"Impact Hold ms\",\"type\":\"slider\",\"min\":80,\"max\":1200,\"step\":10},"
           "{\"key\":\"particleCount\",\"label\":\"Particle Count\",\"type\":\"slider\",\"min\":4,\"max\":40,\"step\":1},"
           "{\"key\":\"burstSpeedSpread\",\"label\":\"Burst Speed Spread\",\"type\":\"slider\",\"min\":20,\"max\":260,\"step\":1},"
           "{\"key\":\"saturation\",\"label\":\"Saturation\",\"type\":\"slider\",\"min\":120,\"max\":255,\"step\":1},"
           "{\"key\":\"hueDrift\",\"label\":\"Hue Drift\",\"type\":\"slider\",\"min\":0,\"max\":80,\"step\":1},"
           "{\"key\":\"trailHold\",\"label\":\"Trail Hold\",\"type\":\"slider\",\"min\":45,\"max\":92,\"step\":1},"
           "{\"key\":\"glowWidth\",\"label\":\"Glow Width\",\"type\":\"slider\",\"min\":6,\"max\":22,\"step\":1},"
           "{\"key\":\"burstBoost\",\"label\":\"Burst Boost\",\"type\":\"slider\",\"min\":80,\"max\":220,\"step\":1}"
           "]";
  }

  String settingsStateJson() const override {
    String out = "{";
    out += "\"speedMin\":" + String(settings_.speedMin);
    out += ",\"speedMax\":" + String(settings_.speedMax);
    out += ",\"impactRadius\":" + String(settings_.impactRadius);
    out += ",\"impactDurationMs\":" + String(settings_.impactDurationMs);
    out += ",\"particleCount\":" + String(settings_.particleCount);
    out += ",\"burstSpeedSpread\":" + String(settings_.burstSpeedSpread);
    out += ",\"saturation\":" + String(settings_.saturation);
    out += ",\"hueDrift\":" + String(settings_.hueDrift);
    out += ",\"trailHold\":" + String(settings_.trailHold);
    out += ",\"glowWidth\":" + String(settings_.glowWidth);
    out += ",\"burstBoost\":" + String(settings_.burstBoost);
    out += "}";
    return out;
  }

  bool setSetting(const String &key, const String &value) override {
    const int intVal = value.toInt();
    if (key == "speedMin") settings_.speedMin = static_cast<uint8_t>(constrain(intVal, 4, 120));
    else if (key == "speedMax") settings_.speedMax = static_cast<uint8_t>(constrain(intVal, 4, 180));
    else if (key == "impactRadius") settings_.impactRadius = static_cast<uint16_t>(constrain(intVal, 4, 2000));
    else if (key == "impactDurationMs") settings_.impactDurationMs = static_cast<uint16_t>(constrain(intVal, 80, 1200));
    else if (key == "particleCount") settings_.particleCount = static_cast<uint8_t>(constrain(intVal, 4, 40));
    else if (key == "burstSpeedSpread") settings_.burstSpeedSpread = static_cast<uint8_t>(constrain(intVal, 20, 260));
    else if (key == "saturation") settings_.saturation = static_cast<uint8_t>(constrain(intVal, 120, 255));
    else if (key == "hueDrift") settings_.hueDrift = static_cast<uint8_t>(constrain(intVal, 0, 80));
    else if (key == "trailHold") settings_.trailHold = static_cast<uint8_t>(constrain(intVal, 45, 92));
    else if (key == "glowWidth") settings_.glowWidth = static_cast<uint8_t>(constrain(intVal, 6, 22));
    else if (key == "burstBoost") settings_.burstBoost = static_cast<uint8_t>(constrain(intVal, 80, 220));
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
      settings_.particleCount,
      settings_.burstSpeedSpread,
      settings_.saturation,
      settings_.hueDrift,
      settings_.trailHold,
      settings_.glowWidth,
      settings_.burstBoost,
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
    settings_.particleCount = preset.particleCount;
    settings_.burstSpeedSpread = preset.burstSpeedSpread;
    settings_.saturation = preset.saturation;
    settings_.hueDrift = preset.hueDrift;
    settings_.trailHold = preset.trailHold;
    settings_.glowWidth = preset.glowWidth;
    settings_.burstBoost = preset.burstBoost;

    settings_.speedMin = static_cast<uint8_t>(constrain(settings_.speedMin, 4, 120));
    settings_.speedMax = static_cast<uint8_t>(constrain(settings_.speedMax, settings_.speedMin, 180));
    settings_.impactRadius = static_cast<uint16_t>(constrain(static_cast<int>(settings_.impactRadius), 4, 2000));
    settings_.impactDurationMs = static_cast<uint16_t>(constrain(static_cast<int>(settings_.impactDurationMs), 80, 1200));
    settings_.particleCount = static_cast<uint8_t>(constrain(settings_.particleCount, 4, 40));
    settings_.burstSpeedSpread = static_cast<uint8_t>(constrain(settings_.burstSpeedSpread, 20, 255));
    settings_.saturation = static_cast<uint8_t>(constrain(settings_.saturation, 120, 255));
    settings_.hueDrift = static_cast<uint8_t>(constrain(settings_.hueDrift, 0, 80));
    settings_.trailHold = static_cast<uint8_t>(constrain(settings_.trailHold, 45, 92));
    settings_.glowWidth = static_cast<uint8_t>(constrain(settings_.glowWidth, 6, 22));
    settings_.burstBoost = static_cast<uint8_t>(constrain(settings_.burstBoost, 80, 220));

    return true;
  }

 private:
  struct PresetData {
    uint8_t speedMin;
    uint8_t speedMax;
    uint16_t impactRadius;
    uint16_t impactDurationMs;
    uint8_t particleCount;
    uint8_t burstSpeedSpread;
    uint8_t saturation;
    uint8_t hueDrift;
    uint8_t trailHold;
    uint8_t glowWidth;
    uint8_t burstBoost;
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
  float huePhase_ = 0.0f;
  std::vector<BurstParticle> burstParticles_;
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
    return minS + (maxS - minS) * randomUnit();
  }

  static float randomUnit() {
    return static_cast<float>(random(0, 10001)) / 10000.0f;
  }

  static float randomRange(float minV, float maxV) {
    return minV + ((maxV - minV) * randomUnit());
  }

  static float gaussianGlow(float distance, float sigma) {
    const float x = distance / std::max(0.3f, sigma);
    return expf(-0.5f * x * x);
  }

  void addColoredSpark(std::vector<float> &target,
                       std::vector<float> &hueSum,
                       std::vector<float> &hueWeight,
                       float center,
                       uint8_t hue,
                       float intensity,
                       float sigma,
                       uint16_t count) {
    if (intensity <= 0.0f) {
      return;
    }

    for (uint16_t i = 0; i < count; ++i) {
      const float d = ringDistance(static_cast<float>(i), center, count);
      const float glow = gaussianGlow(d, sigma);
      const float amount = glow * intensity;
      target[i] = std::max(target[i], amount);
      hueSum[i] += amount * static_cast<float>(hue);
      hueWeight[i] += amount;
    }
  }

  void spawnBurst(float center, uint16_t count) {
    const uint8_t particleCount = static_cast<uint8_t>(constrain(settings_.particleCount, 4, 40));
    const float speedSpread = clampf(static_cast<float>(settings_.burstSpeedSpread) / 100.0f, 0.2f, 2.6f);
    const float lifeMs = std::max(80.0f, static_cast<float>(settings_.impactDurationMs));
    const float impactRadius = clampf(static_cast<float>(settings_.impactRadius), 4.0f, static_cast<float>(count));

    const float minMul = std::max(0.18f, 1.0f - (speedSpread * 0.50f));
    const float maxMul = 1.0f + speedSpread;

    burstParticles_.clear();
    burstParticles_.reserve(particleCount);
    for (uint8_t idx = 0; idx < particleCount; ++idx) {
      float direction = (idx % 2 == 0) ? -1.0f : 1.0f;
      direction *= (random(0, 4) == 3) ? -1.0f : 1.0f;

      const float travelGoal = impactRadius * randomRange(0.40f, 1.00f);
      const float particleLife = lifeMs * randomRange(0.62f, 1.12f);
      const float baseSpeed = travelGoal / std::max(0.06f, particleLife / 1000.0f);
      const float speed = baseSpeed * randomRange(minMul, maxMul);
      const uint8_t hue = static_cast<uint8_t>(hueBoom_ + static_cast<int8_t>(random(-22, 23)));

      burstParticles_.push_back({center,
                                 direction * speed,
                                 0.0f,
                                 particleLife,
                                 hue,
                                 randomRange(0.65f, 1.55f),
                                 1.0f});
    }

    const uint8_t sparkCount = std::max<uint8_t>(2, particleCount / 3);
    for (uint8_t idx = 0; idx < sparkCount; ++idx) {
      const float direction = randomUnit() < 0.5f ? -1.0f : 1.0f;
      const float travelGoal = impactRadius * randomRange(0.35f, 1.10f);
      const float particleLife = lifeMs * randomRange(0.30f, 0.72f);
      const float baseSpeed = travelGoal / std::max(0.04f, particleLife / 1000.0f);
      const float speed = baseSpeed * randomRange(0.95f, 1.55f + (speedSpread * 0.25f));
      const uint8_t hue = random8();

      burstParticles_.push_back({center,
                                 direction * speed,
                                 0.0f,
                                 particleLife,
                                 hue,
                                 randomRange(0.16f, 0.34f),
                                 randomRange(1.15f, 1.55f)});
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
    burstParticles_.clear();
  }
};

REGISTER_EFFECT(CollideEffect, 54, "Collide");

}  // namespace
