#include "EffectRegistry.h"

#include <FastLED.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

struct Settings {
  uint8_t seeds = 8;
  uint8_t min_size = 6;
  uint8_t max_size = 18;
  uint8_t flicker = 50;
  uint8_t whitehot = 20;
  uint8_t bgEmber = 10;
  uint8_t speed = 40;
  uint8_t delay = 14;
  uint8_t glow = 70;
  uint8_t density = 40;
  uint8_t red = 255;
  uint8_t green = 77;
  uint8_t blue = 0;
  bool nightmode = false;
};

struct EmberParticle {
  int16_t pos = 0;
  float anchorPos = 0.0f;
  float centerPos = 0.0f;
  int16_t heat = 0;
  uint8_t targetHeat = 220;
  float size = 1.0f;
  float startSize = 1.0f;
  float peakSize = 2.0f;
  int32_t life = 100;
  int32_t maxLife = 100;
  int32_t age = 0;
  int16_t birthFrames = 30;
  float igniteRate = 4.0f;
  int16_t mergeBoostCooldown = 0;
  int16_t mergeBoostCount = 0;
  float lifeDrift = 1.0f;
  float stallChance = 0.06f;
  float burstChance = 0.1f;
  float burstStrength = 1.5f;
  float fadeAccel = 1.0f;
  float ripplePhase = 0.0f;
  float rippleRate = 0.05f;
  float rippleAmp = 1.0f;
  float driftDir = 1.0f;
  float driftSpan = 1.2f;
};

class EmberEffect : public IEffect {
 public:
  void begin(CRGB *leds, uint16_t count) override {
    (void)leds;
    particles_.clear();
    heatBuffer_.assign(count, 0);
    prevHeatBuffer_.assign(count, 0);
    frameCount_ = 0;
  }

