#ifndef KALEIDOSCOPE_H
#define KALEIDOSCOPE_H

#include <FastLED.h>

// Kaleidoscope pattern state
struct {
  uint32_t lastUpdate = 0;
  uint16_t hueOffset = 0;
  uint8_t saturation = 255;
  uint8_t brightness = 200;
} kaleidoscopeState;

// Speed control (50-220%)
extern uint8_t kaleidoscopeSpeedPercent;
extern uint8_t BRIGHTNESS;

void updateKaleidoscope(unsigned long currentTime) {
  static uint32_t lastUpdate = 0;
  
  // Update interval based on speed control (default ~50ms at 100%)
  uint32_t updateInterval = (50UL * 100) / kaleidoscopeSpeedPercent;
  
  if (currentTime - lastUpdate < updateInterval) {
    return;
  }
  lastUpdate = currentTime;

  // Shift the hue offset for rotation
  kaleidoscopeState.hueOffset += 2;

  // 4-way symmetrical kaleidoscope pattern
  uint16_t segmentSize = NUM_LEDS / 4;

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // Calculate position within each segment (0 to segmentSize-1)
    uint16_t posInSegment = i % segmentSize;
    
    // Create gradient based on position in segment
    uint8_t hue = (uint8_t)(
      kaleidoscopeState.hueOffset + 
      ((posInSegment * 255UL) / segmentSize)
    );
    
    uint8_t saturation = 255;
    uint8_t brightness = kaleidoscopeState.brightness;
    
    // Apply global brightness scaling
    brightness = (brightness * BRIGHTNESS) / 255;
    
    leds[i] = CHSV(hue, saturation, brightness);
  }
  
  FastLED.show();
}

void initKaleidoscope() {
  kaleidoscopeState.hueOffset = 0;
  kaleidoscopeState.saturation = 255;
  kaleidoscopeState.brightness = 200;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

#endif
