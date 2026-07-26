#include "EffectRegistry.h"

class KaleidoscopeEffect : public IEffect {
 public:
  void render(const EffectContext &ctx, CRGB *leds, uint16_t count) override {
    const uint8_t speedScale = map(ctx.state.speed, 1, 255, 1, 8);
    const uint16_t half = count / 2;

    for (uint16_t i = 0; i < half; ++i) {
      const uint8_t hue = static_cast<uint8_t>((i * speedScale + ctx.phase) & 0xFF);
      const CRGB c = CHSV(hue, 255, 255);
      leds[i] = c;
      leds[count - 1 - i] = c;
    }

    if ((count & 1U) == 1U) {
      leds[half] = CHSV(static_cast<uint8_t>(ctx.phase), 255, 255);
    }
  }
};

REGISTER_EFFECT(KaleidoscopeEffect, 2, "Kaleidoscope");
