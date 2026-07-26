#ifndef PACMAN_EFFECT_H
#define PACMAN_EFFECT_H

#define MAX_COINS 6
#define MAX_GHOSTS 6

int coins[MAX_COINS] = { -1, -1, -1 };
unsigned long lastCoinRespawn = 0;
const unsigned long coinRespawnInterval = 10000;

extern CRGB leds[NUM_LEDS];

struct Ghost {
  int pos;
  int dir;
  CRGB normalColor;
  bool vulnerable;
};

Ghost ghosts[MAX_GHOSTS];
int pacmanPos = 0;
int pacmanDir = 1;
int pacmanSpeed = 200;
unsigned long lastMoveTime = 0;

bool blinkZone[NUM_LEDS] = { false };
unsigned long blinkStartTime = 0;
const unsigned long blinkDuration = 600;

bool powerMode = false;
unsigned long powerStart = 0;
const unsigned long powerDuration = 6000;

void fadeAllLeds(uint8_t fadeAmount = 180) {
  for (int i = 0; i < NUM_LEDS; i++) {
    bool isCoinPixel = false;
    for (int c = 0; c < MAX_COINS; c++) {
      if (coins[c] == i) {
        isCoinPixel = true;
        break;
      }
    }
    if (!isCoinPixel) {
      leds[i].fadeToBlackBy(fadeAmount);
    }
  }
}


void respawnPacman() {
  bool collision;
  do {
    collision = false;
    pacmanPos = random(NUM_LEDS);
    for (int i = 0; i < MAX_GHOSTS; i++) {
      if (ghosts[i].pos == pacmanPos) {
        collision = true;
        break;
      }
    }
  } while (collision);
  pacmanDir = (random(2) == 0) ? -1 : 1;
}

void spawnCoins() {
  for (int i = 0; i < MAX_COINS; i++) {
    if (coins[i] == -1) {
      int pos;
      do {
        pos = random(NUM_LEDS);
      } while (leds[pos].getAverageLight() > 20 || pos == pacmanPos);
      coins[i] = pos;
    }
  }
}

void clearCoin(int index) {
  coins[index] = -1;
}

void drawCoins() {
  for (int i = 0; i < MAX_COINS; i++) {
    if (coins[i] != -1) {
      uint8_t pulse = beatsin8(4, 150, 255);
      leds[coins[i]] = CHSV(160, 255, pulse); // Blue pulsing
    }
  }
}

void setupPacmanEffect() {
  FastLED.clear();
  respawnPacman();

  for (int i = 0; i < MAX_GHOSTS; i++) {
    ghosts[i].pos = random(NUM_LEDS);
    ghosts[i].dir = (random(2) == 0) ? -1 : 1;
    ghosts[i].normalColor = CHSV(random8(), 255, 255);
    ghosts[i].vulnerable = false;
  }
}

void updateGhosts() {
  for (int i = 0; i < MAX_GHOSTS; i++) {
    if (random8() < 50) {
      ghosts[i].pos += ghosts[i].dir;
      if (ghosts[i].pos <= 0 || ghosts[i].pos >= NUM_LEDS - 1) {
        ghosts[i].dir *= -1;
        ghosts[i].pos += ghosts[i].dir;
      }
    }
  }
}

void flickerPacmanCollision() {
  const int flickerCount = 4;
  const int flickerDelay = 80;

  for (int i = 0; i < flickerCount; i++) {
    for (int j = -1; j <= 1; j++) {
      int pos = pacmanPos + j;
      if (pos >= 0 && pos < NUM_LEDS) leds[pos] = CRGB::Black;
    }
    FastLED.show();
    delay(flickerDelay);

    for (int j = -1; j <= 1; j++) {
      int pos = pacmanPos + j;
      if (pos >= 0 && pos < NUM_LEDS) leds[pos] = CRGB::Yellow;
    }
    FastLED.show();
    delay(flickerDelay);
  }
}

