#ifndef LIGHTNING_STRIKE_ALT_H
#define LIGHTNING_STRIKE_ALT_H

#include <FastLED.h>
#include "LEDConfig.h"
#include "WallCountTest.h"

extern CRGB leds[NUM_LEDS];
extern uint8_t BRIGHTNESS;

// ============================================================
// LIGHTNING ALT - SIMPLE-LIKE STRIKE WITH SPEED CONTROL
// ============================================================

// Lightning timing parameters
uint32_t lightningAltMinInterval = 5000;        // Minimum time between strikes (ms)
uint32_t lightningAltMaxInterval = 30000;       // Maximum time between strikes (ms)
uint8_t lightningAltFrequentChance = 70;        // % chance of frequent cluster
uint8_t lightningAltSpeedPercent = 100;         // Global speed multiplier (50-220%)

// Lightning appearance parameters
uint8_t lightningAltFlashBrightness = 185;      // Peak brightness of main channel
uint16_t lightningAltMinFlashDuration = 25;     // Min strike duration (ms)
uint16_t lightningAltMaxFlashDuration = 140;    // Max strike duration (ms)
uint8_t lightningAltMinStrikeLength = 12;       // Min LEDs in one strike segment
uint8_t lightningAltMaxStrikeLength = 90;       // Max LEDs in one strike segment
uint8_t lightningAltMinStrikeWidth = 1;         // Min strike thickness (LEDs around channel)
uint8_t lightningAltMaxStrikeWidth = 4;         // Max strike thickness (LEDs around channel)

// Lightning glow and fade
uint8_t lightningAltGlowSize = 8;               // Size of glow around strike
uint8_t lightningAltGlowBrightness = 120;       // Brightness of glow
uint16_t lightningAltFadeDuration = 130;        // How long fade takes (ms)
bool lightningAltTestMode = false;              // Test mode: fixed-area strike loop
uint16_t lightningAltTestStartLed = 50;         // Fixed test strike start position
uint16_t lightningAltTestInterval = 1000;       // Test strike interval (ms)
bool lightningAltStrikePinEnabled = false;      // Enable lead-in pin that races into strike
uint8_t lightningAltStrikePinChance = 60;       // Chance (%) a strike uses strike-pin when enabled
uint8_t lightningAltPinStepDelay = 1;           // Delay between pin movements (ms)
uint8_t lightningAltPinStepSize = 3;            // LEDs advanced per pin step
uint8_t lightningAltPinSpeedMultiplier = 4;     // Multiplier for strike-pin travel speed (4-50)
uint8_t lightningAltPinStartBrightness = 28;    // Initial pin brightness
uint8_t lightningAltPinBrightnessStep = 40;     // Pin brightness growth per step

const CRGB lightningAltSimpleWhites[] = {
  CRGB(255, 255, 255),
  CRGB(250, 250, 255),
  CRGB(248, 250, 255),
  CRGB(245, 248, 255),
  CRGB(238, 242, 255),
};
const uint8_t lightningAltSimpleWhitesCount = sizeof(lightningAltSimpleWhites) / sizeof(lightningAltSimpleWhites[0]);

inline uint8_t altComfortScale(uint8_t userLevel) {
  // Nonlinear response for dark-room comfort: low/mid values are much softer
  // while still allowing full output at the top end.
  uint8_t curved = scale8_video(userLevel, userLevel);
  return scale8_video(curved, 230);
}

// Lightning state machine
enum LightningAltPhase {
  LIGHTNING_ALT_IDLE,
  LIGHTNING_ALT_PRE_FLASH,
  LIGHTNING_ALT_STRIKE_PIN,
  LIGHTNING_ALT_FLASH,
  LIGHTNING_ALT_FLICKER,
  LIGHTNING_ALT_RIPPLE,
  LIGHTNING_ALT_FADE,
  LIGHTNING_ALT_DONE
};

struct LightningAltState {
  LightningAltPhase phase = LIGHTNING_ALT_IDLE;
  unsigned long phaseStart = 0;
  unsigned long strikeStart = 0;
  unsigned long lastStrike = 0;
  unsigned long nextStrikeTime = 0;

