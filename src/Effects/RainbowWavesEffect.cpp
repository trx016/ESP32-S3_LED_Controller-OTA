#include "EffectRegistry.h"

class RainbowWavesEffect : public IEffect {
 public:
  void render(const EffectContext &ctx, CRGB *leds, uint16_t count) override {
    const uint8_t scale = map(ctx.state.speed, 1, 255, 2, 16);
    for (uint16_t i = 0; i < count; ++i) {
      const uint8_t hue = static_cast<uint8_t>((i / scale + ctx.phase) & 0xFF);
      leds[i] = CHSV(hue, 220, 255);
    }
  }
};

REGISTER_EFFECT(RainbowWavesEffect, 1, "Rainbow Waves");
