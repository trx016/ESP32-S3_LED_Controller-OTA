#ifndef NEURO_TRIP_H
#define NEURO_TRIP_H

#include <FastLED.h>
#include "LEDConfig.h"

extern CRGB leds[];
extern uint8_t BRIGHTNESS;
extern uint16_t wall1Count;
extern uint16_t wall2Count;
extern uint16_t wall3Count;
extern uint16_t wall4Count;
extern uint8_t neuroTripSpeedPercent;
extern uint8_t neuroTripDepth;
extern uint8_t neuroTripPulse;

struct {
  uint32_t lastUpdate = 0;
  uint16_t phase = 0;
} neuroTripState;

inline void neuroTripComputeWallLengths(uint16_t lengths[4]) {
  uint16_t weights[4] = {
    (uint16_t)max(1, (int)wall1Count),
    (uint16_t)max(1, (int)wall2Count),
    (uint16_t)max(1, (int)wall3Count),
    (uint16_t)max(1, (int)wall4Count)
  };

  uint32_t sum = (uint32_t)weights[0] + weights[1] + weights[2] + weights[3];
  uint16_t used = 0;
  for (uint8_t i = 0; i < 4; i++) {
    lengths[i] = (uint16_t)((weights[i] * (uint32_t)NUM_LEDS) / sum);
    if (lengths[i] == 0) {
      lengths[i] = 1;
    }
    used += lengths[i];
  }

  while (used < NUM_LEDS) {
    uint8_t best = 0;
    for (uint8_t i = 1; i < 4; i++) {
      if (weights[i] > weights[best]) {
        best = i;
      }
    }
    lengths[best]++;
    used++;
  }

  while (used > NUM_LEDS) {
    uint8_t best = 0;
    for (uint8_t i = 1; i < 4; i++) {
      if (lengths[i] > lengths[best]) {
        best = i;
      }
    }
    if (lengths[best] > 1) {
      lengths[best]--;
      used--;
    } else {
      break;
    }
  }
}

inline void initNeuroTrip() {
  neuroTripState.lastUpdate = 0;
  neuroTripState.phase = 0;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

inline void updateNeuroTrip(unsigned long currentTime) {
  uint32_t interval = (24UL * 100) / max<uint8_t>(30, neuroTripSpeedPercent);
  if (currentTime - neuroTripState.lastUpdate < interval) {
    return;
  }
  neuroTripState.lastUpdate = currentTime;

  neuroTripState.phase += max<uint8_t>(1, neuroTripSpeedPercent / 24);

  uint16_t wallLengths[4];
  neuroTripComputeWallLengths(wallLengths);

  uint16_t cursor = 0;
  for (uint8_t wall = 0; wall < 4; wall++) {
    uint16_t len = wallLengths[wall];
    if (len == 0) {
      continue;
    }

    for (uint16_t i = 0; i < len; i++) {
      uint16_t idx = (cursor + i) % NUM_LEDS;
      uint8_t local = (uint8_t)((i * 255UL) / max<uint16_t>(1, len - 1));

      uint8_t wave1 = sin8((uint8_t)(local * 3) + (uint8_t)neuroTripState.phase + wall * 37);
      uint8_t wave2 = sin8((uint8_t)((255 - local) * 5) - (uint8_t)(neuroTripState.phase * 2) + wall * 59);
      uint8_t wave3 = sin8((uint8_t)(local * 7) + (wave1 >> 1) + (uint8_t)(neuroTripState.phase / 2));

      uint8_t interference = (uint8_t)(((uint16_t)wave1 * 3 + (uint16_t)wave2 * 2 + wave3) / 6);
      uint8_t hue = (uint8_t)((neuroTripState.phase >> 1) + wall * 48 + (interference >> 1));
      uint8_t sat = (uint8_t)min(255, 180 + (wave2 >> 2));

      uint8_t baseVal = qadd8(24, scale8(interference, neuroTripDepth));
      uint8_t pulseVal = scale8(sin8((uint8_t)(neuroTripState.phase * 2) + wall * 40), neuroTripPulse);
      uint8_t val = qadd8(scale8(baseVal, 180), pulseVal);
      val = scale8_video(val, BRIGHTNESS);

      leds[idx] = CHSV(hue, sat, val);
    }

    // Highlight boundaries between flat wall areas for stronger geometry perception.
    for (uint8_t e = 0; e < 3; e++) {
      uint16_t edgeIdx = (cursor + e) % NUM_LEDS;
      leds[edgeIdx] += CHSV((uint8_t)(neuroTripState.phase + wall * 64), 90, scale8_video((uint8_t)(140 - e * 36), BRIGHTNESS));
    }

    cursor += len;
  }
}

#endif
