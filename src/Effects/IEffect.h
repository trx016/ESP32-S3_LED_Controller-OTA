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
  virtual void render(const EffectContext &ctx, CRGB *leds, uint16_t count) = 0;
};
