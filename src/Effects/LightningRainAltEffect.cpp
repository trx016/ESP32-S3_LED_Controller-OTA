#include "EffectRegistry.h"

#ifndef LED_STRIP_LENGTH
#define LED_STRIP_LENGTH 1600
#endif

class LightningRainAltEffect : public IEffect {
 public:
  void begin(CRGB *leds, uint16_t count) override {
    (void)leds;
    m_count = count;
    if (m_count > LED_STRIP_LENGTH) {
      m_count = LED_STRIP_LENGTH;
    }
    for (uint8_t i = 0; i < kDropCount; ++i) {
      m_drops[i] = -1;
    }
  }

  void render(const EffectContext &ctx, CRGB *leds, uint16_t count) override {
    const uint16_t activeCount = (count < m_count) ? count : m_count;
    if (activeCount == 0) {
      return;
    }

    fadeToBlackBy(leds, activeCount, map(ctx.state.speed, 1, 255, 16, 72));

    const uint8_t spawnChance = map(ctx.state.speed, 1, 255, 5, 40);
    for (uint8_t i = 0; i < kDropCount; ++i) {
      if (m_drops[i] < 0 && random8() < spawnChance) {
        m_drops[i] = random16(activeCount);
      }

      if (m_drops[i] >= 0) {
        const int pos = m_drops[i];
        if (pos >= 0 && pos < activeCount) {
          leds[pos] += CRGB(0, 0, 180);
          if (pos > 0) {
            leds[pos - 1] += CRGB(0, 0, 60);
          }
        }

        m_drops[i] += map(ctx.state.speed, 1, 255, 1, 6);
        if (m_drops[i] >= activeCount) {
          m_drops[i] = -1;
        }
      }
    }

    const uint8_t strikeCount = map(ctx.state.speed, 1, 255, 1, 5);
    for (uint8_t n = 0; n < strikeCount; ++n) {
      const int pos = random16(activeCount);
      const uint8_t span = random8(2, 14);
      for (uint8_t d = 0; d < span; ++d) {
        const int idx = pos + d;
        if (idx >= 0 && idx < activeCount) {
          leds[idx] += CRGB(180, 180, 255);
        }
      }
    }
  }

 private:
  static constexpr uint8_t kDropCount = 40;
  int m_drops[kDropCount] = {0};
  uint16_t m_count = 0;
};

REGISTER_EFFECT(LightningRainAltEffect, 5, "Lightning Alt + Rain");