  void render(const EffectContext &ctx, CRGB *leds, uint16_t count) override {
    if (heatBuffer_.size() != count) {
      heatBuffer_.assign(count, 0);
    }
    if (prevHeatBuffer_.size() != count) {
      prevHeatBuffer_.assign(count, 0);
    }
    std::fill(heatBuffer_.begin(), heatBuffer_.end(), 0);

    const float globalSpeed = std::max(0.25f, static_cast<float>(ctx.state.speed) / 100.0f);
    const float effectSpeed = std::max(0.25f, std::min(3.0f, static_cast<float>(settings_.speed) / 40.0f));
    const float speedScale = std::max(0.25f, std::min(3.0f, globalSpeed * effectSpeed));

    const int requestedParticles = static_cast<int>(settings_.seeds);
    const int largeStripCap = std::max(96, static_cast<int>(count) / 6);
    const int targetParticles = std::max(1, std::min(largeStripCap, requestedParticles));
    const int baseSpawnEvery = std::max(1, static_cast<int>(roundf(10.0f - (settings_.delay / 12.0f))));
    const int spawnEvery = std::max(1, static_cast<int>(roundf(baseSpawnEvery / std::max(0.35f, speedScale))));

    if ((frameCount_ % static_cast<uint32_t>(spawnEvery)) == 0) {
      const int deficit = std::max(0, targetParticles - static_cast<int>(particles_.size()));
      const int spawnBatch = std::min(12, std::max(1, 1 + (deficit / 20)));
      for (int i = 0; i < spawnBatch; ++i) {
        spawnParticle(count, targetParticles, false);
      }
    }

    std::vector<EmberParticle> updated;
    updated.reserve(particles_.size());
    for (auto &p : particles_) {
      if (updateParticle(p, speedScale, count)) {
        updated.push_back(p);
      }
    }
    particles_.swap(updated);

    // Refill immediately after die-off instead of waiting for the next spawn tick.
    if (static_cast<int>(particles_.size()) < targetParticles) {
      const int deficit = targetParticles - static_cast<int>(particles_.size());
      const int refillCount = std::min(18, std::max(1, deficit));
      for (int i = 0; i < refillCount; ++i) {
        spawnParticle(count, targetParticles, true);
      }
    }

    if (static_cast<int>(particles_.size()) > targetParticles) {
      std::sort(particles_.begin(), particles_.end(), [](const EmberParticle &a, const EmberParticle &b) {
        return a.heat > b.heat;
      });
      particles_.resize(targetParticles);
    }

    cohereParticles(speedScale, count, targetParticles);
    refillLargestGap(speedScale, count, targetParticles);

    for (const auto &p : particles_) {
      addGlow(heatBuffer_, p, count);
    }

    const uint8_t globalBrightness = ctx.state.brightness;
    const uint8_t keepPrev = static_cast<uint8_t>(constrain(static_cast<int>(200 - (speedScale * 28.0f)), 130, 210));
    const int maxRise = std::max(6, static_cast<int>(roundf(8.0f + (18.0f * speedScale))));
    const int maxFall = std::max(5, static_cast<int>(roundf(7.0f + (15.0f * speedScale))));
    for (uint16_t i = 0; i < count; ++i) {
      const uint8_t desired = qadd8(heatBuffer_[i], settings_.bgEmber);
      const uint8_t prev = prevHeatBuffer_[i];

      uint16_t blended = static_cast<uint16_t>((static_cast<uint16_t>(prev) * keepPrev) +
                                               (static_cast<uint16_t>(desired) * (255 - keepPrev)));
      uint8_t levelHeat = static_cast<uint8_t>(blended / 255);

      if (levelHeat > prev) {
        levelHeat = static_cast<uint8_t>(std::min<int>(levelHeat, static_cast<int>(prev) + maxRise));
      } else if (levelHeat < prev) {
        levelHeat = static_cast<uint8_t>(std::max<int>(levelHeat, static_cast<int>(prev) - maxFall));
      }

      heatBuffer_[i] = levelHeat;
      prevHeatBuffer_[i] = levelHeat;

      if (levelHeat == 0) {
        leds[i] = CRGB::Black;
        continue;
      }

      uint8_t flicker = 255;
      if (settings_.flicker > 0 && random8(100) < settings_.flicker) {
        flicker = random8(180, 255);
      }

      uint8_t level = scale8(levelHeat, flicker);
      if (settings_.nightmode) {
        // Keep ember structure visible in dark rooms even when global brightness is very low.
        const uint16_t boosted = static_cast<uint16_t>(level) * 3U;
        uint8_t perceived = static_cast<uint8_t>(std::min<uint16_t>(255, boosted));
        if (perceived > 0) {
          perceived = std::max<uint8_t>(perceived, 16);
        }
        level = perceived;
      } else {
        level = scale8(level, globalBrightness);
      }

      int16_t red = settings_.red;
      int16_t green = settings_.green;
      int16_t blue = settings_.blue;
      if (settings_.whitehot > 0) {
        const uint8_t whiteAmount = scale8(level, settings_.whitehot);
        red += whiteAmount;
        green += whiteAmount;
        blue += whiteAmount;
      }

      red = (red * level) / 255;
      green = (green * level) / 255;
      blue = (blue * level) / 255;

      if (settings_.nightmode) {
        const uint8_t nightCap = static_cast<uint8_t>(constrain(static_cast<int>(globalBrightness) + 20, 20, 60));
        red = scale8(static_cast<uint8_t>(constrain(red, 0, 255)), nightCap);
        green = scale8(static_cast<uint8_t>(constrain(green, 0, 255)), nightCap);
        blue = scale8(static_cast<uint8_t>(constrain(blue, 0, 255)), nightCap);
      }

      leds[i] = CRGB(
          static_cast<uint8_t>(constrain(red, 0, 255)),
          static_cast<uint8_t>(constrain(green, 0, 255)),
          static_cast<uint8_t>(constrain(blue, 0, 255)));
    }

    ++frameCount_;
  }

