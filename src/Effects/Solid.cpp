#include "EffectRegistry.h"
#include "EffectPresetStore.h"

#include <FastLED.h>

namespace {

class SolidEffect : public IEffect {
 public:
  void render(const EffectContext &ctx, CRGB *leds, uint16_t count) override {
    (void)ctx;
    fill_solid(leds, count, CRGB(red_, green_, blue_));
  }

  String settingsSchemaJson() const override {
    return "["
           "{\"key\":\"red\",\"label\":\"Red\",\"type\":\"slider\",\"min\":0,\"max\":255,\"step\":1},"
           "{\"key\":\"green\",\"label\":\"Green\",\"type\":\"slider\",\"min\":0,\"max\":255,\"step\":1},"
           "{\"key\":\"blue\",\"label\":\"Blue\",\"type\":\"slider\",\"min\":0,\"max\":255,\"step\":1}"
           "]";
  }

  String settingsStateJson() const override {
    String out = "{";
    out += "\"red\":" + String(red_);
    out += ",\"green\":" + String(green_);
    out += ",\"blue\":" + String(blue_);
    out += "}";
    return out;
  }

  bool setSetting(const String &key, const String &value) override {
    const int v = constrain(value.toInt(), 0, 255);
    if (key == "red") {
      red_ = static_cast<uint8_t>(v);
      return true;
    }
    if (key == "green") {
      green_ = static_cast<uint8_t>(v);
      return true;
    }
    if (key == "blue") {
      blue_ = static_cast<uint8_t>(v);
      return true;
    }
    return false;
  }

  void resetSettings() override {
    red_ = 255;
    green_ = 160;
    blue_ = 48;
  }

  bool savePreset(uint8_t slot) override {
    const PresetData preset = {red_, green_, blue_};
    return saveEffectPresetBytes(1, slot, reinterpret_cast<const uint8_t *>(&preset), sizeof(preset));
  }

  bool loadPreset(uint8_t slot) override {
    PresetData preset = {0, 0, 0};
    if (!loadEffectPresetBytes(1, slot, reinterpret_cast<uint8_t *>(&preset), sizeof(preset))) {
      return false;
    }

    red_ = preset.red;
    green_ = preset.green;
    blue_ = preset.blue;
    return true;
  }

 private:
  struct PresetData {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
  };

  uint8_t red_ = 255;
  uint8_t green_ = 160;
  uint8_t blue_ = 48;
};

REGISTER_EFFECT(SolidEffect, 1, "Solid");

}  // namespace
