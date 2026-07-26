#include "EffectRegistry.h"

class NeuroTripEffect : public IEffect {
 public:
  void render(const EffectContext &ctx, CRGB *leds, uint16_t count) override {
    const uint8_t waveScale = map(ctx.state.speed, 1, 255, 4, 18);
    for (uint16_t i = 0; i < count; ++i) {
      const uint8_t wave = sin8(static_cast<uint8_t>(i * waveScale + ctx.phase));
      const uint8_t hue = static_cast<uint8_t>((wave + ctx.phase) & 0xFF);
      leds[i] = CHSV(hue, 200, wave);
    }
  }
};

REGISTER_EFFECT(NeuroTripEffect, 3, "Neuro Trip");
