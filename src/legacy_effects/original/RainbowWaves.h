#ifndef RAINBOW_WAVES_H
#define RAINBOW_WAVES_H

#include <FastLED.h>

struct {
  uint16_t leftPosX16 = 0;
  uint16_t rightPosX16 = 0;
  int16_t leftVelocityX16 = 16;
  int16_t rightVelocityX16 = -16;
  uint16_t collisionDeltaV = 32;
  uint16_t explosionCenter = 0;
  uint8_t leftHue = 0;
  uint8_t rightHue = 128;
  uint8_t burstHue = 64;
  uint8_t burstFrame = 0;
  uint8_t burstRadius = 0;
  bool explosionActive = false;
} rainbowWavesState;

extern uint8_t rainbowWavesSpeedPercent;
extern uint8_t particleMayhemSpeedBoost;
extern uint8_t particleMayhemParticleCount;
extern uint8_t particleMayhemDensity;
extern uint8_t particleMayhemExplosionSize;
extern uint8_t particleMayhemExplosionSpeed;
extern uint8_t particleMayhemRandomSpeed;
extern uint8_t BRIGHTNESS;

static inline uint16_t particleMayhemWrapIndex(int32_t index) {
  while (index < 0) {
    index += NUM_LEDS;
  }
  while (index >= NUM_LEDS) {
    index -= NUM_LEDS;
  }
  return (uint16_t)index;
}

static inline uint16_t particleMayhemWrapX16(int32_t position) {
  int32_t span = (int32_t)NUM_LEDS * 16;
  while (position < 0) {
    position += span;
  }
  while (position >= span) {
    position -= span;
  }
  return (uint16_t)position;
}

static inline uint16_t particleMayhemDistance(uint16_t a, uint16_t b) {
  uint16_t delta = a > b ? a - b : b - a;
  uint16_t wrapDelta = NUM_LEDS - delta;
  return delta < wrapDelta ? delta : wrapDelta;
}

static inline void particleMayhemAddGlow(int32_t center, uint8_t hue, uint8_t value, uint8_t radius) {
  static const uint8_t falloff[] = {255, 190, 145, 105, 72, 48};
  if (radius > 5) {
    radius = 5;
  }
  for (int8_t offset = -((int8_t)radius); offset <= (int8_t)radius; offset++) {
    uint8_t strength = falloff[abs(offset)];
    uint16_t ledIndex = particleMayhemWrapIndex(center + offset);
    leds[ledIndex] += CHSV(hue + (offset * 5), 220, scale8(value, strength));
  }
}

static inline void particleMayhemRespawn() {
  uint16_t halfSpan = NUM_LEDS / 2;
  uint16_t scatter = max<uint16_t>(halfSpan / 3, 12);
  uint8_t minSpeed = (uint8_t)(10 + particleMayhemDensity + particleMayhemSpeedBoost / 20);
  uint8_t spread = (uint8_t)(8 + particleMayhemRandomSpeed / 10 + particleMayhemSpeedBoost / 25);

  rainbowWavesState.leftPosX16 = (uint16_t)(random16(scatter) * 16U);
  rainbowWavesState.rightPosX16 = particleMayhemWrapX16((int32_t)(halfSpan + random16(scatter)) * 16);
  rainbowWavesState.leftVelocityX16 = (int16_t)(minSpeed + random8(spread + 1));
  rainbowWavesState.rightVelocityX16 = -(int16_t)(minSpeed + random8(spread + 1));
  rainbowWavesState.collisionDeltaV = (uint16_t)abs(rainbowWavesState.leftVelocityX16 - rainbowWavesState.rightVelocityX16);
  rainbowWavesState.leftHue = random8();
  rainbowWavesState.rightHue = rainbowWavesState.leftHue + random8(80, 160);
  rainbowWavesState.burstHue = rainbowWavesState.leftHue;
  rainbowWavesState.burstFrame = 0;
  rainbowWavesState.burstRadius = 0;
  rainbowWavesState.explosionActive = false;
}