  int mainLed1, mainLed2;        // Logical strike endpoints (wrapped ring)
  int mainStrikeLength;          // Length of main strike segment
  int mainStrikeWidth;           // Thickness of strike around main channel
  uint16_t currentStrikeDuration; // Active randomized strike duration
  uint8_t flickerCount;
  uint8_t flickerStep;
  uint8_t rippleStep;
  uint8_t rippleRange;
  uint8_t rippleSpeed;
  uint8_t fadeBrightness;
  bool isPowerStrike;

  int pinPosition;
  int pinPrevPosition;
  int pinTarget;
  int pinDirection;
  uint8_t pinBrightness;

  uint16_t currentFadeDuration;
  uint16_t currentMaxLifecycle;
  unsigned long lastFadeUpdate;
  CRGB strikeColor;
};

LightningAltState lightningAlt;

// Force an immediate lightning strike (for settings preview)
inline void forceImmediateLightningAltStrike() {
  lightningAlt.phase = LIGHTNING_ALT_IDLE;
  lightningAlt.lastStrike = 0;  // Reset timing to trigger next update
  lightningAlt.nextStrikeTime = 0;
}

// ============================================================
// RELAXING RAIN - GENTLE AND SOOTHING
// ============================================================

// Rain color modes
enum RainColorMode {
  RAIN_MULTI_TONE,    // Blue palette with variation
  RAIN_STATIC_BLUE,   // Solid blue
  RAIN_WHITE_BLUE     // White to blue gradient
};

uint8_t rainAltColorMode = RAIN_MULTI_TONE;    // Current rain color mode

// Rain appearance (relaxing feel)
uint8_t rainAltSpawnChance = 2;                 // Spawn threshold (0-255)
uint8_t rainAltFadeAmount = 8;                  // Fade amount (0-120)
uint16_t rainAltUpdateDelay = 60;               // Slower update for calming effect (ms)
uint8_t rainAltBrightness = 200;                // Brightness of raindrops (0-255)

// Rain color variations (multi-tone for depth)
uint8_t rainAltLightBlue = 80;                  // Light blue component
uint8_t rainAltMediumBlue = 150;                // Medium blue for variation
uint8_t rainAltDeepBlue = 220;                  // Deep blue for realism

// Rain behavior
uint8_t rainAltMultiToneChance = 70;            // % chance to use varied blue tones
uint8_t rainAltTrailChance = 30;                // % chance drops leave faint trails
uint8_t rainAltTrailFade = 15;                  // How fast trails fade
uint16_t rainAltPeakDuration = 300;             // How long drops stay bright (ms)

struct RainAltDrop {
  int position;
  unsigned long birthTime;
  uint8_t intensity;
  bool isTrail;
};

#define MAX_RAIN_DROPS_ALT 220
RainAltDrop rainAltDrops[MAX_RAIN_DROPS_ALT];

inline bool altIsInStrikeSuppressionZone(uint16_t pos, uint16_t activeLeds);

inline void spawnAltRainDrop(uint16_t activeLeds, unsigned long currentTime) {
  for (int i = 0; i < MAX_RAIN_DROPS_ALT; i++) {
    if (rainAltDrops[i].position < 0) {
      uint16_t newPos = (uint16_t)random(activeLeds);
      if (altIsInStrikeSuppressionZone(newPos, activeLeds)) {
        continue;
      }
      rainAltDrops[i].position = newPos;
      rainAltDrops[i].birthTime = currentTime;
      rainAltDrops[i].intensity = rainAltBrightness;
      rainAltDrops[i].isTrail = (random8() < rainAltTrailChance);
      return;
    }
  }
}

inline uint16_t altActivePerimeterLeds() {
  uint32_t total = (uint32_t)wall1Count + (uint32_t)wall2Count + (uint32_t)wall3Count + (uint32_t)wall4Count;
  if (total == 0) {
    return NUM_LEDS;
  }
  if (total > NUM_LEDS) {
    total = NUM_LEDS;
  }
  return (uint16_t)total;
}