  String settingsSchemaJson() const override {
    return "["
           "{\"key\":\"seeds\",\"label\":\"Max Active Seeds\",\"type\":\"slider\",\"min\":1,\"max\":200,\"step\":1},"
           "{\"key\":\"min_size\",\"label\":\"Min Size\",\"type\":\"slider\",\"min\":1,\"max\":50,\"step\":1},"
           "{\"key\":\"max_size\",\"label\":\"Max Size\",\"type\":\"slider\",\"min\":1,\"max\":200,\"step\":1},"
           "{\"key\":\"flicker\",\"label\":\"Flicker\",\"type\":\"slider\",\"min\":0,\"max\":100,\"step\":1},"
           "{\"key\":\"whitehot\",\"label\":\"White Hot %\",\"type\":\"slider\",\"min\":0,\"max\":100,\"step\":1},"
           "{\"key\":\"bgEmber\",\"label\":\"BG Ember\",\"type\":\"slider\",\"min\":0,\"max\":100,\"step\":1},"
           "{\"key\":\"nightmode\",\"label\":\"Night Mode\",\"type\":\"toggle\"},"
           "{\"key\":\"speed\",\"label\":\"Speed\",\"type\":\"slider\",\"min\":1,\"max\":200,\"step\":1},"
           "{\"key\":\"delay\",\"label\":\"Delay\",\"type\":\"slider\",\"min\":0,\"max\":120,\"step\":1},"
           "{\"key\":\"glow\",\"label\":\"Glow\",\"type\":\"slider\",\"min\":10,\"max\":100,\"step\":1},"
           "{\"key\":\"density\",\"label\":\"Density\",\"type\":\"slider\",\"min\":5,\"max\":100,\"step\":1},"
           "{\"key\":\"red\",\"label\":\"Red\",\"type\":\"slider\",\"min\":0,\"max\":255,\"step\":1},"
           "{\"key\":\"green\",\"label\":\"Green\",\"type\":\"slider\",\"min\":0,\"max\":255,\"step\":1},"
           "{\"key\":\"blue\",\"label\":\"Blue\",\"type\":\"slider\",\"min\":0,\"max\":255,\"step\":1}"
           "]";
  }

  String settingsStateJson() const override {
    String out = "{";
    out += "\"seeds\":" + String(settings_.seeds);
    out += ",\"min_size\":" + String(settings_.min_size);
    out += ",\"max_size\":" + String(settings_.max_size);
    out += ",\"flicker\":" + String(settings_.flicker);
    out += ",\"whitehot\":" + String(settings_.whitehot);
    out += ",\"bgEmber\":" + String(settings_.bgEmber);
    out += ",\"nightmode\":" + String(settings_.nightmode ? "true" : "false");
    out += ",\"speed\":" + String(settings_.speed);
    out += ",\"delay\":" + String(settings_.delay);
    out += ",\"glow\":" + String(settings_.glow);
    out += ",\"density\":" + String(settings_.density);
    out += ",\"red\":" + String(settings_.red);
    out += ",\"green\":" + String(settings_.green);
    out += ",\"blue\":" + String(settings_.blue);
    out += "}";
    return out;
  }

  bool setSetting(const String &key, const String &value) override {
    const int intVal = value.toInt();

    if (key == "seeds") settings_.seeds = static_cast<uint8_t>(constrain(intVal, 1, 200));
    else if (key == "min_size") settings_.min_size = static_cast<uint8_t>(constrain(intVal, 1, 50));
    else if (key == "max_size") settings_.max_size = static_cast<uint8_t>(constrain(intVal, 1, 200));
    else if (key == "flicker") settings_.flicker = static_cast<uint8_t>(constrain(intVal, 0, 100));
    else if (key == "whitehot") settings_.whitehot = static_cast<uint8_t>(constrain(intVal, 0, 100));
    else if (key == "bgEmber") settings_.bgEmber = static_cast<uint8_t>(constrain(intVal, 0, 100));
    else if (key == "nightmode") settings_.nightmode = (value == "1" || value == "true" || value == "on");
    else if (key == "speed") settings_.speed = static_cast<uint8_t>(constrain(intVal, 1, 200));
    else if (key == "delay") settings_.delay = static_cast<uint8_t>(constrain(intVal, 0, 120));
    else if (key == "glow") settings_.glow = static_cast<uint8_t>(constrain(intVal, 10, 100));
    else if (key == "density") settings_.density = static_cast<uint8_t>(constrain(intVal, 5, 100));
    else if (key == "red") settings_.red = static_cast<uint8_t>(constrain(intVal, 0, 255));
    else if (key == "green") settings_.green = static_cast<uint8_t>(constrain(intVal, 0, 255));
    else if (key == "blue") settings_.blue = static_cast<uint8_t>(constrain(intVal, 0, 255));
    else return false;

    if (settings_.max_size < settings_.min_size) {
      settings_.max_size = settings_.min_size;
    }

    return true;
  }

