#include <stdint.h>
#ifndef LIGHTNING_STRIKE_H
#define LIGHTNING_STRIKE_H

#include <FastLED.h>
#include "LEDConfig.h"

extern CRGB leds[NUM_LEDS];
extern unsigned long lastStrikeTime;
extern unsigned long nextStrikeInterval;
extern uint8_t BRIGHTNESS;

// Lightning tunable parameters
extern uint8_t lightningFlashBrightness;      // Peak brightness of flash (0-255)
extern uint16_t lightningFlashDuration;       // How long initial flash lasts (ms)
extern uint8_t lightningFlickerCount;         // Number of flicker pulses (0-10)
extern uint16_t lightningFlickerDelay;        // Delay between flickers (ms)
extern uint8_t lightningBranchChance;         // Chance of side branches (0-100)
extern uint8_t lightningBranchLength;         // How far branches extend
extern uint16_t lightningRippleDuration;      // Duration of ripple effect (ms)
extern uint8_t lightningFadeSpeed;            // How fast it fades (0-255)
extern uint8_t lightningSecondaryChance;      // Chance of secondary strike (0-100)
extern uint16_t lightningSecondaryDelay;      // Delay before secondary strike (ms)
extern uint8_t lightningGlowSize;             // Size of corona glow around strike
extern uint8_t lightningColorVariation;       // Blue component in fade (0-255)

const int MAX_STRIKE_LENGTH = 80; // Max strike length

bool debugBypass = false;

// ---------------------------
// ENUM AND STRUCT DEFINITIONS
// ---------------------------
enum StrikePhase {
  STRIKE_IDLE,
  STRIKE_FLASH,
  STRIKE_FLICKER,
  STRIKE_RIPPLE,
  STRIKE_FADE,
  STRIKE_DONE
};

struct LightningStrikeState {
  StrikePhase phase = STRIKE_IDLE;
  unsigned long phaseStart = 0;

  int led1, led2;
  int leftBranch1, leftBranch2;
  int rightBranch1, rightBranch2;
  int flickerCount = 0;
  int flickerStep = 0;
  int rippleStep = 1;
  int rippleRange = 0;
  uint8_t fadeBrightness = 255;

  bool isPowerStrike = false;
  bool hasSecondaryStrike = false;
  CRGB strikeColor;
  int rippleSpeed = 5;
};

LightningStrikeState strike;

// ---------------------------
// SET STRIKE INTERVAL LOGIC
// ---------------------------
void updateNextStrikeInterval() {
  if (debugBypass) {
    nextStrikeInterval = 2000;
    return;
  }

  int roll = random(100);
  if (roll < 50) nextStrikeInterval = random(5000, 15000);        // Frequent
  else if (roll < 80) nextStrikeInterval = random(15000, 30000);  // Moderate
  else nextStrikeInterval = random(30000, 120000);                 // Rare
}

// ---------------------------
// START STRIKE SETUP
// ---------------------------
void beginLightningStrike() {
  strike.phase = STRIKE_FLASH;
  strike.phaseStart = millis();

  // Select two LEDs randomly
  strike.led1 = random(NUM_LEDS);
  strike.led2 = random(NUM_LEDS);
  if (strike.led1 > strike.led2) {
    int temp = strike.led1;
    strike.led1 = strike.led2;
    strike.led2 = temp;
  }

  // Limit the max length of the strike
  int length = strike.led2 - strike.led1 + 1;
  if (length > MAX_STRIKE_LENGTH) {
    strike.led2 = strike.led1 + MAX_STRIKE_LENGTH - 1;
    if (strike.led2 >= NUM_LEDS) {
      strike.led2 = NUM_LEDS - 1;
      strike.led1 = strike.led2 - MAX_STRIKE_LENGTH + 1;
    }
  }

  length = strike.led2 - strike.led1 + 1;

  strike.isPowerStrike = random(0, 100) < 30;

  // Define appearance and timing based on power
  strike.strikeColor = strike.isPowerStrike ? CRGB::White : CRGB(CHSV(0, 0, 255));
  strike.flickerCount = strike.isPowerStrike ? random(4, 7) : random(2, 4);
  strike.rippleRange = strike.isPowerStrike ? random(10, 21) : random(5, 11);
  strike.fadeBrightness = BRIGHTNESS;
  strike.flickerStep = 0;
  strike.rippleStep = 1;

  // Flash initial range of LEDs
  for (int i = strike.led1; i <= strike.led2; i++) {
    leds[i] = strike.strikeColor;
  }
  FastLED.show();
}


