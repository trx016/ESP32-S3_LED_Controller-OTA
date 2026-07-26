#include "EffectRegistry.h"

#include <string.h>

#ifndef LED_STRIP_LENGTH
#define LED_STRIP_LENGTH 1600
#endif

class FlameEffect : public IEffect {
 public:
  void begin(CRGB *leds, uint16_t count) override {
    (void)leds;
    m_count = count;
    if (m_count > LED_STRIP_LENGTH) {
      m_count = LED_STRIP_LENGTH;
    }
    memset(m_heat, 0, sizeof(m_heat));
  }

  void render(const EffectContext &ctx, CRGB *leds, uint16_t count) override {
    const uint16_t activeCount = (count < m_count) ? count : m_count;
    if (activeCount == 0) {
      return;
    }

    const uint8_t cooling = map(ctx.state.speed, 1, 255, 70, 20);

    for (uint16_t i = 0; i < activeCount; ++i) {
      const uint8_t drop = random8(0, static_cast<uint8_t>(((cooling * 10) / activeCount) + 2));
      m_heat[i] = qsub8(m_heat[i], drop);
    }

    for (int k = static_cast<int>(activeCount) - 1; k >= 2; --k) {
      m_heat[k] = static_cast<uint8_t>((m_heat[k - 1] + m_heat[k - 2] + m_heat[k - 2]) / 3);
    }

    if (random8() < 120) {
      const uint8_t y = random8(7);
      if (y < activeCount) {
        m_heat[y] = qadd8(m_heat[y], random8(160, 255));
      }
    }

    for (uint16_t j = 0; j < activeCount; ++j) {
      leds[j] = HeatColor(m_heat[j]);
    }
  }

 private:
  uint8_t m_heat[LED_STRIP_LENGTH] = {0};
  uint16_t m_count = 0;
};

REGISTER_EFFECT(FlameEffect, 4, "Flame");