  void resetSettings() override {
    settings_ = Settings();
  }

 private:
  static constexpr float kPi = 3.14159265f;
  static constexpr float kTau = kPi * 2.0f;

  Settings settings_;
  uint32_t frameCount_ = 0;
  std::vector<EmberParticle> particles_;
  std::vector<uint8_t> heatBuffer_;
  std::vector<uint8_t> prevHeatBuffer_;

  float wrapCenter(float value, uint16_t count) const {
    if (count == 0) {
      return 0.0f;
    }
    const float mod = fmodf(value, static_cast<float>(count));
    return mod < 0.0f ? (mod + static_cast<float>(count)) : mod;
  }

  float ringDistance(float a, float b, uint16_t count) const {
    if (count == 0) {
      return fabsf(a - b);
    }
    const float c = static_cast<float>(count);
    const float diff = fmodf(fabsf(a - b), c);
    return std::min(diff, c - diff);
  }

  float signedRingDelta(float src, float dst, uint16_t count) const {
    if (count == 0) {
      return dst - src;
    }
    const float c = static_cast<float>(count);
    float delta = fmodf(dst - src, c);
    if (delta < 0.0f) {
      delta += c;
    }
    if (delta > (c * 0.5f)) {
      delta -= c;
    }
    return delta;
  }

  int particleRadius(const EmberParticle &p) const {
    const float spread = ((p.size - 6.0f) * 5.0f / 12.0f) + 1.0f;
    const float glowStrength = ((static_cast<float>(settings_.glow) - 10.0f) * 7.0f / 90.0f) + 1.0f;
    const int radius = static_cast<int>(spread + (glowStrength * 0.5f));
    return std::max(1, radius);
  }

  EmberParticle buildSpawnParticle(uint16_t count, int selected, bool emergencySpawn) const {
    EmberParticle p;
    const int minSize = std::max(1, static_cast<int>(settings_.min_size));
    const int maxSize = std::max(minSize, static_cast<int>(settings_.max_size));

    p.pos = selected;
    p.anchorPos = static_cast<float>(selected);
    p.centerPos = static_cast<float>(selected);
    p.targetHeat = static_cast<uint8_t>(random(150, 256));
    p.heat = static_cast<int16_t>(emergencySpawn ? random(25, 71) : random(8, 41));
    p.peakSize = static_cast<float>(random(minSize, maxSize + 1));
    p.startSize = std::max(0.25f, static_cast<float>(minSize) * (0.15f + (random(0, 201) / 1000.0f)));
    p.size = p.startSize;
    p.life = static_cast<int32_t>(random(80, 341)) * 4;
    p.maxLife = p.life;
    p.birthFrames = static_cast<int16_t>(emergencySpawn ? random(16, 41) : random(20, 91));
    p.igniteRate = emergencySpawn ? (3.5f + (random(0, 401) / 100.0f)) : (2.0f + (random(0, 601) / 100.0f));
    p.lifeDrift = 0.65f + (random(0, 801) / 1000.0f);
    p.stallChance = 0.02f + (random(0, 101) / 1000.0f);
    p.burstChance = 0.04f + (random(0, 141) / 1000.0f);
    p.burstStrength = 1.0f + (random(0, 181) / 100.0f);
    p.fadeAccel = 0.85f + (random(0, 701) / 1000.0f);
    p.ripplePhase = random(0, 6284) / 1000.0f;
    p.rippleRate = 0.035f + (random(0, 551) / 10000.0f);
    p.rippleAmp = 0.6f + (random(0, 151) / 100.0f);
    p.driftDir = random8(2) == 0 ? -1.0f : 1.0f;
    p.driftSpan = 0.4f + (random(0, 181) / 100.0f);
    return p;
  }