// ---------------------------
// UPDATE STRIKE LOGIC
// ---------------------------
void updateLightningStrike() {
  unsigned long now = millis();

  switch (strike.phase) {

    // -------------------------
    // PHASE 1: FLASH (Replaces: delay(random(20, 50)))
    // -------------------------
    case STRIKE_FLASH:
      if (now - strike.phaseStart >= random(5, 25)) {
        strike.phase = STRIKE_FLICKER;
        strike.phaseStart = now;
      }
      break;

      // -------------------------
      // PHASE 2: FLICKER (Replaces: delay(random(30, 60)))
      // -------------------------
    case STRIKE_FLICKER:
      if (strike.flickerStep < strike.flickerCount) {
        if (now - strike.phaseStart >= 50) {  // slowed to 50ms for better visibility
          // Fade LEDs to darken previous frame
          for (int i = 0; i < NUM_LEDS; i++) {
            leds[i].fadeToBlackBy(80);
          }

          int length = strike.led2 - strike.led1 + 1;
          int flashes = max(2, length / 3);
          flashes = random(1, flashes + 1);

          // Add flicker pulses within the LED range
          for (int f = 0; f < flashes; f++) {
            int pos = random(strike.led1, strike.led2 + 1);
            leds[pos] = CHSV(0, 0, 255);
            if (pos > strike.led1) leds[pos - 1] = CHSV(0, 0, 200);
            if (pos < strike.led2) leds[pos + 1] = CHSV(0, 0, 200);
          }

          FastLED.show();
          strike.phaseStart = now;
          strike.flickerStep++;
        }
      } else {
        strike.phase = STRIKE_RIPPLE;
        strike.phaseStart = now;
      }
      break;


    // -------------------------
    // PHASE 3: RIPPLE (Replaces: delay(rippleSpeed))
    // -------------------------
    case STRIKE_RIPPLE:
      if (now - strike.phaseStart >= strike.rippleSpeed) {
        uint8_t rippleBrightness = map(strike.rippleStep, 1, strike.rippleRange, 255, strike.isPowerStrike ? 30 : 10);

        int leftPos = strike.led1 - strike.rippleStep;
        int rightPos = strike.led2 + strike.rippleStep;

        if (leftPos >= 0) {
          leds[leftPos] = CRGB(rippleBrightness, rippleBrightness, rippleBrightness);
        }
        if (rightPos < NUM_LEDS) {
          leds[rightPos] = CRGB(rippleBrightness, rippleBrightness, rippleBrightness);
        }

        FastLED.show();
        strike.rippleStep++;
        strike.phaseStart = now;

        if (strike.rippleStep > strike.rippleRange) {
          strike.phase = STRIKE_FADE;
          strike.phaseStart = now;
        }
      }
      break;

    // -------------------------
    // PHASE 4: FADE OUT (Replaces: delay(30))
    // -------------------------
    case STRIKE_FADE:
      if (now - strike.phaseStart >= 30) {
        strike.fadeBrightness -= 4;

        if (strike.fadeBrightness <= 0) {
          // Reset brightness and clean up
          strike.fadeBrightness = BRIGHTNESS;
          FastLED.setBrightness(strike.fadeBrightness);
          strike.phase = STRIKE_DONE;
          clearLeds();
          FastLED.show();
        } else {
          FastLED.setBrightness(strike.fadeBrightness);
          FastLED.show();
          strike.phaseStart = now;
        }
      }
      break;

    // -------------------------
    // PHASE 5: CLEANUP & NEXT STRIKE TIMER SETUP
    // -------------------------
    case STRIKE_DONE:
      strike.phase = STRIKE_IDLE;
      lastStrikeTime = now;
      updateNextStrikeInterval();
      break;

    default:
      break;
  }
}
#endif
