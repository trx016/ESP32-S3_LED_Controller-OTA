#include "EffectRegistry.h"
#include <vector>
#include <algorithm>

namespace {

struct Settings {
  uint8_t speed = 40;
  uint8_t delay = 14;
  uint8_t glow = 70;
  uint8_t density = 40;
  uint32_t color = 0xFF4D00;  // #ff4d00
  uint8_t seeds = 8;
  uint8_t min_size = 6;
  uint8_t max_size = 18;
  uint8_t flicker = 50;
  uint8_t whitehot = 20;
  uint8_t bgEmber = 10;
  uint8_t brightness = 100;
};

struct EmberParticle {
  int16_t pos = 0;
  int8_t velocity = 0;
  uint8_t heat = 0;
  uint8_t size = 1;
  uint8_t life = 0;
  uint8_t max_life = 0;
};

class LivingEffect : public IEffect {
 private:
  Settings settings;
  uint32_t frame_count = 0;
  std::vector<EmberParticle> particles;
  std::vector<uint8_t> heat_buffer;

  void spawnParticle(uint16_t count, uint8_t target_particles) {
    if (particles.size() >= target_particles) {
      return;
    }

    EmberParticle p;
    p.pos = count > 0 ? random(count) : 0;
    if (!particles.empty()) {
      const EmberParticle &anchor = particles[random8(particles.size())];
      p.pos = constrain(anchor.pos + random8(41) - 20, 0, (int)count - 1);
    }

    p.velocity = random8(3) - 1;
    p.heat = random8(120, 255);
    p.size = random8(settings.min_size, settings.max_size + 1);
    p.life = random8(70, 220);
    p.max_life = p.life;
    particles.push_back(p);
  }

  void updateParticle(EmberParticle &p, uint16_t count) {
    if (p.life == 0) {
      return;
    }

    p.life = qsub8(p.life, 1 + (random8(100) < 20));
    p.heat = qsub8(p.heat, 1 + (random8(100) < 25));
    if (random8(100) < 12) {
      p.size = (p.size > 1) ? (p.size - 1) : 1;
    }

    if (random8(100) < 15) {
      p.velocity += (random8(2) == 0) ? -1 : 1;
    }
    p.velocity = constrain(p.velocity, -2, 2);

    p.pos += p.velocity;
    if (p.pos < 0) p.pos = 0;
    if (p.pos >= (int16_t)count) p.pos = count - 1;
  }

  void addParticleGlow(std::vector<uint8_t> &buffer, const EmberParticle &p, uint16_t count) {
    if (p.life == 0) {
      return;
    }

    uint8_t spread = map(p.size, settings.min_size, settings.max_size, 1, 6);
    uint8_t glow_strength = map(settings.glow, 10, 100, 1, 8);
    int16_t radius = spread + glow_strength / 2;
    if (radius < 1) radius = 1;

    int16_t start = std::max(0, p.pos - radius);
    int16_t end = std::min((int16_t)count - 1, p.pos + radius);
    for (int16_t i = start; i <= end; ++i) {
      int16_t dist = abs(i - p.pos);
      uint8_t falloff = (dist >= radius) ? 0 : (uint8_t)(255 - (dist * 255 / (radius + 1)));
      uint8_t amount = scale8(p.heat, falloff);
      if (p.life < p.max_life / 3) {
        uint8_t fade = p.max_life / 3;
        if (fade == 0) fade = 1;
        amount = scale8(amount, scale8(p.life * 255 / fade, 255));
      }
      buffer[i] = qadd8(buffer[i], amount);
    }
  }

 public:
  void begin(CRGB *leds, uint16_t count) override {
    (void)leds;
    heat_buffer.assign(count, 0);
    particles.clear();
    frame_count = 0;
  }

