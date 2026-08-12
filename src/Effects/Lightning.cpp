#include "EffectRegistry.h"
#include "EffectPresetStore.h"

#include <FastLED.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

struct Settings {
  uint8_t activity = 24;
  uint8_t minSpread = 8;
  uint8_t maxSpread = 18;
  uint8_t softness = 82;
  uint8_t branching = 28;
};

struct Pulse {
  int16_t center = 0;
  float peak = 0.3f;
  float radius = 8.0f;
  int32_t durationMs = 180;
  int32_t ageMs = 0;
  float flickerHz = 14.0f;
  float phase = 0.0f;
};

class LightningEffect : public IEffect {
 public:
  void begin(CRGB *leds, uint16_t count) override {
    (void)leds;
    energy_.assign(count, 0.0f);
    pulses_.clear();
    burstRemaining_ = 0;
    burstDelayMs_ = 0;
  }

  void render(const EffectContext &ctx, CRGB *leds, uint16_t count) override {
    if (count == 0) {
      return;
    }

    if (energy_.size() != count) {
      energy_.assign(count, 0.0f);
      pulses_.clear();
      burstRemaining_ = 0;
      burstDelayMs_ = 0;
    }

    const int32_t dtMs = std::max<int32_t>(1, ctx.deltaMs > 0 ? static_cast<int32_t>(ctx.deltaMs) : 33);
    const float speed = clampf(static_cast<float>(ctx.state.speed) / 100.0f, 0.25f, 3.0f);

    const float activity = clampf(static_cast<float>(settings_.activity) / 100.0f, 0.01f, 1.0f);
    const float minSpread = clampf(static_cast<float>(settings_.minSpread), 2.0f, 60.0f);
    const float maxSpread = clampf(static_cast<float>(settings_.maxSpread), minSpread, 60.0f);
    const float softness = clampf(static_cast<float>(settings_.softness) / 100.0f, 0.1f, 1.0f);
    const float branching = clampf(static_cast<float>(settings_.branching) / 100.0f, 0.0f, 1.0f);
    const float timingScale = clampf(speed, 0.25f, 3.0f);

    const float strikesPerSecond = (0.10f + (activity * 1.20f)) * (0.60f + (0.80f * speed));
    const float spawnChance = 1.0f - expf(-(strikesPerSecond * (static_cast<float>(dtMs) / 1000.0f)));
    if (randomUnit() < spawnChance) {
      spawnCluster(count, minSpread, maxSpread, branching, activity, timingScale);
    }

    if (burstRemaining_ > 0) {
      burstDelayMs_ -= static_cast<int32_t>(std::max(1.0f, static_cast<float>(dtMs) * timingScale));
      if (burstDelayMs_ <= 0) {
        spawnCluster(count,
                     std::max(2.0f, minSpread * 0.9f),
                     std::max(2.0f, maxSpread * 0.9f),
                     branching * 0.8f,
                     activity * 0.8f,
                     timingScale);
        --burstRemaining_;
        burstDelayMs_ = static_cast<int32_t>(randomFloat(40.0f, 110.0f) / timingScale);
      }
    }

    std::vector<float> target(count, 0.0f);
    std::vector<Pulse> updated;
    updated.reserve(pulses_.size());

    for (auto pulse : pulses_) {
      pulse.ageMs += static_cast<int32_t>(std::max(1.0f, static_cast<float>(dtMs) * timingScale));
      if (pulse.ageMs >= pulse.durationMs) {
        continue;
      }

      const float t = clampf(static_cast<float>(pulse.ageMs) / static_cast<float>(pulse.durationMs), 0.0f, 1.0f);
      const float envelope = pulseEnvelope(t);
      const float flicker =
          0.84f + (0.16f * sinf(((static_cast<float>(pulse.ageMs) / 1000.0f) * pulse.flickerHz * kTau) + pulse.phase));
      const float amp = pulse.peak * envelope * flicker;
      const float sigma = std::max(1.0f, pulse.radius * (0.35f + (0.60f * softness)));

      for (uint16_t i = 0; i < count; ++i) {
        const float d = static_cast<float>(ringDistance(i, pulse.center, count));
        const float x = d / sigma;
        const float falloff = expf(-0.5f * x * x);
        target[i] += amp * falloff;
      }

      updated.push_back(pulse);
    }

    pulses_.swap(updated);

    const float hold = clampf(0.92f - ((speed - 0.25f) * 0.14f) + ((softness - 0.5f) * 0.08f), 0.55f, 0.92f);
    const float riseStep = 0.020f + (0.070f * speed);
    const float fallStep = 0.015f + (0.050f * speed);
    const float fadeFloor = 0.012f;
    for (uint16_t i = 0; i < count; ++i) {
      const float t = clampf(target[i], 0.0f, 1.0f);
      float e = (energy_[i] * hold) + (t * (1.0f - hold));

      if (e > energy_[i]) {
        e = std::min(e, energy_[i] + riseStep);
      } else {
        e = std::max(e, energy_[i] - fallStep);
      }

      energy_[i] = clampf(e, 0.0f, 1.0f);
      if (t <= 0.0005f && energy_[i] < fadeFloor) {
        energy_[i] = 0.0f;
      }

      const float power = powf(energy_[i], 0.72f);
      const uint8_t white = static_cast<uint8_t>(std::min(255.0f, 255.0f * power));
      leds[i] = CRGB(white, white, white);
    }
  }

