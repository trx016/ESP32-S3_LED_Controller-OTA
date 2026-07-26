#include "EffectRegistry.h"

class SolidEffect : public IEffect {
 public:
  void render(const EffectContext &ctx, CRGB *leds, uint16_t count) override {
    fill_solid(leds, count, CRGB(ctx.state.red, ctx.state.green, ctx.state.blue));
  }
};

REGISTER_EFFECT(SolidEffect, 0, "Solid");
