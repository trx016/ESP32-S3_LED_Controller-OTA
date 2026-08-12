#include "EffectRegistry.h"
#include "EffectPresetStore.h"

#include <FastLED.h>

#include <algorithm>

namespace {

struct Settings {
  uint8_t cycleSeconds = 10;
  uint16_t transitionMs = 1600;
  uint8_t saturation = 230;
  uint8_t value = 255;
};

class NeoPrismEffect : public IEffect {
 public:
  void begin(CRGB *leds, uint16_t count) override {
    (void)leds;
    (void)count;
    activeHue_ = random8();
    targetHue_ = random8();
    while (targetHue_ == activeHue_) {
      targetHue_ = random8();
    }
    phaseMs_ = 0;
    transitioning_ = false;
  }

  void render(const EffectContext &ctx, CRGB *leds, uint16_t count) override {
    if (count == 0) {
      return;
    }

    const int32_t dtMs = std::max<int32_t>(1, ctx.deltaMs > 0 ? static_cast<int32_t>(ctx.deltaMs) : 33);
    const float speedScale = clampf(static_cast<float>(ctx.state.speed) / 128.0f, 0.2f, 3.0f);
    phaseMs_ += static_cast<int32_t>(std::max(1.0f, static_cast<float>(dtMs) * speedScale));

    const int32_t holdMs = std::max<int32_t>(500, static_cast<int32_t>(settings_.cycleSeconds) * 1000);
    const int32_t blendMs = std::max<int32_t>(120, settings_.transitionMs);

    if (!transitioning_ && phaseMs_ >= holdMs) {
      transitioning_ = true;
      phaseMs_ = 0;
      targetHue_ = random8();
      while (targetHue_ == activeHue_) {
        targetHue_ = random8();
      }
    }

    if (transitioning_ && phaseMs_ >= blendMs) {
      activeHue_ = targetHue_;
      transitioning_ = false;
      phaseMs_ = 0;
    }

    const uint8_t hue = computeHue();
    const CRGB color = CHSV(hue, settings_.saturation, settings_.value);
    fill_solid(leds, count, color);
  }

  String settingsSchemaJson() const override {
    return "["
           "{\"key\":\"cycleSeconds\",\"label\":\"Color Hold sec\",\"type\":\"slider\",\"min\":1,\"max\":60,\"step\":1},"
           "{\"key\":\"transitionMs\",\"label\":\"Transition ms\",\"type\":\"slider\",\"min\":120,\"max\":5000,\"step\":10},"
           "{\"key\":\"saturation\",\"label\":\"Saturation\",\"type\":\"slider\",\"min\":0,\"max\":255,\"step\":1},"
           "{\"key\":\"value\",\"label\":\"Color Intensity\",\"type\":\"slider\",\"min\":1,\"max\":255,\"step\":1}"
           "]";
  }

  String settingsStateJson() const override {
    String out = "{";
    out += "\"cycleSeconds\":" + String(settings_.cycleSeconds);
    out += ",\"transitionMs\":" + String(settings_.transitionMs);
    out += ",\"saturation\":" + String(settings_.saturation);
    out += ",\"value\":" + String(settings_.value);
    out += "}";
    return out;
  }

  bool setSetting(const String &key, const String &value) override {
    const int intVal = value.toInt();
    if (key == "cycleSeconds") settings_.cycleSeconds = static_cast<uint8_t>(constrain(intVal, 1, 60));
    else if (key == "transitionMs") settings_.transitionMs = static_cast<uint16_t>(constrain(intVal, 120, 5000));
    else if (key == "saturation") settings_.saturation = static_cast<uint8_t>(constrain(intVal, 0, 255));
    else if (key == "value") settings_.value = static_cast<uint8_t>(constrain(intVal, 1, 255));
    else return false;
    return true;
  }

  void resetSettings() override {
    settings_ = Settings();
  }

  bool savePreset(uint8_t slot) override {
    const PresetData preset = {
        settings_.cycleSeconds,
        settings_.transitionMs,
        settings_.saturation,
        settings_.value,
    };
    return saveEffectPresetBytes(55, slot, reinterpret_cast<const uint8_t *>(&preset), sizeof(preset));
  }

  bool loadPreset(uint8_t slot) override {
    PresetData preset = {};
    if (!loadEffectPresetBytes(55, slot, reinterpret_cast<uint8_t *>(&preset), sizeof(preset))) {
      return false;
    }

    settings_.cycleSeconds = preset.cycleSeconds;
    settings_.transitionMs = preset.transitionMs;
    settings_.saturation = preset.saturation;
    settings_.value = preset.value;
    return true;
  }

 private:
  struct PresetData {
    uint8_t cycleSeconds;
    uint16_t transitionMs;
    uint8_t saturation;
    uint8_t value;
  };

  Settings settings_;
  uint8_t activeHue_ = 0;
  uint8_t targetHue_ = 0;
  int32_t phaseMs_ = 0;
  bool transitioning_ = false;

  static float clampf(float value, float low, float high) {
    return std::max(low, std::min(high, value));
  }

  uint8_t computeHue() const {
    if (!transitioning_) {
      return activeHue_;
    }

    const float blendMs = static_cast<float>(std::max<int32_t>(120, settings_.transitionMs));
    const float t = clampf(static_cast<float>(phaseMs_) / blendMs, 0.0f, 1.0f);
    const int16_t diff = static_cast<int16_t>(targetHue_) - static_cast<int16_t>(activeHue_);
    const float hue = static_cast<float>(activeHue_) + (static_cast<float>(diff) * t);
    return static_cast<uint8_t>(static_cast<int>(hue) & 0xFF);
  }
};

REGISTER_EFFECT(NeoPrismEffect, 55, "NeoPrism");

}  // namespace