void updateRainbowWaves(unsigned long currentTime) {
  static uint32_t lastUpdate = 0;

  uint16_t effectiveSpeed = (uint16_t)rainbowWavesSpeedPercent + (uint16_t)particleMayhemSpeedBoost;
  if (effectiveSpeed > 320) {
    effectiveSpeed = 320;
  }
  uint32_t updateInterval = (24UL * 100) / effectiveSpeed;
  if (currentTime - lastUpdate < updateInterval) {
    return;
  }
  lastUpdate = currentTime;

  uint8_t fadeAmount = (uint8_t)max(26, 78 - (int)particleMayhemDensity * 10);
  uint8_t glowRadius = (uint8_t)min(5, 2 + particleMayhemDensity);
  uint8_t preSparks = (uint8_t)(2 + particleMayhemDensity * 2 + particleMayhemParticleCount * 2);
  fadeToBlackBy(leds, NUM_LEDS, fadeAmount);

  if (!rainbowWavesState.explosionActive) {
    if (particleMayhemRandomSpeed > 0) {
      int8_t jitterRange = (int8_t)(1 + particleMayhemRandomSpeed / 25);  // 1..5
      int8_t leftJitter = (int8_t)random8((uint8_t)(jitterRange * 2 + 1)) - jitterRange;
      int8_t rightJitter = (int8_t)random8((uint8_t)(jitterRange * 2 + 1)) - jitterRange;
      if (random8() < particleMayhemRandomSpeed) {
        rainbowWavesState.leftVelocityX16 += leftJitter;
      }
      if (random8() < particleMayhemRandomSpeed) {
        rainbowWavesState.rightVelocityX16 += rightJitter;
      }

      int16_t minSpeed = (int16_t)(8 + particleMayhemDensity + particleMayhemSpeedBoost / 20);
      int16_t maxSpeed = (int16_t)(36 + particleMayhemDensity * 2 + particleMayhemRandomSpeed / 5 + particleMayhemSpeedBoost / 4);
      if (rainbowWavesState.leftVelocityX16 < minSpeed) {
        rainbowWavesState.leftVelocityX16 = minSpeed;
      }
      if (rainbowWavesState.leftVelocityX16 > maxSpeed) {
        rainbowWavesState.leftVelocityX16 = maxSpeed;
      }
      if (rainbowWavesState.rightVelocityX16 > -minSpeed) {
        rainbowWavesState.rightVelocityX16 = -minSpeed;
      }
      if (rainbowWavesState.rightVelocityX16 < -maxSpeed) {
        rainbowWavesState.rightVelocityX16 = -maxSpeed;
      }
    }

    rainbowWavesState.leftPosX16 = particleMayhemWrapX16((int32_t)rainbowWavesState.leftPosX16 + rainbowWavesState.leftVelocityX16);
    rainbowWavesState.rightPosX16 = particleMayhemWrapX16((int32_t)rainbowWavesState.rightPosX16 + rainbowWavesState.rightVelocityX16);

    uint16_t leftIndex = rainbowWavesState.leftPosX16 / 16;
    uint16_t rightIndex = rainbowWavesState.rightPosX16 / 16;

    particleMayhemAddGlow(leftIndex, rainbowWavesState.leftHue, scale8_video(255, BRIGHTNESS), glowRadius);
    particleMayhemAddGlow(rightIndex, rainbowWavesState.rightHue, scale8_video(255, BRIGHTNESS), glowRadius);

    for (uint8_t spark = 0; spark < preSparks; spark++) {
      uint16_t sparkIndex = particleMayhemWrapIndex((int32_t)leftIndex + random8(9) - 4);
      leds[sparkIndex] += CHSV(rainbowWavesState.leftHue + random8(40), 180, scale8_video(110, BRIGHTNESS));
      sparkIndex = particleMayhemWrapIndex((int32_t)rightIndex + random8(9) - 4);
      leds[sparkIndex] += CHSV(rainbowWavesState.rightHue + random8(40), 180, scale8_video(110, BRIGHTNESS));
    }

    if (particleMayhemDistance(leftIndex, rightIndex) <= (uint8_t)(2 + particleMayhemDensity / 2)) {
      rainbowWavesState.explosionActive = true;
      rainbowWavesState.collisionDeltaV = (uint16_t)abs(rainbowWavesState.leftVelocityX16 - rainbowWavesState.rightVelocityX16);
      rainbowWavesState.explosionCenter = particleMayhemWrapIndex(leftIndex);
      rainbowWavesState.burstHue = (uint8_t)((rainbowWavesState.leftHue / 2) + (rainbowWavesState.rightHue / 2));
      rainbowWavesState.burstFrame = 0;
      rainbowWavesState.burstRadius = 0;
      leds[rainbowWavesState.explosionCenter] += CHSV(0, 0, scale8_video(255, BRIGHTNESS));
    }
  } else {
    uint8_t deltaBoost = (uint8_t)min(24, rainbowWavesState.collisionDeltaV / 3);
    uint8_t expansionStep = max<uint8_t>(1, (effectiveSpeed / 45) + (particleMayhemExplosionSize / 4) + (particleMayhemExplosionSpeed / 3) + (deltaBoost / 7));
    uint8_t baseValue = scale8_video((uint8_t)min(255, 175 + deltaBoost * 3), BRIGHTNESS);
    uint8_t shellCount = (uint8_t)min(12, 3 + particleMayhemExplosionSize / 2 + particleMayhemDensity / 2 + particleMayhemParticleCount / 2 + deltaBoost / 5);
    uint8_t burstFramesMax = (uint8_t)max(8, 18 + particleMayhemExplosionSize * 2 + particleMayhemDensity + deltaBoost / 2 - particleMayhemExplosionSpeed);
    uint8_t explosionSparks = (uint8_t)(5 + particleMayhemDensity * 3 + particleMayhemParticleCount * 3 + deltaBoost / 2);

    rainbowWavesState.burstFrame++;
    rainbowWavesState.burstRadius += expansionStep;

    leds[rainbowWavesState.explosionCenter] += CHSV(0, 0, scale8_video(baseValue, 200));

    for (uint8_t shell = 0; shell < shellCount; shell++) {
      uint8_t radius = rainbowWavesState.burstRadius + (shell * 2);
      uint8_t shellValue = qsub8(baseValue, shell * 22 + rainbowWavesState.burstFrame * 6);
      if (shellValue == 0) {
        continue;
      }

      uint16_t leftShell = particleMayhemWrapIndex((int32_t)rainbowWavesState.explosionCenter - radius);
      uint16_t rightShell = particleMayhemWrapIndex((int32_t)rainbowWavesState.explosionCenter + radius);
      leds[leftShell] += CHSV(rainbowWavesState.burstHue + shell * 18, 240, shellValue);
      leds[rightShell] += CHSV(rainbowWavesState.burstHue + 96 + shell * 18, 240, shellValue);
    }

    for (uint8_t spark = 0; spark < explosionSparks; spark++) {
      int16_t offset = (int16_t)random8((rainbowWavesState.burstRadius * 2) + 1) - rainbowWavesState.burstRadius;
      uint16_t sparkIndex = particleMayhemWrapIndex((int32_t)rainbowWavesState.explosionCenter + offset);
      leds[sparkIndex] += CHSV(rainbowWavesState.burstHue + random8(96), 200, qsub8(baseValue, random8(96)));
    }

    if (rainbowWavesState.burstFrame >= burstFramesMax) {
      particleMayhemRespawn();
    }
  }

  FastLED.show();
}

void initRainbowWaves() {
  particleMayhemRespawn();
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

#endif
