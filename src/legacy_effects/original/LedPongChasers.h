#ifndef LED_PONG_CHASERS_H
#define LED_PONG_CHASERS_H

#include <FastLED.h>

#define NUM_CHASERS 10        // Number of bouncing dots
#define FADE_AMOUNT 40       // Trail fade (0 = no fade, 255 = instant fade)
#define RIPPLE_SIZE 3        // How far the ripple shockwave spreads on impact
#define RIPPLE_BRIGHTNESS 100  // Brightness of the ripple

struct Chaser {
  float position;
  float velocity;
  CRGB color;
};

Chaser chasers[NUM_CHASERS];

void initPongChasers(int numLeds) {
  for (int i = 0; i < NUM_CHASERS; i++) {
    chasers[i].position = random(numLeds);
    chasers[i].velocity = 0.5 + (random(10) / 10.0);  // Speed between 0.5 and 1.4
    if (random(2) == 0) chasers[i].velocity *= -1;    // Random direction
    chasers[i].color = CHSV(random(255), 255, 255);   // Random color
  }
}

void spawnRipple(CRGB* leds, int center, int numLeds) {
  for (int d = 1; d <= RIPPLE_SIZE; d++) {
    uint8_t fade = map(d, 1, RIPPLE_SIZE, RIPPLE_BRIGHTNESS, 10);  // Decrease brightness as ripple expands
    if (center - d >= 0) leds[center - d] += CHSV(0, 0, fade);
    if (center + d < numLeds) leds[center + d] += CHSV(0, 0, fade);
  }
}

void updatePongChasers(CRGB* leds, int numLeds) {
  // Fade trail
  for (int i = 0; i < numLeds; i++) {
    leds[i].fadeToBlackBy(FADE_AMOUNT);
  }

  // Move chasers and draw their color
  for (int i = 0; i < NUM_CHASERS; i++) {
    chasers[i].position += chasers[i].velocity;

    // Bounce off ends
    if (chasers[i].position <= 0) {
      chasers[i].position = 0;
      chasers[i].velocity *= -1;
    } else if (chasers[i].position >= numLeds - 1) {
      chasers[i].position = numLeds - 1;
      chasers[i].velocity *= -1;
    }
  }

  // Handle collisions after movement
  for (int i = 0; i < NUM_CHASERS; i++) {
    for (int j = i + 1; j < NUM_CHASERS; j++) {
      int posA = round(chasers[i].position);
      int posB = round(chasers[j].position);
      if (posA == posB) {
        // Swap and rewind
        float temp = chasers[i].velocity;
        chasers[i].velocity = -chasers[j].velocity;
        chasers[j].velocity = -temp;

        chasers[i].position += chasers[i].velocity;
        chasers[j].position += chasers[j].velocity;

        // Clamp to bounds
        chasers[i].position = constrain(chasers[i].position, 0, numLeds - 1);
        chasers[j].position = constrain(chasers[j].position, 0, numLeds - 1);

        // Shockwave effect
        spawnRipple(leds, posA, numLeds);
      }
    }
  }

  // Draw chasers
  for (int i = 0; i < NUM_CHASERS; i++) {
    int pos = round(chasers[i].position);
    leds[pos] += chasers[i].color;
  }
}


#endif
