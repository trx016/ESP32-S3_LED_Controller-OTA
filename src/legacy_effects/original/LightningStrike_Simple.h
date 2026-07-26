#ifndef LIGHTNING_STRIKE_SIMPLE_H
#define LIGHTNING_STRIKE_SIMPLE_H

// ============================================================
// SIMPLE HORIZONTAL LIGHTNING
// Fast white strikes across walls with quick dim linger
// Optimized for square wall layout (400 LEDs per wall)
// ============================================================

enum SimpleLightningPhase {
  SIMPLE_IDLE,
  SIMPLE_FLASH,
  SIMPLE_LINGER,
  SIMPLE_DONE
};

struct SimpleLightningState {
  SimpleLightningPhase phase;
  unsigned long phaseStartTime;
  unsigned long lastStrikeTime;
  unsigned long nextStrikeDelay;
  int strikeWall;           // Which wall (0-3) for the strike
  int strikeStart;          // Start LED index
  int strikeLength;         // Length of strike
  CRGB strikeColor;
};

SimpleLightningState simpleLightning;

// ============================================================
// CONFIGURATION
// ============================================================

uint16_t simpleLightningMinLength = 50;      // Min strike length
uint16_t simpleLightningMaxLength = 200;     // Max strike length
uint16_t simpleLightningFlashDuration = 12;  // Bright flash duration (ms)
uint16_t simpleLightningLingerDuration = 60; // Dim fade duration (ms)
uint8_t simpleLightningLingerBrightness = 80; // Brightness during linger (0-255)
uint16_t simpleLightningMinInterval = 1000;  // Min ms between strikes
uint16_t simpleLightningMaxInterval = 5000;  // Max ms between strikes

// White color palette (no yellow, just slightly different whites)
const CRGB whiteColors[] = {
  CRGB(255, 255, 255),  // Pure white
  CRGB(250, 250, 255),  // Slight cool white
  CRGB(248, 250, 255),  // Cool white tint
  CRGB(245, 248, 255),  // Cool white tint
  CRGB(238, 242, 255),  // Slightly dim cool white
};
const uint8_t numWhiteColors = sizeof(whiteColors) / sizeof(whiteColors[0]);

// ============================================================
// WALL MAPPING
// Square layout: 400 LEDs per wall
// Wall 0: Left wall (LEDs 0-399)
// Wall 1: Bottom wall (LEDs 400-799)
// Wall 2: Right wall (LEDs 800-1199)
// Wall 3: Top wall (LEDs 1200-1599)
// ============================================================

int getWallStartLED(uint8_t wall) {
  return wall * 400;
}

// Get LED position for a horizontal strike on a wall
// wall: 0-3, position: 0-399 on that wall
int getStrikeLED(uint8_t wall, int position) {
  return getWallStartLED(wall) + position;
}

// ============================================================
// UPDATE FUNCTION
// ============================================================

void beginSimpleStrike(unsigned long currentTime) {
  simpleLightning.strikeWall = random(0, 4);

  int maxStart = 400 - (int)simpleLightningMinLength;
  simpleLightning.strikeStart = random(0, maxStart > 0 ? maxStart : 1);
  simpleLightning.strikeLength = random((int)simpleLightningMinLength, (int)simpleLightningMaxLength + 1);

  if (simpleLightning.strikeStart + simpleLightning.strikeLength > 400) {
    simpleLightning.strikeLength = 400 - simpleLightning.strikeStart;
  }

  if (simpleLightning.strikeLength < (int)simpleLightningMinLength) {
    simpleLightning.strikeLength = simpleLightningMinLength;
    if (simpleLightning.strikeLength > 400) {
      simpleLightning.strikeLength = 400;
    }
    if (simpleLightning.strikeStart + simpleLightning.strikeLength > 400) {
      simpleLightning.strikeStart = 400 - simpleLightning.strikeLength;
    }
  }

  simpleLightning.strikeColor = whiteColors[random(0, numWhiteColors)];
  simpleLightning.phase = SIMPLE_FLASH;
  simpleLightning.phaseStartTime = currentTime;
  simpleLightning.lastStrikeTime = currentTime;
  simpleLightning.nextStrikeDelay = random(simpleLightningMinInterval, simpleLightningMaxInterval + 1);
}

void initSimpleLightningState() {
  simpleLightning.phase = SIMPLE_IDLE;
  simpleLightning.phaseStartTime = 0;
  simpleLightning.lastStrikeTime = 0;
  simpleLightning.nextStrikeDelay = random(simpleLightningMinInterval, simpleLightningMaxInterval + 1);
  simpleLightning.strikeWall = 0;
  simpleLightning.strikeStart = 0;
  simpleLightning.strikeLength = simpleLightningMinLength;
  simpleLightning.strikeColor = whiteColors[0];
}

void updateSimpleLightning(unsigned long currentTime) {
  if (simpleLightning.phase == SIMPLE_IDLE || simpleLightning.phase == SIMPLE_DONE) {
    if (simpleLightning.lastStrikeTime == 0 || currentTime - simpleLightning.lastStrikeTime >= simpleLightning.nextStrikeDelay) {
      beginSimpleStrike(currentTime);
    }
    return;
  }

  unsigned long phaseAge = currentTime - simpleLightning.phaseStartTime;

  // ============================================================
  // FLASH PHASE - Bright white strike
  // ============================================================
  if (simpleLightning.phase == SIMPLE_FLASH) {
    if (phaseAge < simpleLightningFlashDuration) {
      int wallStart = getWallStartLED(simpleLightning.strikeWall);

      for (int i = 0; i < simpleLightning.strikeLength; i++) {
        int ledPos = wallStart + simpleLightning.strikeStart + i;
        if (ledPos < NUM_LEDS) {
          leds[ledPos] += simpleLightning.strikeColor;
        }
      }
    } else {
      // Transition to linger
      simpleLightning.phase = SIMPLE_LINGER;
      simpleLightning.phaseStartTime = currentTime;
    }
  }

  // ============================================================
  // LINGER PHASE - Dim white fade
  // ============================================================
  else if (simpleLightning.phase == SIMPLE_LINGER) {
    if (phaseAge < simpleLightningLingerDuration) {
      uint8_t brightness = map(phaseAge, 0, simpleLightningLingerDuration, 255, simpleLightningLingerBrightness);
      CRGB lingerColor = simpleLightning.strikeColor;
      lingerColor.nscale8_video(brightness);

      int wallStart = getWallStartLED(simpleLightning.strikeWall);

      for (int i = 0; i < simpleLightning.strikeLength; i++) {
        int ledPos = wallStart + simpleLightning.strikeStart + i;
        if (ledPos < NUM_LEDS) {
          leds[ledPos] += lingerColor;
        }
      }
    } else {
      simpleLightning.phase = SIMPLE_DONE;
    }
  }
}

#endif // LIGHTNING_STRIKE_SIMPLE_H
