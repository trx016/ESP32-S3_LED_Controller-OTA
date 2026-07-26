#include <stdint.h>
void updateRainEffect(unsigned long currentTime) {
  static unsigned long lastUpdate = 0;
  extern uint8_t rainDropChance; // % chance per frame to spawn new drop
  extern uint8_t rainFadeAmount; // how fast drops fade
  extern uint16_t rainUpdateDelay;
  const CRGB rainDropColor = CRGB::Blue; // color of the raindrop

  if (currentTime - lastUpdate > rainUpdateDelay) { // update every 50ms
    lastUpdate = currentTime;

    // Fade all LEDs a bit
    fadeToBlackBy(leds, NUM_LEDS, rainFadeAmount);

    // Randomly create new "raindrops"
    for (int i = 0; i < NUM_LEDS; i++) {
      if (random8() < rainDropChance) {
        leds[i] += rainDropColor; // add drop color (not overwrite)
      }
    }
  }
}
