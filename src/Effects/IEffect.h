#pragma once

#include <FastLED.h>

#include "EffectTypes.h"

class IEffect {
 public:
  virtual ~IEffect() = default;

  virtual void begin(CRGB *leds, uint16_t count) {
    (void)leds;
    (void)count;
  }

  // Called when this effect becomes the active pattern.
  virtual void onActivate(const LedEffectState &state) {
    (void)state;
  }

  // Called when this effect is no longer active.
  virtual void onDeactivate() {
  }

  virtual void render(const EffectContext &ctx, CRGB *leds, uint16_t count) = 0;

  // Returns effect-specific UI schema as JSON array.
  virtual String settingsSchemaJson() const {
    return "[]";
  }

  // Returns current effect-specific settings as a JSON object.
  virtual String settingsStateJson() const {
    return "{}";
  }

  // Applies a single setting by key/value pair.
  virtual bool setSetting(const String &key, const String &value) {
    (void)key;
    (void)value;
    return false;
  }

  // Resets effect-specific settings to defaults.
  virtual void resetSettings() {
  }

  // Preset slots are 1..5 for effect-specific settings.
  virtual bool savePreset(uint8_t slot) {
    (void)slot;
    return false;
  }

  virtual bool loadPreset(uint8_t slot) {
    (void)slot;
    return false;
  }
};
