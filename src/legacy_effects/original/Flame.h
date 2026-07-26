#include <stdint.h>

// User-configurable variables (extern in your .ino)
extern uint8_t flameCool;           // base cooling speed (1-50)
extern uint8_t flameSparking;       // spark spawn chance (0-255, now with extended range)
extern uint16_t flameUpdateDelay;
extern uint8_t flameClumpMinSize;   // min spark width
extern uint8_t flameClumpMaxSize;   // max spark width
extern uint8_t flamePaletteMode;    // 0-4 visual palette selection
extern uint8_t flameBgEmberLevel;   // 0-30 ember floor intensity
extern uint8_t BRIGHTNESS;          // global brightness for adaptive scaling

// Internal state per LED
static byte clumpIntensity[NUM_LEDS];  // current brightness of spark
static byte clumpFadeRate[NUM_LEDS];   // fade rate per LED
static byte sparkType[NUM_LEDS];       // spark detail type (0=none, 1-3 for size variants)
static byte sparkTarget[NUM_LEDS];     // target brightness for growth phase
static byte sparkRiseRate[NUM_LEDS];   // growth speed per LED
static byte sparkLifeScale[NUM_LEDS];  // per-spark lifetime scale (% fade speed)
static byte sparkSpreadPower[NUM_LEDS]; // remaining outward spread strength

struct FlamePalette {
  CRGB low;
  CRGB mid;
  CRGB high;
  CRGB peak;
};

static const FlamePalette flamePalettes[5] = {
  // 0 - Existing classic ember palette
  {CRGB(220, 25, 0), CRGB(244, 95, 0), CRGB(250, 175, 30), CRGB(236, 228, 120)},
  // 1 - Neon blend
  {CRGB(190, 0, 170), CRGB(255, 0, 120), CRGB(110, 20, 255), CRGB(0, 245, 220)},
  // 2 - Cool blue
  {CRGB(0, 35, 150), CRGB(0, 110, 230), CRGB(70, 205, 255), CRGB(205, 245, 255)},
  // 3 - Arctic white variant
  {CRGB(40, 60, 100), CRGB(120, 155, 210), CRGB(210, 230, 255), CRGB(255, 255, 255)},
  // 4 - Warm white variant
  {CRGB(110, 40, 10), CRGB(180, 110, 45), CRGB(240, 205, 140), CRGB(255, 240, 210)}
};

inline CRGB blendCRGB(const CRGB& a, const CRGB& b, uint8_t amount) {
  return CRGB(
    lerp8by8(a.r, b.r, amount),
    lerp8by8(a.g, b.g, amount),
    lerp8by8(a.b, b.b, amount)
  );
}

inline FlamePalette getActiveFlamePalette(unsigned long currentTime) {
  uint8_t paletteIndex = (uint8_t)constrain((int)flamePaletteMode, 0, 4);
  FlamePalette p = flamePalettes[paletteIndex];

  // Neon blend: slowly rotate only the base (low) anchor through neon tones.
  if (paletteIndex == 1) {
    static const CRGB neonBaseCycle[] = {
      CRGB(255, 0, 170),
      CRGB(175, 0, 255),
      CRGB(0, 210, 255),
      CRGB(0, 255, 170),
      CRGB(255, 20, 120)
    };
    const uint8_t cycleCount = sizeof(neonBaseCycle) / sizeof(neonBaseCycle[0]);
    const uint32_t stepMs = 18000UL;
    uint32_t cyclePos = (uint32_t)(currentTime % (stepMs * cycleCount));
    uint8_t idx = (uint8_t)(cyclePos / stepMs);
    uint8_t nextIdx = (uint8_t)((idx + 1U) % cycleCount);
    uint32_t within = cyclePos % stepMs;
    uint8_t t = (uint8_t)((within * 255UL) / stepMs);
    p.low = blendCRGB(neonBaseCycle[idx], neonBaseCycle[nextIdx], ease8InOutApprox(t));
  }

  return p;
}