inline uint16_t altWrapIndex(int32_t index, uint16_t length) {
  if (length == 0) {
    return 0;
  }
  int32_t wrapped = index % (int32_t)length;
  if (wrapped < 0) {
    wrapped += length;
  }
  return (uint16_t)wrapped;
}

inline void drawAltWrappedWidth(uint16_t center, int width, const CRGB& color, uint16_t activeLeds) {
  if (activeLeds == 0) {
    return;
  }
  int clampedWidth = constrain(width, 0, 12);
  for (int w = -clampedWidth; w <= clampedWidth; w++) {
    uint16_t idx = altWrapIndex((int32_t)center + w, activeLeds);
    leds[idx] += color;
  }
}

inline int altShortestDirection(int from, int to, uint16_t activeLeds) {
  uint16_t cw = altWrapIndex((int32_t)to - from, activeLeds);
  uint16_t ccw = altWrapIndex((int32_t)from - to, activeLeds);
  return (cw <= ccw) ? 1 : -1;
}

inline void clearAltWrappedRange(uint16_t start, uint16_t length, int width, uint16_t activeLeds) {
  if (activeLeds == 0 || length == 0) {
    return;
  }
  int clampedWidth = constrain(width, 0, 12);
  for (uint16_t i = 0; i < length; i++) {
    uint16_t center = altWrapIndex((int32_t)start + i, activeLeds);
    for (int w = -clampedWidth; w <= clampedWidth; w++) {
      uint16_t idx = altWrapIndex((int32_t)center + w, activeLeds);
      leds[idx] = CRGB::Black;
    }
  }
}

inline void fadeAltWrappedRange(uint16_t start, uint16_t length, int width, uint8_t fadeAmount, uint16_t activeLeds) {
  if (activeLeds == 0 || length == 0) {
    return;
  }
  int clampedWidth = constrain(width, 0, 12);
  for (uint16_t i = 0; i < length; i++) {
    uint16_t center = altWrapIndex((int32_t)start + i, activeLeds);
    for (int w = -clampedWidth; w <= clampedWidth; w++) {
      uint16_t idx = altWrapIndex((int32_t)center + w, activeLeds);
      leds[idx].fadeToBlackBy(fadeAmount);
    }
  }
}

inline bool altIsIndexInWrappedSegment(uint16_t index, uint16_t start, uint16_t length, uint16_t activeLeds) {
  if (activeLeds == 0 || length == 0) {
    return false;
  }
  uint16_t offset = altWrapIndex((int32_t)index - (int32_t)start, activeLeds);
  return offset < length;
}

inline bool altIsStrikePhaseActive() {
  return lightningAlt.phase == LIGHTNING_ALT_PRE_FLASH ||
         lightningAlt.phase == LIGHTNING_ALT_STRIKE_PIN ||
         lightningAlt.phase == LIGHTNING_ALT_FLASH ||
         lightningAlt.phase == LIGHTNING_ALT_FLICKER ||
         lightningAlt.phase == LIGHTNING_ALT_RIPPLE ||
         lightningAlt.phase == LIGHTNING_ALT_FADE;
}

inline bool altIsInStrikeSuppressionZone(uint16_t pos, uint16_t activeLeds) {
  if (!altIsStrikePhaseActive() || activeLeds == 0 || lightningAlt.mainStrikeLength <= 0) {
    return false;
  }

  uint16_t strikeLen = (uint16_t)min((int)activeLeds, lightningAlt.mainStrikeLength);
  uint16_t padding = (uint16_t)constrain((int)lightningAlt.mainStrikeWidth + (int)lightningAltGlowSize + 2, 0, (int)activeLeds / 2);
  uint32_t totalLen = (uint32_t)strikeLen + ((uint32_t)padding * 2U);
  if (totalLen >= activeLeds) {
    return true;
  }

  uint16_t zoneStart = altWrapIndex((int32_t)lightningAlt.mainLed1 - (int32_t)padding, activeLeds);
  return altIsIndexInWrappedSegment(pos, zoneStart, (uint16_t)totalLen, activeLeds);
}