  void spawnParticle(uint16_t count, int targetParticles, bool emergencySpawn) {
    if (count == 0 || static_cast<int>(particles_.size()) >= targetParticles) {
      return;
    }

    const int minGap = std::max(1, static_cast<int>((static_cast<float>(count) / std::max(1, targetParticles)) * 1.35f));

    int selected = random(count);
    if (!particles_.empty()) {
      float bestScore = -1.0f;
      for (int i = 0; i < 32; ++i) {
        const int candidate = random(count);
        float nearest = 1000000.0f;
        for (const auto &p : particles_) {
          nearest = std::min(nearest, ringDistance(static_cast<float>(candidate), p.anchorPos, count));
        }
        if (nearest > bestScore) {
          bestScore = nearest;
          selected = candidate;
        }
      }

      if (bestScore < static_cast<float>(minGap)) {
        for (int i = 0; i < 24; ++i) {
          const int candidate = random(count);
          bool farEnough = true;
          for (const auto &p : particles_) {
            if (ringDistance(static_cast<float>(candidate), p.anchorPos, count) < static_cast<float>(minGap)) {
              farEnough = false;
              break;
            }
          }
          if (farEnough) {
            selected = candidate;
            break;
          }
        }
      }
    }

    particles_.push_back(buildSpawnParticle(count, selected, emergencySpawn));
  }

  void refillLargestGap(float speedScale, uint16_t count, int targetParticles) {
    if (count < 8 || targetParticles <= 1 || particles_.size() < 2) {
      return;
    }

    const int gapCheckEvery = std::max(1, static_cast<int>(roundf(8.0f / std::max(0.35f, speedScale))));
    if ((frameCount_ % static_cast<uint32_t>(gapCheckEvery)) != 0) {
      return;
    }

    float largestGap = 0.0f;
    float gapMidpoint = 0.0f;
    for (size_t i = 0; i < particles_.size(); ++i) {
      const EmberParticle &a = particles_[i];
      const EmberParticle &b = particles_[(i + 1) % particles_.size()];
      const float forward = wrapCenter(b.anchorPos - a.anchorPos, count);
      if (forward > largestGap) {
        largestGap = forward;
        gapMidpoint = wrapCenter(a.anchorPos + (forward * 0.5f), count);
      }
    }

    const float expectedSpacing = static_cast<float>(count) / static_cast<float>(std::max(1, targetParticles));
    const float voidThreshold = expectedSpacing * 2.6f;
    if (largestGap < voidThreshold) {
      return;
    }

    const int spawnPos = static_cast<int>(roundf(gapMidpoint)) % std::max<uint16_t>(1, count);
    EmberParticle newcomer = buildSpawnParticle(count, spawnPos, true);

    if (static_cast<int>(particles_.size()) >= targetParticles) {
      auto weakest = std::min_element(
          particles_.begin(), particles_.end(),
          [](const EmberParticle &lhs, const EmberParticle &rhs) { return lhs.heat < rhs.heat; });
      if (weakest != particles_.end()) {
        // Rejuvenate a weak ember toward the gap instead of hard-replacing it.
        const float pull = 0.45f;
        weakest->anchorPos = wrapCenter(weakest->anchorPos + (signedRingDelta(weakest->anchorPos, newcomer.anchorPos, count) * pull), count);
        weakest->centerPos = weakest->anchorPos;
        weakest->pos = static_cast<int16_t>(roundf(weakest->centerPos)) % std::max<uint16_t>(1, count);
        weakest->life = std::max<int32_t>(weakest->life, newcomer.life / 2);
        weakest->maxLife = std::max<int32_t>(weakest->maxLife, newcomer.maxLife / 2);
        weakest->heat = std::max<int16_t>(weakest->heat, newcomer.heat);
        weakest->targetHeat = std::max<uint8_t>(weakest->targetHeat, newcomer.targetHeat);
        weakest->birthFrames = std::max<int16_t>(weakest->birthFrames, newcomer.birthFrames / 2);
      }
      return;
    }

    particles_.push_back(newcomer);
  }