// Helper: Get high-resolution color gradient for spark intensity
// This provides fine detail and smooth transitions across the full range
inline CRGB getFlameColorForIntensity(byte intensity, unsigned long currentTime) {
  FlamePalette p = getActiveFlamePalette(currentTime);

  if (intensity < 85) {
    uint8_t t = (uint8_t)((uint16_t)intensity * 255U / 84U);
    return blendCRGB(p.low, p.mid, t);
  }
  if (intensity < 170) {
    uint8_t t = (uint8_t)(((uint16_t)(intensity - 85) * 255U) / 84U);
    return blendCRGB(p.mid, p.high, t);
  }

  uint8_t t = (uint8_t)(((uint16_t)(intensity - 170) * 255U) / 85U);
  return blendCRGB(p.high, p.peak, t);
}

// Helper: Determine spark size variant (creates size variance)
// Returns 1 (small delicate), 2 (medium), or 3 (large bright)
inline uint8_t getSparkSizeVariant(uint8_t roll) {
  if (roll < 120) return 1;        // 47% small sparks (lots of delicate detail)
  else if (roll < 200) return 2;   // 31% medium sparks
  else return 3;                   // 22% large bright sparks
}

inline uint8_t flameHash8(uint16_t x) {
  x ^= (x << 7);
  x ^= (x >> 9);
  x ^= (x << 8);
  return (uint8_t)x;
}

// Subtle ember bed for low-light ambience without overpowering sparks.
inline CRGB getFlameBaseEmber(int ledIndex, unsigned long currentTime) {
  uint8_t emberLevel = (uint8_t)constrain((int)flameBgEmberLevel, 0, 30);
  if (emberLevel == 0) {
    return CRGB::Black;
  }

  uint8_t brightnessScale = 120;
  if (BRIGHTNESS <= 25) {
    brightnessScale = 180;
  } else if (BRIGHTNESS <= 60) {
    brightnessScale = 150;
  } else if (BRIGHTNESS <= 120) {
    brightnessScale = 130;
  }
  uint8_t paletteIndex = (uint8_t)constrain((int)flamePaletteMode, 0, 4);
  FlamePalette p = getActiveFlamePalette(currentTime);
  uint8_t emberBase = scale8(emberLevel, brightnessScale);
  if (emberBase == 0) {
    emberBase = 1;
  }

  // Non-traveling random flicker: each LED twinkles independently over time,
  // with no spatial phase progression that can look like rotation.
  // Keep this intentionally slow/smooth for motion comfort.
  uint16_t tick = (uint16_t)(currentTime >> 10);  // ~1024ms per state
  uint16_t ledSeed = (uint16_t)(ledIndex * 131);
  uint8_t n0 = flameHash8((uint16_t)(ledSeed ^ (tick * 11U)));
  uint8_t n1 = flameHash8((uint16_t)(ledSeed ^ ((tick + 1U) * 11U)));
  uint8_t blend = (uint8_t)((currentTime & 0x3FFUL) >> 2);
  blend = ease8InOutApprox(blend);
  uint8_t randomFlicker = lerp8by8(n0, n1, blend);
  randomFlicker = 96 + scale8(randomFlicker, 80);
  uint8_t textureScale = (emberBase > 7) ? (emberBase >> 3) : 1;
  uint8_t texture = scale8(randomFlicker, textureScale);

  uint8_t r = qadd8(scale8(p.low.r, emberBase), scale8(p.low.r, texture));
  uint8_t g = qadd8(scale8(p.low.g, emberBase), scale8(p.low.g, texture));
  uint8_t b = qadd8(scale8(p.low.b, emberBase), scale8(p.low.b, texture));

  return CRGB(r, g, b);
}

inline int wrapFlameIndex(int idx) {
  if (idx < 0) {
    return idx + NUM_LEDS;
  }
  if (idx >= NUM_LEDS) {
    return idx - NUM_LEDS;
  }
  return idx;
}

inline int pickFlameSpawnCenter() {
  // Often branch from existing flame so ignition feels connected.
  if (random8() < 170) {
    for (uint8_t tries = 0; tries < 20; tries++) {
      int candidate = random(NUM_LEDS);
      if (clumpIntensity[candidate] > 30) {
        int drift = (int)random8(0, 7) - 3;
        return wrapFlameIndex(candidate + drift);
      }
    }
  }
  return random(NUM_LEDS);
}

