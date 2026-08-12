from __future__ import annotations

import math
import random
from typing import List, Tuple

from effect_runtime import EffectBase, EffectContext


class CollideEffect(EffectBase):
    def __init__(self) -> None:
        self.led_count = 120
        self._energy: List[float] = []
        self._active = False
        self._exploding = False
        self._cooldown_ms = 0
        self._explosion_age_ms = 0
        self._explosion_center = 0.0
        self._a = 0.0
        self._b = 0.0
        self._va = 0.0
        self._vb = 0.0
        self._hue_a = random.randint(0, 255)
        self._hue_b = random.randint(0, 255)
        self._hue_boom = random.randint(0, 255)
        self._hue_phase = 0.0
        self._burst_particles: List[dict] = []

    def editor_settings_schema(self):
        return [
            {"key": "speedMin", "label": "Speed Min", "type": "slider", "min": 4, "max": 120, "step": 1, "default": 28},
            {"key": "speedMax", "label": "Speed Max", "type": "slider", "min": 4, "max": 180, "step": 1, "default": 86},
            {"key": "impactRadius", "label": "Impact Radius", "type": "slider", "min": 4, "max": 2000, "step": 1, "default": 24},
            {"key": "impactDurationMs", "label": "Impact Hold ms", "type": "slider", "min": 80, "max": 1200, "step": 10, "default": 320},
            {"key": "particleCount", "label": "Particle Count", "type": "slider", "min": 4, "max": 40, "step": 1, "default": 18},
            {"key": "burstSpeedSpread", "label": "Burst Speed Spread", "type": "slider", "min": 20, "max": 260, "step": 1, "default": 160},
            {"key": "saturation", "label": "Saturation", "type": "slider", "min": 120, "max": 255, "step": 1, "default": 255},
            {"key": "hueDrift", "label": "Hue Drift", "type": "slider", "min": 0, "max": 80, "step": 1, "default": 26},
            {"key": "trailHold", "label": "Trail Hold", "type": "slider", "min": 45, "max": 92, "step": 1, "default": 68},
            {"key": "glowWidth", "label": "Glow Width", "type": "slider", "min": 6, "max": 22, "step": 1, "default": 11},
            {"key": "burstBoost", "label": "Burst Boost", "type": "slider", "min": 80, "max": 220, "step": 1, "default": 145},
        ]

    def render(self, ctx: EffectContext) -> List[Tuple[int, int, int]]:
        if len(self._energy) != ctx.led_count:
            self._energy = [0.0] * ctx.led_count
            self._active = False
            self._exploding = False
            self._cooldown_ms = 0
            self._explosion_age_ms = 0
            self._burst_particles = []

        dt_ms = max(1, int(ctx.delta_ms) if ctx.delta_ms > 0 else 33)
        speed_scale = max(0.2, min(3.0, float(ctx.speed_norm) * 2.0))
        dt_sec = (dt_ms / 1000.0) * speed_scale
        hue_drift = max(0.0, min(80.0, float(ctx.effect_state.get("hueDrift", 26.0))))
        self._hue_phase = (self._hue_phase + (dt_ms * (0.08 + (hue_drift / 220.0)) * speed_scale)) % 256.0

        target = [0.0] * ctx.led_count
        hue_sum = [0.0] * ctx.led_count
        hue_weight = [0.0] * ctx.led_count

        if not self._active and not self._exploding:
            self._cooldown_ms -= max(1, int(dt_ms * speed_scale))
            if self._cooldown_ms <= 0:
                self._spawn_pair(ctx.led_count, ctx.effect_state)

        if self._active:
            self._a = self._wrap(self._a + self._va * dt_sec, ctx.led_count)
            self._b = self._wrap(self._b + self._vb * dt_sec, ctx.led_count)

            glow_sigma = max(0.6, min(2.2, float(ctx.effect_state.get("glowWidth", 11.0)) / 10.0))
            self._add_colored_spark(target, hue_sum, hue_weight, self._a, self._hue_a, 1.0, glow_sigma, ctx.led_count)
            self._add_colored_spark(target, hue_sum, hue_weight, self._b, self._hue_b, 1.0, glow_sigma, ctx.led_count)

            dist = self._ring_distance(self._a, self._b, ctx.led_count)
            if dist <= max(1.0, (abs(self._va) + abs(self._vb)) * dt_sec * 0.8):
                self._active = False
                self._exploding = True
                self._explosion_age_ms = 0
                toward = self._signed_delta(self._a, self._b, ctx.led_count) * 0.5
                self._explosion_center = self._wrap(self._a + toward, ctx.led_count)
                self._hue_boom = int(((self._hue_a + self._hue_b) * 0.5) + random.randint(0, 255)) & 0xFF
                self._spawn_burst(self._explosion_center, ctx.led_count, ctx.effect_state)

        if self._exploding:
            impact_duration = max(80, int(ctx.effect_state.get("impactDurationMs", 320)))
            burst_boost = max(0.8, min(2.2, float(ctx.effect_state.get("burstBoost", 145.0)) / 100.0))
            base_sigma = max(0.6, min(2.2, float(ctx.effect_state.get("glowWidth", 11.0)) / 10.0))
            impact_radius = max(4.0, min(float(ctx.led_count), float(ctx.effect_state.get("impactRadius", 24))))
            survivors: List[dict] = []
            for particle in self._burst_particles:
                particle["age_ms"] += max(1.0, dt_ms * speed_scale)
                life_ms = max(60.0, float(particle["life_ms"]))
                if particle["age_ms"] >= life_ms:
                    continue

                t = max(0.0, min(1.0, particle["age_ms"] / life_ms))
                particle["pos"] = self._wrap(particle["pos"] + (particle["vel"] * dt_sec), ctx.led_count)

                envelope = (max(0.0, 1.0 - t) ** 0.72) * burst_boost
                sigma = base_sigma * particle["size"] * (0.8 + 0.9 * (1.0 - t))
                sigma = max(0.35, min(impact_radius * 0.25, sigma))
                self._add_colored_spark(
                    target,
                    hue_sum,
                    hue_weight,
                    particle["pos"],
                    int(particle["hue"]) & 0xFF,
                    envelope * float(particle.get("intensity", 1.0)),
                    sigma,
                    ctx.led_count,
                )
                survivors.append(particle)

            self._burst_particles = survivors
            self._explosion_age_ms += max(1, int(dt_ms * speed_scale))
            if len(self._burst_particles) == 0 or self._explosion_age_ms > max(5000, impact_duration * 6):
                self._exploding = False
                self._cooldown_ms = random.randint(120, 580)
                self._burst_particles = []

        pixels: List[Tuple[int, int, int]] = []
        brightness = max(0.0, min(1.0, float(ctx.brightness_norm)))
        trail_hold = max(0.45, min(0.92, float(ctx.effect_state.get("trailHold", 68.0)) / 100.0))
        glow_sigma = max(0.6, min(2.2, float(ctx.effect_state.get("glowWidth", 11.0)) / 10.0))
        saturation = max(120, min(255, int(ctx.effect_state.get("saturation", 255))))
        for i in range(ctx.led_count):
            e = (self._energy[i] * trail_hold) + (target[i] * (1.0 - trail_hold))
            if target[i] <= 0.0005 and e < 0.006:
                e = 0.0
            self._energy[i] = max(0.0, min(1.0, e))

            value = int(255 * (self._energy[i] ** 0.78) * brightness)
            value = max(0, min(255, value))
            total = hue_weight[i]
            if value == 0 or total <= 0.0001:
                pixels.append((0, 0, 0))
                continue

            fallback_hue = (int((self._hue_a + self._hue_b) * 0.5) + int(self._hue_phase * 0.5)) & 0xFF
            hue = (hue_sum[i] / total) if total > 0.0 else float(fallback_hue)
            r, g, b = self._hsv_to_rgb(int(hue) & 0xFF, saturation, value)
            pixels.append((r, g, b))

        return pixels

    def _spawn_pair(self, count: int, effect_state: dict) -> None:
        if count < 2:
            return

        speed_min = max(4.0, min(120.0, float(effect_state.get("speedMin", 24))))
        speed_max = max(speed_min, min(160.0, float(effect_state.get("speedMax", 68))))

        base = random.randint(0, max(0, count - 1))
        min_sep = max(2, count // 5)
        max_sep = max(min_sep + 1, (count * 4) // 5)
        sep = random.randint(min_sep, max_sep)

        self._a = self._wrap(base - (sep * 0.5), count)
        self._b = self._wrap(base + (sep * 0.5), count)

        toward = self._signed_delta(self._a, self._b, count)
        dir_a = 1.0 if toward >= 0.0 else -1.0
        dir_b = -dir_a

        self._va = dir_a * random.uniform(speed_min, speed_max)
        self._vb = dir_b * random.uniform(speed_min, speed_max)
        self._hue_a = random.randint(0, 255)
        self._hue_b = (self._hue_a + random.randint(70, 190)) & 0xFF

        self._active = True
        self._exploding = False
        self._explosion_age_ms = 0
        self._burst_particles = []

    def _spawn_burst(self, center: float, count: int, effect_state: dict) -> None:
        particle_count = max(4, min(40, int(effect_state.get("particleCount", 18))))
        speed_spread = max(0.2, min(2.6, float(effect_state.get("burstSpeedSpread", 160.0)) / 100.0))
        life_ms = max(80.0, float(effect_state.get("impactDurationMs", 320.0)))
        impact_radius = max(4.0, min(float(count), float(effect_state.get("impactRadius", 24.0))))

        min_mul = max(0.18, 1.0 - (speed_spread * 0.50))
        max_mul = 1.0 + speed_spread

        particles: List[dict] = []
        for idx in range(particle_count):
            direction = -1.0 if idx % 2 == 0 else 1.0
            direction *= random.choice([1.0, 1.0, 1.0, -1.0])

            travel_goal = impact_radius * random.uniform(0.40, 1.00)
            particle_life = life_ms * random.uniform(0.62, 1.12)
            base_speed = travel_goal / max(0.06, particle_life / 1000.0)
            speed = base_speed * random.uniform(min_mul, max_mul)
            hue = (self._hue_boom + random.randint(-22, 22)) & 0xFF
            particles.append(
                {
                    "pos": center,
                    "vel": direction * speed,
                    "age_ms": 0.0,
                    "life_ms": particle_life,
                    "hue": hue,
                    "size": random.uniform(0.65, 1.55),
                    "intensity": 1.0,
                }
            )

        spark_count = max(2, particle_count // 3)
        for _ in range(spark_count):
            direction = random.choice([-1.0, 1.0])
            travel_goal = impact_radius * random.uniform(0.35, 1.10)
            particle_life = life_ms * random.uniform(0.30, 0.72)
            base_speed = travel_goal / max(0.04, particle_life / 1000.0)
            speed = base_speed * random.uniform(0.95, 1.55 + (speed_spread * 0.25))
            particles.append(
                {
                    "pos": center,
                    "vel": direction * speed,
                    "age_ms": 0.0,
                    "life_ms": particle_life,
                    "hue": random.randint(0, 255),
                    "size": random.uniform(0.16, 0.34),
                    "intensity": random.uniform(1.15, 1.55),
                }
            )

        self._burst_particles = particles

    def _add_colored_spark(
        self,
        target: List[float],
        hue_sum: List[float],
        hue_weight: List[float],
        center: float,
        hue: int,
        intensity: float,
        sigma: float,
        count: int,
    ) -> None:
        if intensity <= 0.0:
            return
        for i in range(count):
            d = self._ring_distance(float(i), center, count)
            glow = self._gaussian_glow(d, sigma)
            amount = glow * intensity
            target[i] = max(target[i], amount)
            hue_sum[i] += amount * float(hue)
            hue_weight[i] += amount

    @staticmethod
    def _gaussian_glow(distance: float, sigma: float) -> float:
        x = distance / max(0.3, sigma)
        return math.exp(-0.5 * x * x)

    @staticmethod
    def _hsv_to_rgb(h: int, s: int, v: int) -> Tuple[int, int, int]:
        h = float(h & 0xFF) / 255.0
        s = max(0.0, min(1.0, float(s) / 255.0))
        v = max(0.0, min(1.0, float(v) / 255.0))

        i = int(h * 6.0)
        f = (h * 6.0) - i
        p = v * (1.0 - s)
        q = v * (1.0 - f * s)
        t = v * (1.0 - (1.0 - f) * s)
        i = i % 6

        if i == 0:
            r, g, b = v, t, p
        elif i == 1:
            r, g, b = q, v, p
        elif i == 2:
            r, g, b = p, v, t
        elif i == 3:
            r, g, b = p, q, v
        elif i == 4:
            r, g, b = t, p, v
        else:
            r, g, b = v, p, q

        return int(r * 255), int(g * 255), int(b * 255)

    @staticmethod
    def _wrap(value: float, count: int) -> float:
        if count <= 0:
            return 0.0
        out = value % float(count)
        if out < 0.0:
            out += float(count)
        return out

    @staticmethod
    def _ring_distance(a: float, b: float, count: int) -> float:
        if count <= 0:
            return abs(a - b)
        d = abs(a - b) % float(count)
        return min(d, float(count) - d)

    @staticmethod
    def _signed_delta(src: float, dst: float, count: int) -> float:
        if count <= 0:
            return dst - src
        delta = (dst - src) % float(count)
        if delta > (count * 0.5):
            delta -= float(count)
        return delta