inline void clearAltRainDropsInStrikeZone(uint16_t activeLeds) {
  if (!altIsStrikePhaseActive() || activeLeds == 0) {
    return;
  }
  for (int i = 0; i < MAX_RAIN_DROPS_ALT; i++) {
    if (rainAltDrops[i].position >= 0) {
      uint16_t pos = (uint16_t)rainAltDrops[i].position;
      if (altIsInStrikeSuppressionZone(pos, activeLeds)) {
        rainAltDrops[i].position = -1;
      }
    }
  }
}

// ============================================================
// RAIN UPDATE - RELAXING MULTI-TONE EFFECT
// ============================================================
void updateRainEffectAlt(unsigned long currentTime) {
  static unsigned long lastUpdate = 0;

  if (currentTime - lastUpdate > rainAltUpdateDelay) {
    lastUpdate = currentTime;
    const uint16_t activeLeds = altActivePerimeterLeds();

    // Gentle overall fade
    fadeToBlackBy(leds, NUM_LEDS, rainAltFadeAmount);

    // Two independent spawn attempts per tick for ~2x drop throughput.
    if (random8() < rainAltSpawnChance) {
      spawnAltRainDrop(activeLeds, currentTime);
    }
    if (random8() < rainAltSpawnChance) {
      spawnAltRainDrop(activeLeds, currentTime);
    }

    // Update existing drops
    for (int i = 0; i < MAX_RAIN_DROPS_ALT; i++) {
      if (rainAltDrops[i].position >= 0) {
        if (rainAltDrops[i].position >= activeLeds) {
          rainAltDrops[i].position = -1;
          continue;
        }
        if (altIsInStrikeSuppressionZone((uint16_t)rainAltDrops[i].position, activeLeds)) {
          rainAltDrops[i].position = -1;
          continue;
        }
        unsigned long age = currentTime - rainAltDrops[i].birthTime;

        // Peak for a moment, then fade
        if (age < rainAltPeakDuration) {
          rainAltDrops[i].intensity = rainAltBrightness;
        } else {
          rainAltDrops[i].intensity = map(age, rainAltPeakDuration, rainAltPeakDuration + 1000, rainAltBrightness, 0);
        }

        if (rainAltDrops[i].intensity > 0) {
          CRGB dropColor;
          uint8_t intensity = rainAltDrops[i].intensity;
          
          // Select color based on rain color mode
          if (rainAltColorMode == RAIN_STATIC_BLUE) {
            // Solid blue
            dropColor = CRGB(0, 110, 255);
            dropColor.nscale8_video(intensity);
          } else if (rainAltColorMode == RAIN_WHITE_BLUE) {
            // White to blue gradient - more white/cyan drops
            uint8_t colorVariation = random8();
            if (colorVariation < 60) {
              // Mostly white drops
              dropColor = CRGB(intensity / 2, intensity / 2, intensity);
            } else {
              // Some blue drops
              dropColor = CRGB(0, intensity / 2, intensity);
            }
          } else {
            // Multi-tone blue palette (default)
            uint8_t colorVariation = random8();
            if (colorVariation < 100) {
              // Light blue
              dropColor = CRGB(rainAltLightBlue, rainAltLightBlue, 255);
            } else if (colorVariation < 180) {
              // Medium blue
              dropColor = CRGB(rainAltMediumBlue / 2, rainAltMediumBlue, 255);
            } else {
              // Deep blue (most realistic)
              dropColor = CRGB(rainAltDeepBlue / 3, rainAltDeepBlue / 2, 255);
            }
            dropColor.nscale8_video(intensity);
          }

          leds[rainAltDrops[i].position] += dropColor;

          // Optional faint trails
          if (rainAltDrops[i].isTrail) {
            uint8_t trailIntensity = rainAltDrops[i].intensity / 3;
            uint16_t trailPos = altWrapIndex((int32_t)rainAltDrops[i].position - 1, activeLeds);
            leds[trailPos] += CRGB(trailIntensity / 3, trailIntensity / 2, trailIntensity);
          }
        } else {
          rainAltDrops[i].position = -1;  // Mark as unused
        }
      }
    }
  }
}