  String settingsSchemaJson() const override {
    return "["
           "{\"key\":\"activity\",\"label\":\"Activity\",\"type\":\"slider\",\"min\":1,\"max\":100,\"step\":1},"
           "{\"key\":\"minSpread\",\"label\":\"Arc Spread Min\",\"type\":\"slider\",\"min\":2,\"max\":60,\"step\":1},"
           "{\"key\":\"maxSpread\",\"label\":\"Arc Spread Max\",\"type\":\"slider\",\"min\":2,\"max\":60,\"step\":1},"
           "{\"key\":\"softness\",\"label\":\"Softness\",\"type\":\"slider\",\"min\":10,\"max\":100,\"step\":1},"
           "{\"key\":\"branching\",\"label\":\"Branching\",\"type\":\"slider\",\"min\":0,\"max\":100,\"step\":1}"
           "]";
  }

  String settingsStateJson() const override {
    String out = "{";
    out += "\"activity\":" + String(settings_.activity);
    out += ",\"minSpread\":" + String(settings_.minSpread);
    out += ",\"maxSpread\":" + String(settings_.maxSpread);
    out += ",\"softness\":" + String(settings_.softness);
    out += ",\"branching\":" + String(settings_.branching);
    out += "}";
    return out;
  }

  bool setSetting(const String &key, const String &value) override {
    const int intVal = value.toInt();
    if (key == "activity") settings_.activity = static_cast<uint8_t>(constrain(intVal, 1, 100));
    else if (key == "minSpread") settings_.minSpread = static_cast<uint8_t>(constrain(intVal, 2, 60));
    else if (key == "maxSpread") settings_.maxSpread = static_cast<uint8_t>(constrain(intVal, 2, 60));
    else if (key == "softness") settings_.softness = static_cast<uint8_t>(constrain(intVal, 10, 100));
    else if (key == "branching") settings_.branching = static_cast<uint8_t>(constrain(intVal, 0, 100));
    else return false;

    if (settings_.maxSpread < settings_.minSpread) {
      settings_.maxSpread = settings_.minSpread;
    }
    return true;
  }

  void resetSettings() override {
    settings_ = Settings();
  }

  bool savePreset(uint8_t slot) override {
    const PresetData preset = {
        settings_.activity,
        settings_.minSpread,
        settings_.maxSpread,
        settings_.softness,
        settings_.branching,
    };
    return saveEffectPresetBytes(53, slot, reinterpret_cast<const uint8_t *>(&preset), sizeof(preset));
  }

  bool loadPreset(uint8_t slot) override {
    PresetData preset = {};
    if (!loadEffectPresetBytes(53, slot, reinterpret_cast<uint8_t *>(&preset), sizeof(preset))) {
      return false;
    }

    settings_.activity = preset.activity;
    settings_.minSpread = preset.minSpread;
    settings_.maxSpread = std::max(preset.minSpread, preset.maxSpread);
    settings_.softness = preset.softness;
    settings_.branching = preset.branching;
    return true;
  }

