// LEDUtils.h
#ifndef LED_UTILS_H
#define LED_UTILS_H

#include <FastLED.h>
#include "LEDConfig.h"

extern CRGB leds[NUM_LEDS];

inline void clearLeds() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

#endif
