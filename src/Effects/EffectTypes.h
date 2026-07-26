#pragma once

#include <Arduino.h>

struct LedEffectState {
  bool powerOn;
  uint8_t pattern;
  uint8_t brightness;
  uint8_t speed;
  uint16_t fps;
  bool dither;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

struct EffectContext {
  LedEffectState state;
  uint32_t nowMs;
  uint16_t phase;
  uint32_t frame;
};