void flickerGhostEaten() {
  const int flickerCount = 3;
  const int flickerDelay = 70;

  for (int i = 0; i < flickerCount; i++) {
    for (int j = -1; j <= 1; j++) {
      int pos = pacmanPos + j;
      if (pos >= 0 && pos < NUM_LEDS) leds[pos] = CRGB::Black;
    }
    FastLED.show();
    delay(flickerDelay);

    for (int j = -1; j <= 1; j++) {
      int pos = pacmanPos + j;
      if (pos >= 0 && pos < NUM_LEDS) leds[pos] = CRGB::Blue;
    }
    FastLED.show();
    delay(flickerDelay);
  }
}

void checkCollisions() {
  // Check coins collision with 1 pixel margin on each side
  for (int i = 0; i < MAX_COINS; i++) {
    if (coins[i] != -1) {
      // positions to check, with wrap-around
      int leftPos = (pacmanPos == 0) ? NUM_LEDS - 1 : pacmanPos - 1;
      int rightPos = (pacmanPos == NUM_LEDS - 1) ? 0 : pacmanPos + 1;

      if (coins[i] == pacmanPos || coins[i] == leftPos || coins[i] == rightPos) {
        pacmanSpeed = 100;
        powerMode = true;
        powerStart = millis();
        for (int j = 0; j < MAX_GHOSTS; j++) {
          ghosts[j].vulnerable = true;
        }
        clearCoin(i);
      }
    }
  }

  // Check ghosts collision with 1 pixel margin on each side
  for (int i = 0; i < MAX_GHOSTS; i++) {
    // positions to check, with wrap-around
    int leftPos = (pacmanPos == 0) ? NUM_LEDS - 1 : pacmanPos - 1;
    int rightPos = (pacmanPos == NUM_LEDS - 1) ? 0 : pacmanPos + 1;

    if (ghosts[i].pos == pacmanPos || ghosts[i].pos == leftPos || ghosts[i].pos == rightPos) {
      if (ghosts[i].vulnerable) {
        flickerGhostEaten();
        ghosts[i].pos = random(NUM_LEDS);
        ghosts[i].vulnerable = false;
      } else {
        flickerPacmanCollision();
        respawnPacman();
      }
    }
  }
}

void drawPacmanScene() {
  fadeAllLeds();

  // Draw ghosts
  for (int i = 0; i < MAX_GHOSTS; i++) {
    CRGB ghostColor = ghosts[i].vulnerable ? CRGB::Blue : ghosts[i].normalColor;
    leds[ghosts[i].pos] = ghostColor;

    int trailPos1 = ghosts[i].pos - ghosts[i].dir;
    int trailPos2 = ghosts[i].pos - 2 * ghosts[i].dir;
    if (trailPos1 >= 0 && trailPos1 < NUM_LEDS)
      leds[trailPos1] = ghostColor.nscale8(70);
    if (trailPos2 >= 0 && trailPos2 < NUM_LEDS)
      leds[trailPos2] = ghostColor.nscale8(30);
  }

  // Draw Pac-Man
  CRGB pacColor = (millis() / 150) % 2 ? CRGB::Yellow : CRGB(180, 180, 0);
  leds[pacmanPos] = pacColor;

  FastLED.show();
}

void updatePacmanEffect() {
  unsigned long now = millis();

  if (now - lastMoveTime >= pacmanSpeed) {
    lastMoveTime = now;

    pacmanPos += pacmanDir;
    if (pacmanPos >= NUM_LEDS) pacmanPos = 0;
    if (pacmanPos < 0) pacmanPos = NUM_LEDS - 1;

    updateGhosts();
    checkCollisions();

    if (!powerMode && now - lastCoinRespawn > coinRespawnInterval) {
      spawnCoins();
      lastCoinRespawn = now;
    }

    if (powerMode && now - powerStart > powerDuration) {
      powerMode = false;
      pacmanSpeed = 200;
      for (int i = 0; i < MAX_GHOSTS; i++) {
        ghosts[i].vulnerable = false;
      }
      lastCoinRespawn = now;
    }

    drawCoins();
    drawPacmanScene();
    FastLED.show();
  }
}

#endif