  bool updateParticle(EmberParticle &p, float speedScale, uint16_t count) {
    if (p.life <= 0) {
      return false;
    }

    ++p.age;
    if (p.mergeBoostCooldown > 0) {
      --p.mergeBoostCooldown;
    }

    const float speedDecayFactor = 0.18f + (1.05f * speedScale);
    int lifeDecay = std::max(1, static_cast<int>(roundf(speedDecayFactor * p.lifeDrift)));
    if ((random(0, 10000) / 10000.0f) < p.stallChance) {
      lifeDecay = std::max(0, lifeDecay - 1);
    }
    if ((random(0, 10000) / 10000.0f) < p.burstChance) {
      lifeDecay += static_cast<int>(roundf(p.burstStrength * 0.5f));
    }

    p.life = std::max(0, p.life - lifeDecay - (random8(100) < 8 ? 1 : 0));

    const float lifeRatio = static_cast<float>(p.life) / std::max(1.0f, static_cast<float>(p.maxLife));
    const float birthRatio = std::min(1.0f, static_cast<float>(p.age) / std::max(1.0f, static_cast<float>(p.birthFrames)));

    float growth;
    if (lifeRatio > 0.55f) {
      growth = (1.0f - lifeRatio) / 0.45f;
    } else {
      growth = std::max(0.0f, lifeRatio / 0.55f);
    }
    growth *= 0.35f + (0.65f * birthRatio);

    p.size = std::max(1.0f, p.startSize + ((p.peakSize - p.startSize) * growth));

    const float progress = 1.0f - lifeRatio;
    const float motionSpeed = 0.55f + (0.65f * speedScale);
    const float phase = (frameCount_ * p.rippleRate * motionSpeed) + p.ripplePhase;
    const float peakForMotion = std::max(1.0f, p.peakSize);
    const float sizeRatio = std::max(0.0f, std::min(1.0f, p.size / peakForMotion));
    const float motionScale = 0.12f + ((sizeRatio * sizeRatio) * 0.88f);

    const float ripple = sinf(phase) * p.rippleAmp * motionScale;
    const float drift = p.driftDir * p.driftSpan * (progress * progress) * motionScale * motionSpeed;
    p.centerPos = p.anchorPos + ripple + drift;
    p.centerPos = wrapCenter(p.centerPos, count);
    p.pos = static_cast<int16_t>(roundf(p.centerPos)) % std::max<uint16_t>(1, count);

    if (p.heat < p.targetHeat && birthRatio < 1.0f) {
      p.heat = std::min<int16_t>(
          static_cast<int16_t>(p.targetHeat),
          static_cast<int16_t>(p.heat + (p.igniteRate * (0.5f + (0.5f * birthRatio)))));
    }

    int heatDrop = std::max(
        1,
      static_cast<int>(roundf((0.45f + (0.85f * speedScale) - (lifeRatio * 0.30f)) * p.fadeAccel)));
    if ((random(0, 10000) / 10000.0f) < (p.stallChance * 0.5f)) {
      heatDrop = std::max(1, heatDrop - 1);
    }

    p.heat = std::max<int16_t>(0, p.heat - heatDrop - (random8(100) < 5 ? 1 : 0));

    return p.life > 0 && p.heat > 0 && p.size >= 1.0f;
  }

