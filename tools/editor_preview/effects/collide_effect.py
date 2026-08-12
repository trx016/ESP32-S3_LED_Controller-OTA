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

    def editor_settings_schema(self):
        return [
            {"key": "speedMin", "label": "Speed Min", "type": "slider", "min": 4, "max": 120, "step": 1, "default": 24},
            {"key": "speedMax", "label": "Speed Max", "type": "slider", "min": 4, "max": 160, "step": 1, "default": 68},
            {"key": "impactRadius", "label": "Impact Radius", "type": "slider", "min": 4, "max": 80, "step": 1, "default": 16},
            {"key": "impactDurationMs", "label": "Impact Hold ms", "type": "slider", "min": 120, "max": 900, "step": 10, "default": 260},
        ]

    def render(self, ctx: EffectContext) -> List[Tuple[int, int, int]]:
        if len(self._energy) != ctx.led_count:
            self._energy = [0.0] * ctx.led_count
            self._active = False
            self._exploding = False
            self._cooldown_ms = 0
            self._explosion_age_ms = 0

        dt_ms = max(1, int(ctx.delta_ms) if ctx.delta_ms > 0 else 33)
        speed_scale = max(0.2, min(3.0, float(ctx.speed_norm) * 2.0))
        dt_sec = (dt_ms / 1000.0) * speed_scale
        self._hue_phase = (self._hue_phase + (dt_ms * 0.2 * speed_scale)) % 256.0

        target = [0.0] * ctx.led_count
        boom_radius = 1.0
        boom_t = 1.0

        if not self._active and not self._exploding:
            self._cooldown_ms -= max(1, int(dt_ms * speed_scale))
            if self._cooldown_ms <= 0:
                self._spawn_pair(ctx.led_count, ctx.effect_state)

        if self._active:
            self._a = self._wrap(self._a + self._va * dt_sec, ctx.led_count)
            self._b = self._wrap(self._b + self._vb * dt_sec, ctx.led_count)
            self._add_spark(target, self._a, ctx.led_count)
            self._add_spark(target, self._b, ctx.led_count)

            dist = self._ring_distance(self._a, self._b, ctx.led_count)
            if dist <= max(1.0, (abs(self._va) + abs(self._vb)) * dt_sec * 0.8):
                self._active = False
                self._exploding = True
                self._explosion_age_ms = 0
                toward = self._signed_delta(self._a, self._b, ctx.led_count) * 0.5
                self._explosion_center = self._wrap(self._a + toward, ctx.led_count)
                self._hue_boom = int(((self._hue_a + self._hue_b) * 0.5) + random.randint(0, 255)) & 0xFF

        if self._exploding:
            impact_duration = max(120, int(ctx.effect_state.get("impactDurationMs", 260)))
            impact_radius = max(4.0, min(80.0, float(ctx.effect_state.get("impactRadius", 16))))
            self._explosion_age_ms += max(1, int(dt_ms * speed_scale))
            t = max(0.0, min(1.0, self._explosion_age_ms / float(impact_duration)))
            envelope = max(0.0, 1.0 - t) ** 1.2
            radius = max(2.0, impact_radius * (0.30 + 0.70 * t))
            boom_radius = radius
            boom_t = t
            for i in range(ctx.led_count):
                d = self._ring_distance(float(i), self._explosion_center, ctx.led_count)
                x = d / radius
                glow = math.exp(-0.5 * x * x)
                target[i] = max(target[i], envelope * glow)

            if t >= 1.0:
                self._exploding = False
                self._cooldown_ms = random.randint(120, 580)

        pixels: List[Tuple[int, int, int]] = []
        brightness = max(0.0, min(1.0, float(ctx.brightness_norm)))
        for i in range(ctx.led_count):
            e = (self._energy[i] * 0.74) + (target[i] * 0.26)
            if target[i] <= 0.0005 and e < 0.006:
                e = 0.0
            self._energy[i] = max(0.0, min(1.0, e))

            d_a = self._ring_distance(float(i), self._a, ctx.led_count)
            d_b = self._ring_distance(float(i), self._b, ctx.led_count)
            d_boom = self._ring_distance(float(i), self._explosion_center, ctx.led_count)

            w_a = self._gaussian_glow(d_a, 1.0) if self._active else 0.0
            w_b = self._gaussian_glow(d_b, 1.0) if self._active else 0.0
            w_boom = self._gaussian_glow(d_boom, boom_radius) * (0.75 + (0.25 * (1.0 - boom_t))) if self._exploding else 0.0
            total = w_a + w_b + w_boom

            value = int(255 * (self._energy[i] ** 0.78) * brightness)
            value = max(0, min(255, value))
            if value == 0 or total <= 0.0001:
                pixels.append((0, 0, 0))
                continue

            hue_boom_shifted = (self._hue_boom + ((i * 5) & 0xFF) + int(self._hue_phase)) & 0xFF
            hue = ((w_a * self._hue_a) + (w_b * self._hue_b) + (w_boom * hue_boom_shifted)) / total
            r, g, b = self._hsv_to_rgb(int(hue) & 0xFF, 245, value)
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

    def _add_spark(self, target: List[float], center: float, count: int) -> None:
        sigma = 0.9
        for i in range(count):
            d = self._ring_distance(float(i), center, count)
            glow = self._gaussian_glow(d, sigma)
            target[i] = max(target[i], glow)

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