  void render(const EffectContext &ctx, CRGB *leds, uint16_t count) override {
    uint16_t led_count = count;
    if (heat_buffer.size() != led_count) {
      heat_buffer.assign(led_count, 0);
    }
    std::fill(heat_buffer.begin(), heat_buffer.end(), 0);

    const uint8_t target_particles = constrain((settings.seeds * 2) + (settings.density / 8), 4, 96);
    const uint8_t spawn_every = constrain(10 - (settings.delay / 12), 2, 10);

    if ((frame_count % spawn_every) == 0) {
      spawnParticle(led_count, target_particles);
    }

    for (size_t i = 0; i < particles.size();) {
      EmberParticle &p = particles[i];
      updateParticle(p, led_count);
      if (p.life == 0) {
        particles.erase(particles.begin() + i);
      } else {
        addParticleGlow(heat_buffer, p, led_count);
        ++i;
      }
    }

    for (uint16_t i = 0; i < led_count; ++i) {
      heat_buffer[i] = qadd8(heat_buffer[i], settings.bgEmber);
      if (heat_buffer[i] > 0) {
        uint8_t flicker = 255;
        if (settings.flicker > 0 && random8(100) < settings.flicker) {
          flicker = random8(180, 255);
        }
        uint8_t level = scale8(heat_buffer[i], flicker);
        level = scale8(level, settings.brightness);

        CRGB base = CRGB(
          (settings.color >> 16) & 0xFF,
          (settings.color >> 8) & 0xFF,
          settings.color & 0xFF);

        if (settings.whitehot > 0) {
          uint8_t white_amount = scale8(level, settings.whitehot);
          base += CRGB(white_amount, white_amount, white_amount);
        }

        base.nscale8(level);
        base.nscale8(ctx.state.brightness);
        leds[i] = base;
      } else {
        leds[i] = CRGB::Black;
      }
    }

    frame_count++;
  }

  String settingsSchemaJson() const override {
    return R"([{"key":"seeds","label":"Seeds","type":"slider","min":0,"max":50,"step":1},{"key":"min_size","label":"Min Size","type":"slider","min":1,"max":50,"step":1},{"key":"max_size","label":"Max Size","type":"slider","min":1,"max":50,"step":1},{"key":"flicker","label":"Flicker","type":"slider","min":0,"max":100,"step":1},{"key":"whitehot","label":"White Hot %","type":"slider","min":0,"max":100,"step":1},{"key":"bgEmber","label":"BG Ember","type":"slider","min":0,"max":100,"step":1},{"key":"brightness","label":"Brightness","type":"slider","min":0,"max":100,"step":1},{"key":"speed","label":"Speed","type":"slider","min":1,"max":200,"step":1},{"key":"delay","label":"Delay","type":"slider","min":0,"max":120,"step":1},{"key":"glow","label":"Glow","type":"slider","min":10,"max":100,"step":1},{"key":"density","label":"Density","type":"slider","min":5,"max":100,"step":1},{"key":"color","label":"Color","type":"color"}])";
  }

  String settingsStateJson() const override {
    char buffer[512];
    snprintf(buffer, sizeof(buffer),
      R"({"settings": {}})");
    return String(buffer);
  }

  bool setSetting(const String &key, const String &value) override {
    int int_val = value.toInt();

    if (false) {}
    else if (key == "speed") settings.speed = constrain(int_val, 0, 255);
    else if (key == "delay") settings.delay = constrain(int_val, 0, 255);
    else if (key == "glow") settings.glow = constrain(int_val, 0, 255);
    else if (key == "density") settings.density = constrain(int_val, 0, 255);
    else if (key == "color") {
      if (value.startsWith("#") && value.length() == 7) {
        settings.color = strtol(value.c_str() + 1, nullptr, 16);
      }
    }
    else if (key == "seeds") settings.seeds = constrain(int_val, 0, 255);
    else if (key == "min_size") settings.min_size = constrain(int_val, 0, 255);
    else if (key == "max_size") settings.max_size = constrain(int_val, 0, 255);
    else if (key == "flicker") settings.flicker = constrain(int_val, 0, 255);
    else if (key == "whitehot") settings.whitehot = constrain(int_val, 0, 255);
    else if (key == "bgEmber") settings.bgEmber = constrain(int_val, 0, 255);
    else if (key == "brightness") settings.brightness = constrain(int_val, 0, 255);
    else {
      return false;
    }
    return true;
  }

  void resetSettings() override {
    settings = Settings();
  }
};

}  // namespace

REGISTER_EFFECT(LivingEffect, 51, "Living");