inline void igniteFlameSeed(int idx, uint8_t target, uint8_t riseRate, uint8_t sizeVariant, uint8_t spreadPower = 0) {
  sparkTarget[idx] = max(sparkTarget[idx], target);
  sparkType[idx] = sizeVariant;
  clumpFadeRate[idx] = random8(1, 8);
  sparkRiseRate[idx] = max(sparkRiseRate[idx], riseRate);
  sparkSpreadPower[idx] = max(sparkSpreadPower[idx], spreadPower);
  if (sizeVariant == 1) {
    sparkLifeScale[idx] = random8(95, 175);   // short/flashy small sparks
  } else if (sizeVariant == 2) {
    sparkLifeScale[idx] = random8(75, 160);   // wider medium variance
  } else {
    sparkLifeScale[idx] = random8(60, 145);   // large sparks can linger longer
  }
}

void updateFlameEffect(unsigned long currentTime) {
  static unsigned long lastUpdate = 0;
  
  if (currentTime - lastUpdate > flameUpdateDelay) {
    lastUpdate = currentTime;

    // Adjust fade rate based on global brightness
    uint8_t brightnessFactor = BRIGHTNESS < 80 ? 2 : 1;

    // Step 1: Grow new sparks to target, then fade established sparks.
    for (int i = 0; i < NUM_LEDS; i++) {
      if (sparkTarget[i] > clumpIntensity[i]) {
        uint8_t rise = max((uint8_t)1, (uint8_t)(sparkRiseRate[i] + random8(0, 2)));
        clumpIntensity[i] = qadd8(clumpIntensity[i], rise);

        if (clumpIntensity[i] >= sparkTarget[i]) {
          clumpIntensity[i] = sparkTarget[i];
          sparkTarget[i] = 0;
        }
        continue;
      }

      // Per-LED flicker varies by current intensity for more natural feel
      byte flickerAmp = scale8(random8(0, 5), clumpIntensity[i] >> 5);
      byte baseFade = (flameCool / brightnessFactor) + (clumpFadeRate[i] / 2) + flickerAmp;
      uint8_t lifeScale = (sparkLifeScale[i] == 0) ? 120 : sparkLifeScale[i];
      uint16_t shapedFade16 = ((uint16_t)baseFade * lifeScale) / 100U;
      if (shapedFade16 < 1U) {
        shapedFade16 = 1U;
      }
      byte shapedFade = (byte)min((uint16_t)255U, shapedFade16);
      
      // Nonlinear fade: larger sparks fade faster for dramatic effect
      if (sparkType[i] == 3 && clumpIntensity[i] > 200) {
        clumpIntensity[i] = qsub8(clumpIntensity[i], (byte)min(255, shapedFade + (shapedFade >> 1)));
      } else if (clumpIntensity[i] > 180) {
        clumpIntensity[i] = qsub8(clumpIntensity[i], (byte)max(1, shapedFade + (shapedFade >> 3)));
      } else {
        clumpIntensity[i] = qsub8(clumpIntensity[i], shapedFade);
      }

      // Occasional linger on low-energy embers increases lifetime variation.
      if (clumpIntensity[i] > 0 && clumpIntensity[i] < 90 && random8() < 36) {
        clumpIntensity[i] = qadd8(clumpIntensity[i], random8(1, 4));
      }

      if (clumpIntensity[i] == 0) {
        sparkType[i] = 0;
        sparkRiseRate[i] = 0;
        sparkLifeScale[i] = 0;
        sparkSpreadPower[i] = 0;
      }
    }

    // Step 2: Expand from active seeds over time (single-point growth feel).
    for (int i = 0; i < NUM_LEDS; i++) {
      uint8_t spread = sparkSpreadPower[i];
      if (spread == 0 || clumpIntensity[i] < 36) {
        continue;
      }

      uint8_t sizeVariant = sparkType[i];
      uint8_t chance = (uint8_t)constrain((int)(40 + spread * 20 + sizeVariant * 10), 0, 230);
      uint8_t childSpread = (spread > 1) ? (uint8_t)(spread - 1) : 0;
      uint8_t riseMin = (sizeVariant == 3) ? 4 : ((sizeVariant == 2) ? 8 : 14);
      uint8_t riseMax = (sizeVariant == 3) ? 10 : ((sizeVariant == 2) ? 17 : 28);

      if (random8() < chance) {
        int leftIdx = wrapFlameIndex(i - 1);
        uint8_t leftTarget = qsub8(clumpIntensity[i], random8(16, 44));
        if (leftTarget > 18) {
          igniteFlameSeed(leftIdx, qadd8(clumpIntensity[leftIdx], leftTarget), random8(riseMin, riseMax), sizeVariant, childSpread);
        }
      }
      if (random8() < chance) {
        int rightIdx = wrapFlameIndex(i + 1);
        uint8_t rightTarget = qsub8(clumpIntensity[i], random8(16, 44));
        if (rightTarget > 18) {
          igniteFlameSeed(rightIdx, qadd8(clumpIntensity[rightIdx], rightTarget), random8(riseMin, riseMax), sizeVariant, childSpread);
        }
      }

      // Occasional side branch that skips one LED adds organic flame tongues.
      if (spread > 2 && random8() < (sizeVariant == 3 ? 85 : 55)) {
        int dir = (random8() < 128) ? -1 : 1;
        int branchIdx = wrapFlameIndex(i + (dir * 2));
        uint8_t branchTarget = qsub8(clumpIntensity[i], random8(28, 62));
        if (branchTarget > 16) {
          igniteFlameSeed(branchIdx, qadd8(clumpIntensity[branchIdx], branchTarget), random8(riseMin, riseMax), sizeVariant, (uint8_t)(childSpread / 2));
        }
      }

      // Reduce spread budget gradually so expansion naturally dies out.
      if (random8() < 110 || spread > 4) {
        sparkSpreadPower[i] = spread - 1;
      }
    }

    // Step 3: Spawn new sparks as single-core ignition points.
    // Add a second roll at high sparking levels. At 255 this yields ~2x cores.
    uint8_t extraSpawnChance = scale8(flameSparking, flameSparking);
    uint8_t spawnRolls = (random8() < extraSpawnChance) ? 2 : 1;
    for (uint8_t roll = 0; roll < spawnRolls; roll++) {
      if (random8() >= flameSparking) {
        continue;
      }

      int center = pickFlameSpawnCenter();
      uint8_t sizeVariant = getSparkSizeVariant(random8());

      int minW, maxW, spawnIntMin, spawnIntMax;
      uint8_t riseMin, riseMax;

      if (sizeVariant == 1) {
        // Small delicate sparks
        minW = 1;
        maxW = 2;
        spawnIntMin = 100;
        spawnIntMax = 160;
        riseMin = 16;
        riseMax = 30;
      } else if (sizeVariant == 2) {
        // Medium sparks
        minW = flameClumpMinSize;
        maxW = flameClumpMaxSize;
        spawnIntMin = BRIGHTNESS < 80 ? 140 : 160;
        spawnIntMax = BRIGHTNESS < 80 ? 200 : 220;
        riseMin = 9;
        riseMax = 18;
      } else {
        // Large sparks bloom slower and branch more.
        minW = max((int)flameClumpMaxSize, 4);
        maxW = min((int)flameClumpMaxSize + 3, 16);
        spawnIntMin = 200;
        spawnIntMax = 255;
        riseMin = 4;
        riseMax = 10;
      }

      int halfWidth = random(minW, maxW + 1) / 2;

      // Core ignition only; outward growth happens in the temporal spread step.
      uint8_t coreTarget = qadd8(clumpIntensity[center], random8(spawnIntMin, spawnIntMax));
      uint8_t spreadPower = (uint8_t)constrain(halfWidth + random8(0, 3), 1, 16);
      igniteFlameSeed(center, coreTarget, random8(riseMin, riseMax), sizeVariant, spreadPower);
    }

    // Step 4: Draw sparks over a subtle ember background
    for (int i = 0; i < NUM_LEDS; i++) {
      CRGB color = getFlameBaseEmber(i, currentTime);

      // Only draw if spark has intensity
      if (clumpIntensity[i] > 0) {
        CRGB flameColor = getFlameColorForIntensity(clumpIntensity[i], currentTime);
        
        // Full intensity scaling for bright sparks
        color.r = scale8(flameColor.r, clumpIntensity[i]);
        color.g = scale8(flameColor.g, clumpIntensity[i]);
        color.b = scale8(flameColor.b, clumpIntensity[i]);
      }

      leds[i] = color;
    }
  }
}