 private:
  struct PresetData {
    uint8_t activity;
    uint8_t minSpread;
    uint8_t maxSpread;
    uint8_t softness;
    uint8_t branching;
  };

  static constexpr float kTau = 6.28318531f;

  Settings settings_;
  std::vector<float> energy_;
  std::vector<Pulse> pulses_;
  int16_t burstRemaining_ = 0;
  int32_t burstDelayMs_ = 0;

  static float clampf(float value, float low, float high) {
    return std::max(low, std::min(high, value));
  }

  static float randomUnit() {
    return static_cast<float>(random(0, 10001)) / 10000.0f;
  }

  static float randomFloat(float low, float high) {
    return low + ((high - low) * randomUnit());
  }

  static int16_t ringDistance(int16_t a, int16_t b, uint16_t count) {
    if (count == 0) {
      return static_cast<int16_t>(abs(a - b));
    }
    const int16_t c = static_cast<int16_t>(count);
    const int16_t diff = static_cast<int16_t>(abs(a - b) % c);
    return std::min(diff, static_cast<int16_t>(c - diff));
  }

  static float pulseEnvelope(float t) {
    t = clampf(t, 0.0f, 1.0f);
    if (t < 0.12f) {
      return powf(t / 0.12f, 0.65f);
    }
    const float decay = 1.0f - ((t - 0.12f) / 0.88f);
    return powf(std::max(0.0f, decay), 1.6f);
  }

  void spawnCluster(uint16_t count,
                    float minSpread,
                    float maxSpread,
                    float branching,
                    float activity,
                    float timingScale) {
    if (count == 0) {
      return;
    }

    const int16_t center = static_cast<int16_t>(random(count));
    const float baseSpread = randomFloat(minSpread, maxSpread);
    const float peak = 0.18f + (activity * 0.32f) + randomFloat(0.02f, 0.14f);
    const float radius = std::max(2.0f, baseSpread * randomFloat(0.72f, 1.18f));
    const int32_t duration = static_cast<int32_t>(randomFloat(140.0f, 360.0f) / timingScale);

    Pulse mainPulse;
    mainPulse.center = center;
    mainPulse.peak = std::min(0.82f, peak);
    mainPulse.radius = radius;
    mainPulse.durationMs = std::max<int32_t>(60, duration);
    mainPulse.ageMs = 0;
    mainPulse.flickerHz = randomFloat(10.0f, 18.0f) * clampf(0.75f + (timingScale * 0.25f), 0.75f, 1.5f);
    mainPulse.phase = randomFloat(0.0f, kTau);
    pulses_.push_back(mainPulse);

    int branchCount = 0;
    if (randomUnit() < branching) {
      branchCount = 1 + (randomUnit() < (branching * 0.45f) ? 1 : 0);
    }

    for (int i = 0; i < branchCount; ++i) {
      const int16_t offset = static_cast<int16_t>(randomFloat(-baseSpread * 1.8f, baseSpread * 1.8f));
      Pulse branchPulse;
      branchPulse.center = static_cast<int16_t>((center + offset + static_cast<int16_t>(count)) % count);
      branchPulse.peak = std::min(0.65f, peak * randomFloat(0.45f, 0.72f));
      branchPulse.radius = std::max(2.0f, radius * randomFloat(0.45f, 0.75f));
      branchPulse.durationMs = std::max<int32_t>(45, static_cast<int32_t>(randomFloat(90.0f, 240.0f) / timingScale));
      branchPulse.ageMs = 0;
      branchPulse.flickerHz = randomFloat(12.0f, 22.0f) * clampf(0.75f + (timingScale * 0.25f), 0.75f, 1.5f);
      branchPulse.phase = randomFloat(0.0f, kTau);
      pulses_.push_back(branchPulse);
    }

    if (randomUnit() < (0.18f + (activity * 0.45f))) {
      burstRemaining_ = static_cast<int16_t>(random(1, 3));
      burstDelayMs_ = std::max<int32_t>(12, static_cast<int32_t>(randomFloat(30.0f, 90.0f) / timingScale));
    }
  }
};

REGISTER_EFFECT(LightningEffect, 53, "Lightning");

}  // namespace