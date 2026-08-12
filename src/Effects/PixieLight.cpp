#include "EffectRegistry.h"
#include "EffectPresetStore.h"

#include <FastLED.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

struct Settings {
  uint8_t breathMin = 20;
  uint8_t breathMax = 220;
  uint8_t rateMin = 18;
  uint8_t rateMax = 70;
  uint8_t saturation = 210;
  uint8_t hueJitter = 70;
};

class PixieLightEffect : public IEffect {
 public:
  void begin(CRGB *leds, uint16_t count) override {
    (void)leds;
    allocate(count);
  }

  void render(const EffectContext &ctx, CRGB *leds, uint16_t count) override {
    if (count == 0) {
      return;
    }

    if (baseHue_.size() != count || phase_.size() != count || rateHz_.size() != count) {
      allocate(count);
    }

    const int32_t dtMs = std::max<int32_t>(1, ctx.deltaMs > 0 ? static_cast<int32_t>(ctx.deltaMs) : 33);
    const float dtSec = static_cast<float>(dtMs) / 1000.0f;
    const float speedScale = clampf(static_cast<float>(ctx.state.speed) / 128.0f, 0.25f, 3.0f);

    const uint8_t lo = std::min(settings_.breathMin, settings_.breathMax);
    const uint8_t hi = std::max(settings_.breathMin, settings_.breathMax);
    const float valRange = static_cast<float>(std::max<uint8_t>(1, hi - lo));

    for (uint16_t i = 0; i < count; ++i) {
      phase_[i] += rateHz_[i] * dtSec * speedScale * 6.28318531f;
      if (phase_[i] > 6.28318531f) {
        phase_[i] = fmodf(phase_[i], 6.28318531f);
      }

      const float wave = 0.5f + (0.5f * sinf(phase_[i]));
      const uint8_t value = static_cast<uint8_t>(std::min(255.0f, static_cast<float>(lo) + (valRange * wave)));

      uint8_t hue = baseHue_[i];
      if (settings_.hueJitter > 0) {
        const int8_t jitter = static_cast<int8_t>(map(sin8(static_cast<uint8_t>((phase_[i] * 40.0f))),
                                                      0,
                                                      255,
                                                      -static_cast<int>(settings_.hueJitter),
                                                      static_cast<int>(settings_.hueJitter)));
        hue = static_cast<uint8_t>(hue + jitter);
      }

      leds[i] = CHSV(hue, settings_.saturation, value);
    }
  }

  String settingsSchemaJson() const override {
    return "["
           "{\"key\":\"breathMin\",\"label\":\"Breath Min\",\"type\":\"slider\",\"min\":1,\"max\":255,\"step\":1},"
           "{\"key\":\"breathMax\",\"label\":\"Breath Max\",\"type\":\"slider\",\"min\":1,\"max\":255,\"step\":1},"
           "{\"key\":\"rateMin\",\"label\":\"Rate Min\",\"type\":\"slider\",\"min\":5,\"max\":120,\"step\":1},"
           "{\"key\":\"rateMax\",\"label\":\"Rate Max\",\"type\":\"slider\",\"min\":5,\"max\":140,\"step\":1},"
           "{\"key\":\"saturation\",\"label\":\"Saturation\",\"type\":\"slider\",\"min\":0,\"max\":255,\"step\":1},"
           "{\"key\":\"hueJitter\",\"label\":\"Hue Drift\",\"type\":\"slider\",\"min\":0,\"max\":100,\"step\":1}"
           "]";
  }

  String settingsStateJson() const override {
    String out = "{";
    out += "\"breathMin\":" + String(settings_.breathMin);
    out += ",\"breathMax\":" + String(settings_.breathMax);
    out += ",\"rateMin\":" + String(settings_.rateMin);
    out += ",\"rateMax\":" + String(settings_.rateMax);
    out += ",\"saturation\":" + String(settings_.saturation);
    out += ",\"hueJitter\":" + String(settings_.hueJitter);
    out += "}";
    return out;
  }

  bool setSetting(const String &key, const String &value) override {
    const int intVal = value.toInt();
    if (key == "breathMin") settings_.breathMin = static_cast<uint8_t>(constrain(intVal, 1, 255));
    else if (key == "breathMax") settings_.breathMax = static_cast<uint8_t>(constrain(intVal, 1, 255));
    else if (key == "rateMin") settings_.rateMin = static_cast<uint8_t>(constrain(intVal, 5, 120));
    else if (key == "rateMax") settings_.rateMax = static_cast<uint8_t>(constrain(intVal, 5, 140));
    else if (key == "saturation") settings_.saturation = static_cast<uint8_t>(constrain(intVal, 0, 255));
    else if (key == "hueJitter") settings_.hueJitter = static_cast<uint8_t>(constrain(intVal, 0, 100));
    else return false;

    if (settings_.rateMax < settings_.rateMin) {
      settings_.rateMax = settings_.rateMin;
    }
    return true;
  }

  void resetSettings() override {
    settings_ = Settings();
    randomizeRates();
  }

  bool savePreset(uint8_t slot) override {
    const PresetData preset = {
        settings_.breathMin,
        settings_.breathMax,
        settings_.rateMin,
        settings_.rateMax,
        settings_.saturation,
        settings_.hueJitter,
    };
    return saveEffectPresetBytes(56, slot, reinterpret_cast<const uint8_t *>(&preset), sizeof(preset));
  }

  bool loadPreset(uint8_t slot) override {
    PresetData preset = {};
    if (!loadEffectPresetBytes(56, slot, reinterpret_cast<uint8_t *>(&preset), sizeof(preset))) {
      return false;
    }

    settings_.breathMin = preset.breathMin;
    settings_.breathMax = preset.breathMax;
    settings_.rateMin = preset.rateMin;
    settings_.rateMax = std::max(preset.rateMin, preset.rateMax);
    settings_.saturation = preset.saturation;
    settings_.hueJitter = preset.hueJitter;
    randomizeRates();
    return true;
  }

 private:
  struct PresetData {
    uint8_t breathMin;
    uint8_t breathMax;
    uint8_t rateMin;
    uint8_t rateMax;
    uint8_t saturation;
    uint8_t hueJitter;
  };

  Settings settings_;
  std::vector<uint8_t> baseHue_;
  std::vector<float> phase_;
  std::vector<float> rateHz_;

  static float clampf(float value, float low, float high) {
    return std::max(low, std::min(high, value));
  }

  float randomRateHz() const {
    const float minRate = static_cast<float>(settings_.rateMin) / 100.0f;
    const float maxRate = static_cast<float>(settings_.rateMax) / 100.0f;
    const float t = static_cast<float>(random(0, 10001)) / 10000.0f;
    return minRate + ((maxRate - minRate) * t);
  }

  void allocate(uint16_t count) {
    baseHue_.assign(count, 0);
    phase_.assign(count, 0.0f);
    rateHz_.assign(count, 0.2f);

    for (uint16_t i = 0; i < count; ++i) {
      baseHue_[i] = random8();
      phase_[i] = static_cast<float>(random(0, 6284)) / 1000.0f;
      rateHz_[i] = randomRateHz();
    }
  }

  void randomizeRates() {
    for (float &v : rateHz_) {
      v = randomRateHz();
    }
  }
};

REGISTER_EFFECT(PixieLightEffect, 56, "PixieLight");

}  // namespace