  void cohereParticles(float speedScale, uint16_t count, int targetParticles) {
    if (particles_.size() < 2 || count == 0) {
      return;
    }

    const float maxSize = static_cast<float>(settings_.max_size);

    for (size_t i = 0; i + 1 < particles_.size(); ++i) {
      EmberParticle &left = particles_[i];
      float leftCenter = left.centerPos;
      const int leftRadius = particleRadius(left);

      for (size_t j = i + 1; j < particles_.size(); ++j) {
        EmberParticle &right = particles_[j];
        float rightCenter = right.centerPos;
        const int rightRadius = particleRadius(right);

        const float joinDistance = static_cast<float>(leftRadius + rightRadius);
        const float distance = ringDistance(rightCenter, leftCenter, count);
        if (distance > joinDistance) {
          continue;
        }

        const float closeness = 1.0f - (distance / std::max(0.001f, joinDistance));

        const float toRight = signedRingDelta(leftCenter, rightCenter, count);
        const float midpoint = wrapCenter(leftCenter + (toRight * 0.5f), count);
        const float pull = (0.05f + (0.18f * closeness)) * std::min(1.0f, std::max(0.35f, speedScale / 2.0f));

        leftCenter = wrapCenter(leftCenter + (signedRingDelta(leftCenter, midpoint, count) * pull), count);
        rightCenter = wrapCenter(rightCenter + (signedRingDelta(rightCenter, midpoint, count) * pull), count);

        left.centerPos = leftCenter;
        right.centerPos = rightCenter;

        const float heatGap = static_cast<float>(left.heat - right.heat);
        const int transfer = static_cast<int>(fabsf(heatGap) * (0.06f + (0.12f * closeness)));
        if (transfer > 0) {
          if (heatGap > 0) {
            left.heat = std::max<int16_t>(0, left.heat - transfer);
            right.heat = std::min<int16_t>(255, right.heat + transfer);
          } else {
            right.heat = std::max<int16_t>(0, right.heat - transfer);
            left.heat = std::min<int16_t>(255, left.heat + transfer);
          }
        }

        const float growthBonus = 0.04f + (0.22f * closeness);
        left.peakSize = std::min(maxSize, left.peakSize + growthBonus);
        right.peakSize = std::min(maxSize, right.peakSize + growthBonus);

        const bool canAbsorb = static_cast<int>(particles_.size()) > std::max(6, static_cast<int>(targetParticles * 0.55f));
        if (canAbsorb && distance < std::max(0.8f, std::min(leftRadius, rightRadius) * 0.22f)) {
          EmberParticle *dominant = left.heat >= right.heat ? &left : &right;
          EmberParticle *weaker = left.heat >= right.heat ? &right : &left;
          const int siphon = std::max(1, static_cast<int>((0.02f + (0.09f * closeness)) * weaker->heat));

          weaker->heat = std::max<int16_t>(0, weaker->heat - siphon);
          dominant->heat = std::min<int16_t>(255, dominant->heat + static_cast<int>(siphon * 0.4f));
          weaker->life = std::max<int32_t>(0, weaker->life - std::max(1, static_cast<int>(speedScale * (0.12f + (0.22f * closeness)))));

          if (dominant->mergeBoostCooldown <= 0) {
            const float lifeFactor = 2.0f + (random(0, 501) / 100.0f);
            dominant->life = std::min<int32_t>(20000, static_cast<int32_t>(std::max(1.0f, static_cast<float>(dominant->life)) * lifeFactor));
            dominant->maxLife = std::min<int32_t>(20000, static_cast<int32_t>(std::max(1.0f, static_cast<float>(dominant->maxLife)) * lifeFactor));
            dominant->mergeBoostCooldown = 60;
            ++dominant->mergeBoostCount;
          }
        }
      }
    }

    for (auto &p : particles_) {
      p.centerPos = wrapCenter(p.centerPos, count);
      p.pos = static_cast<int16_t>(roundf(p.centerPos)) % std::max<uint16_t>(1, count);
    }
  }

  void addGlow(std::vector<uint8_t> &buffer, const EmberParticle &p, uint16_t count) {
    if (count == 0 || p.life <= 0) {
      return;
    }

    const int radius = particleRadius(p);
    const float center = wrapCenter(p.centerPos, count);
    const float birthT = std::min(1.0f, static_cast<float>(p.age) / std::max(1.0f, static_cast<float>(p.birthFrames)));
    const float birthEase = birthT * birthT * (3.0f - (2.0f * birthT));
    const int centerIdx = static_cast<int>(roundf(center));

    for (int offset = -radius; offset <= radius; ++offset) {
      const int idx = (centerIdx + offset + count) % std::max<uint16_t>(1, count);
      const int dist = abs(offset);
      const uint8_t falloff = dist >= radius ? 0 : static_cast<uint8_t>(255 - ((dist * 255) / (radius + 1)));
      int amount = static_cast<int>((p.heat * falloff / 255.0f) * birthEase);

      if (p.life < (p.maxLife / 3)) {
        const int fade = std::max(1, static_cast<int>(p.maxLife / 3));
        amount = (amount * std::max(0, std::min(255, static_cast<int>((p.life * 255.0f) / fade)))) / 255;
      }

      buffer[idx] = qadd8(buffer[idx], static_cast<uint8_t>(std::max(0, std::min(255, amount))));
    }
  }
};

REGISTER_EFFECT(EmberEffect, 52, "Ember");

}  // namespace
