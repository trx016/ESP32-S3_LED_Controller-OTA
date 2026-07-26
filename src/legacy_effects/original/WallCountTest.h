#ifndef WALL_COUNT_TEST_H
#define WALL_COUNT_TEST_H

#include <FastLED.h>
#include "LEDConfig.h"

const uint16_t WALL_LED_COUNT = NUM_LEDS / 4;

uint16_t wall1Count = 218;
uint16_t wall2Count = 196;
uint16_t wall3Count = 198;
uint16_t wall4Count = 176;

inline uint16_t clampWallCount(int value) {
  return (uint16_t)constrain(value, 0, WALL_LED_COUNT);
}

inline void setWallCountValues(int wall1, int wall2, int wall3, int wall4) {
  wall1Count = clampWallCount(wall1);
  wall2Count = clampWallCount(wall2);
  wall3Count = clampWallCount(wall3);
  wall4Count = clampWallCount(wall4);
}

inline void updateWallCountTest(CRGB leds[]) {
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  const CRGB wallColors[4] = { CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Orange };
  const uint16_t counts[4] = { wall1Count, wall2Count, wall3Count, wall4Count };

  uint16_t cursor = 0;
  for (uint8_t wall = 0; wall < 4; wall++) {
    uint16_t lit = min((uint16_t)WALL_LED_COUNT, counts[wall]);
    for (uint16_t i = 0; i < lit; i++) {
      uint16_t idx = cursor + i;
      if (idx < NUM_LEDS) {
        leds[idx] = wallColors[wall];
      }
    }
    cursor += lit;
    if (cursor >= NUM_LEDS) {
      break;
    }
  }
}

#endif