// ============================================================
// LIGHTNING ALT UPDATE - REALISTIC BRANCHING EFFECT
// ============================================================
void updateLightningAltStrike(unsigned long currentTime) {
  static unsigned long lastStrike = 0;
  const uint16_t activeLeds = altActivePerimeterLeds();

  if (activeLeds == 0) {
    return;
  }

  uint8_t speedPct = (uint8_t)constrain(lightningAltSpeedPercent, 50, 220);

  // Handle idle state - wait for next strike
  if (lightningAlt.phase == LIGHTNING_ALT_IDLE) {
    uint32_t rawDelay;
    if (lightningAltTestMode) {
      rawDelay = lightningAltTestInterval;
    } else {
      uint32_t minI = min(lightningAltMinInterval, lightningAltMaxInterval);
      uint32_t maxI = max(lightningAltMinInterval, lightningAltMaxInterval);

      if (minI == maxI) {
        rawDelay = minI;
      } else if (random8() < lightningAltFrequentChance) {
        // Frequent strikes are biased toward earlier times, but still derived
        // from the configured [min, max] window so large max values are honored.
        uint32_t span = maxI - minI;
        uint32_t frequentUpper = min(maxI, minI + max((uint32_t)1, span / 3));
        rawDelay = (frequentUpper > minI) ? (uint32_t)random(minI, frequentUpper + 1) : minI;
      } else {
        rawDelay = (uint32_t)random(minI, maxI + 1);
      }
    }
    lightningAlt.nextStrikeTime = (unsigned long)constrain(((uint32_t)rawDelay * 100UL) / speedPct, 60UL, 120000UL);

    if (currentTime - lastStrike > lightningAlt.nextStrikeTime) {
      lightningAlt.phase = LIGHTNING_ALT_PRE_FLASH;
      lightningAlt.phaseStart = currentTime;
      lastStrike = currentTime;
    }
    return;
  }

  // PRE_FLASH: Short setup before strike
  if (lightningAlt.phase == LIGHTNING_ALT_PRE_FLASH) {
    if (currentTime - lightningAlt.phaseStart >= 2) {
      lightningAlt.strikeStart = currentTime;
      int minLen = min((int)lightningAltMinStrikeLength, (int)activeLeds);
      int maxLen = min((int)lightningAltMaxStrikeLength, (int)activeLeds);
      if (maxLen < 1) {
        lightningAlt.phase = LIGHTNING_ALT_IDLE;
        return;
      }
      if (minLen < 1) {
        minLen = 1;
      }
      if (minLen > maxLen) {
        minLen = maxLen;
      }

      lightningAlt.mainStrikeLength = random(minLen, maxLen + 1);
      uint16_t minDur = min(lightningAltMinFlashDuration, lightningAltMaxFlashDuration);
      uint16_t maxDur = max(lightningAltMinFlashDuration, lightningAltMaxFlashDuration);
      minDur = constrain(minDur, 10, 2000);
      maxDur = constrain(maxDur, 10, 2000);
      uint16_t rawFlashDuration = random(minDur, maxDur + 1);

      // Keep the original strike cadence (about 5-25ms) and use the duration range
      // as a multiplier so tuning stays responsive without losing the original feel.
      uint16_t baseFlashHold = (uint16_t)random(5, 26);
      uint16_t durationScalePct = (uint16_t)map(
        constrain((int)rawFlashDuration, 10, 2000),
        10,
        2000,
        60,
        260
      );
      uint32_t scaledFlash = (uint32_t)baseFlashHold * (uint32_t)durationScalePct;
      lightningAlt.currentStrikeDuration = (uint16_t)constrain(
        (scaledFlash / 100UL) * 100UL / speedPct,
        3UL,
        1200UL
      );

      lightningAlt.currentFadeDuration = (uint16_t)constrain((uint32_t)random(120, 220) * 100UL / speedPct, 40UL, 700UL);
      lightningAlt.currentMaxLifecycle = (uint16_t)constrain(
        (uint32_t)lightningAlt.currentStrikeDuration + lightningAlt.currentFadeDuration + 240,
        50UL,
        1800UL
      );

      int minWidth = min((int)lightningAltMinStrikeWidth, (int)lightningAltMaxStrikeWidth);
      int maxWidth = max((int)lightningAltMinStrikeWidth, (int)lightningAltMaxStrikeWidth);
      lightningAlt.mainStrikeWidth = random(max(0, minWidth), max(0, maxWidth) + 1);
      if (lightningAltTestMode) {
        lightningAlt.mainLed1 = altWrapIndex((int32_t)lightningAltTestStartLed, activeLeds);
      } else {
        lightningAlt.mainLed1 = random(0, activeLeds);
      }
      lightningAlt.mainLed2 = altWrapIndex((int32_t)lightningAlt.mainLed1 + lightningAlt.mainStrikeLength - 1, activeLeds);

      // Remove rain drops where lightning is about to be rendered.
      clearAltRainDropsInStrikeZone(activeLeds);

      uint8_t comfortPeak = altComfortScale(lightningAltFlashBrightness);
      lightningAlt.isPowerStrike = random(0, 100) < 24;
      uint8_t strikePeak = lightningAlt.isPowerStrike
        ? (uint8_t)min(255, comfortPeak + 18)
        : comfortPeak;
      lightningAlt.strikeColor = lightningAltSimpleWhites[random(0, lightningAltSimpleWhitesCount)];
      lightningAlt.strikeColor.nscale8_video(strikePeak);
      lightningAlt.flickerCount = lightningAlt.isPowerStrike ? random(4, 7) : random(2, 4);
      lightningAlt.flickerStep = 0;
      lightningAlt.rippleStep = 1;
      lightningAlt.rippleRange = lightningAlt.isPowerStrike ? random(10, 21) : random(5, 11);
      lightningAlt.rippleSpeed = (uint8_t)max((uint8_t)1, (uint8_t)(5 * 100 / speedPct));
      lightningAlt.fadeBrightness = 255;

      if (lightningAltStrikePinEnabled && random(0, 100) < lightningAltStrikePinChance) {
        int randomDir = random(0, 2) == 0 ? -1 : 1;
        int maxLeadDistance = min((int)activeLeds / 2, 45);
        int leadDistance = random(20, max(21, maxLeadDistance));
        lightningAlt.pinTarget = lightningAlt.mainLed1;
        lightningAlt.pinPosition = altWrapIndex((int32_t)lightningAlt.pinTarget + (randomDir * leadDistance), activeLeds);
        lightningAlt.pinPrevPosition = -1;
        lightningAlt.pinDirection = altShortestDirection(lightningAlt.pinPosition, lightningAlt.pinTarget, activeLeds);
        lightningAlt.pinBrightness = lightningAltPinStartBrightness;
        lightningAlt.phase = LIGHTNING_ALT_STRIKE_PIN;
      } else {
        for (int i = 0; i < lightningAlt.mainStrikeLength; i++) {
          uint16_t idx = altWrapIndex((int32_t)lightningAlt.mainLed1 + i, activeLeds);
          drawAltWrappedWidth(idx, lightningAlt.mainStrikeWidth, lightningAlt.strikeColor, activeLeds);
        }
        FastLED.show();
        lightningAlt.phase = LIGHTNING_ALT_FLASH;
      }
      lightningAlt.phaseStart = currentTime;
    }
    return;
  }

  // STRIKE_PIN: Fast moving bright pin that races to strike origin
  if (lightningAlt.phase == LIGHTNING_ALT_STRIKE_PIN) {
    uint8_t stepDelay = (uint8_t)max((uint8_t)1, (uint8_t)(((uint16_t)lightningAltPinStepDelay * 100U) / speedPct));
    if (currentTime - lightningAlt.phaseStart >= stepDelay) {
      // Keep this as a single moving point: erase previous pin pixel before drawing next.
      if (lightningAlt.pinPrevPosition >= 0) {
        uint16_t prev = altWrapIndex((int32_t)lightningAlt.pinPrevPosition, activeLeds);
        leds[prev] = CRGB::Black;
      }

      uint16_t curr = altWrapIndex((int32_t)lightningAlt.pinPosition, activeLeds);
      leds[curr] = CRGB(lightningAlt.pinBrightness, lightningAlt.pinBrightness, min(255, lightningAlt.pinBrightness + 20));
      FastLED.show();

      uint16_t scaledStep = ((uint16_t)lightningAltPinStepSize * (uint16_t)lightningAltPinSpeedMultiplier * speedPct) / 100U;
      uint8_t stepSize = max((uint8_t)1, (uint8_t)scaledStep);
      uint16_t remaining = (lightningAlt.pinDirection > 0)
        ? altWrapIndex((int32_t)lightningAlt.pinTarget - lightningAlt.pinPosition, activeLeds)
        : altWrapIndex((int32_t)lightningAlt.pinPosition - lightningAlt.pinTarget, activeLeds);

      if (remaining <= stepSize) {
        lightningAlt.pinPosition = lightningAlt.pinTarget;
        for (int i = 0; i < lightningAlt.mainStrikeLength; i++) {
          uint16_t idx = altWrapIndex((int32_t)lightningAlt.mainLed1 + i, activeLeds);
          drawAltWrappedWidth(idx, lightningAlt.mainStrikeWidth, lightningAlt.strikeColor, activeLeds);
        }
        FastLED.show();
        lightningAlt.phase = LIGHTNING_ALT_FLASH;
        lightningAlt.phaseStart = currentTime;
      } else {
        lightningAlt.pinPrevPosition = lightningAlt.pinPosition;
        lightningAlt.pinPosition = altWrapIndex((int32_t)lightningAlt.pinPosition + (lightningAlt.pinDirection * stepSize), activeLeds);
        lightningAlt.pinBrightness = (uint8_t)min(255, lightningAlt.pinBrightness + lightningAltPinBrightnessStep);
        lightningAlt.phaseStart = currentTime;
      }
    }
    return;
  }

  // FLASH: Bright main channel
  if (lightningAlt.phase == LIGHTNING_ALT_FLASH) {
    if (currentTime - lightningAlt.phaseStart >= lightningAlt.currentStrikeDuration) {
      lightningAlt.phase = LIGHTNING_ALT_FLICKER;
      lightningAlt.phaseStart = currentTime;
    }
    return;
  }

  // FLICKER: Discrete pulse flickers inside strike segment
  if (lightningAlt.phase == LIGHTNING_ALT_FLICKER) {
    uint16_t flickerTick = (uint16_t)max((uint16_t)10, (uint16_t)(50U * 100U / speedPct));
    if (lightningAlt.flickerStep < lightningAlt.flickerCount) {
      if (currentTime - lightningAlt.phaseStart >= flickerTick) {
        // Fade only the active strike footprint so rain/background stay stable.
        fadeAltWrappedRange(
          (uint16_t)lightningAlt.mainLed1,
          (uint16_t)lightningAlt.mainStrikeLength,
          lightningAlt.mainStrikeWidth + 2,
          80,
          activeLeds
        );

        int flashes = max(1, lightningAlt.mainStrikeLength / 3);
        flashes = random(1, flashes + 1);
        uint8_t comfortPeak = altComfortScale(lightningAltFlashBrightness);
        uint8_t flickerPeak = (uint8_t)max((uint8_t)10, (uint8_t)(comfortPeak * 7 / 8));
        uint8_t flickerNeighbor = (uint8_t)max((uint8_t)6, (uint8_t)(flickerPeak * 3 / 4));
        for (int f = 0; f < flashes; f++) {
          uint16_t pos = altWrapIndex((int32_t)lightningAlt.mainLed1 + random(0, max(1, lightningAlt.mainStrikeLength)), activeLeds);
          leds[pos] = CRGB(flickerPeak, flickerPeak, flickerPeak);
          uint16_t left = altWrapIndex((int32_t)pos - 1, activeLeds);
          uint16_t right = altWrapIndex((int32_t)pos + 1, activeLeds);
          leds[left] = CRGB(flickerNeighbor, flickerNeighbor, flickerNeighbor);
          leds[right] = CRGB(flickerNeighbor, flickerNeighbor, flickerNeighbor);
        }

        FastLED.show();
        lightningAlt.phaseStart = currentTime;
        lightningAlt.flickerStep++;
      }
    } else {
      lightningAlt.phase = LIGHTNING_ALT_RIPPLE;
      lightningAlt.phaseStart = currentTime;
    }
    return;
  }

  // RIPPLE: Expand bright edges outward from strike endpoints
  if (lightningAlt.phase == LIGHTNING_ALT_RIPPLE) {
    if (currentTime - lightningAlt.phaseStart >= lightningAlt.rippleSpeed) {
      uint8_t comfortPeak = altComfortScale(lightningAltFlashBrightness);
      uint8_t rippleStart = (uint8_t)max((uint8_t)24, (uint8_t)(comfortPeak * 3 / 4));
      uint8_t rippleBrightness = (uint8_t)map(
        lightningAlt.rippleStep,
        1,
        max((uint8_t)1, lightningAlt.rippleRange),
        rippleStart,
        lightningAlt.isPowerStrike ? 24 : 8
      );

      uint16_t leftPos = altWrapIndex((int32_t)lightningAlt.mainLed1 - lightningAlt.rippleStep, activeLeds);
      uint16_t rightPos = altWrapIndex((int32_t)lightningAlt.mainLed2 + lightningAlt.rippleStep, activeLeds);
      leds[leftPos] = CRGB(rippleBrightness, rippleBrightness, rippleBrightness);
      leds[rightPos] = CRGB(rippleBrightness, rippleBrightness, rippleBrightness);

      FastLED.show();
      lightningAlt.rippleStep++;
      lightningAlt.phaseStart = currentTime;

      if (lightningAlt.rippleStep > lightningAlt.rippleRange) {
        lightningAlt.phase = LIGHTNING_ALT_FADE;
        lightningAlt.phaseStart = currentTime;
      }
    }
    return;
  }

  // FADE: Global brightness falloff, like original lightning effect
  if (lightningAlt.phase == LIGHTNING_ALT_FADE) {
    uint16_t fadeTick = (uint16_t)max((uint16_t)8, (uint16_t)(30U * 100U / speedPct));
    uint8_t fadeStep = (uint8_t)max((uint8_t)2, (uint8_t)(4U * speedPct / 100U));
    if (currentTime - lightningAlt.phaseStart >= fadeTick) {
      if (lightningAlt.fadeBrightness <= fadeStep) {
        lightningAlt.fadeBrightness = 255;
        lightningAlt.phase = LIGHTNING_ALT_DONE;

        clearAltWrappedRange((uint16_t)lightningAlt.mainLed1, (uint16_t)lightningAlt.mainStrikeLength, lightningAlt.mainStrikeWidth + lightningAltGlowSize, activeLeds);
        FastLED.show();
      } else {
        lightningAlt.fadeBrightness -= fadeStep;

        // Fade only the lightning footprint; do not touch global strip brightness.
        uint8_t localFade = (uint8_t)constrain((int)fadeStep * 8, 8, 84);
        fadeAltWrappedRange(
          (uint16_t)lightningAlt.mainLed1,
          (uint16_t)lightningAlt.mainStrikeLength,
          lightningAlt.mainStrikeWidth + lightningAltGlowSize,
          localFade,
          activeLeds
        );

        FastLED.show();
        lightningAlt.phaseStart = currentTime;
      }
    }
    return;
  }

  if (lightningAlt.phase == LIGHTNING_ALT_DONE) {
    lightningAlt.phase = LIGHTNING_ALT_IDLE;
    lastStrike = currentTime;
    return;
  }

  // Safety cleanup for any unexpected state
  if (lightningAlt.phase != LIGHTNING_ALT_IDLE) {
    if (currentTime - lightningAlt.strikeStart > lightningAlt.currentMaxLifecycle) {
      clearAltWrappedRange((uint16_t)lightningAlt.mainLed1, (uint16_t)lightningAlt.mainStrikeLength, lightningAlt.mainStrikeWidth + lightningAltGlowSize, activeLeds);
      FastLED.show();
      lightningAlt.phase = LIGHTNING_ALT_IDLE;
    }
  }
}

#endif
