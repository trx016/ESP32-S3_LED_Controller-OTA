#include "EffectRegistry.h"

class PacManEffect : public IEffect {
 public:
  void begin(CRGB *leds, uint16_t count) override {
    (void)leds;
    m_count = count;
    m_head = 0;
  }

  void render(const EffectContext &ctx, CRGB *leds, uint16_t count) override {
    const uint16_t activeCount = (count < m_count) ? count : m_count;
    if (activeCount == 0) {
      return;
    }

    fill_solid(leds, activeCount, CRGB::Black);

    const uint8_t trailLen = map(ctx.state.speed, 1, 255, 6, 18);
    const uint8_t step = map(ctx.state.speed, 1, 255, 1, 7);
    m_head = static_cast<uint16_t>((m_head + step) % activeCount);

    for (uint8_t i = 0; i < trailLen; ++i) {
      const int idx = static_cast<int>(m_head) - i;
      const int wrapped = (idx >= 0) ? idx : (idx + activeCount);
      const uint8_t fade = static_cast<uint8_t>(255 - (i * (220 / trailLen)));
      leds[wrapped] = CRGB(fade, fade, 0);
    }

    const uint8_t pelletSpacing = 24;
    for (uint16_t i = 0; i < activeCount; i += pelletSpacing) {
      if (abs(static_cast<int>(i) - static_cast<int>(m_head)) > trailLen) {
        leds[i] += CRGB(20, 20, 20);
      }
    }
  }

 private:
  uint16_t m_count = 0;
  uint16_t m_head = 0;
};

REGISTER_EFFECT(PacManEffect, 6, "Pac-Man");
