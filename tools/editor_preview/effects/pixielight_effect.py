from __future__ import annotations

import math
import random
from typing import List, Tuple

from effect_runtime import EffectBase, EffectContext


class PixieLightEffect(EffectBase):
    def __init__(self) -> None:
        self.led_count = 120
        self._base_hue: List[int] = []
        self._phase: List[float] = []
        self._rate_hz: List[float] = []

    def editor_settings_schema(self):
        return [
            {"key": "breathMin", "label": "Breath Min", "type": "slider", "min": 1, "max": 255, "step": 1, "default": 20},
            {"key": "breathMax", "label": "Breath Max", "type": "slider", "min": 1, "max": 255, "step": 1, "default": 220},
            {"key": "rateMin", "label": "Rate Min", "type": "slider", "min": 5, "max": 120, "step": 1, "default": 18},
            {"key": "rateMax", "label": "Rate Max", "type": "slider", "min": 5, "max": 140, "step": 1, "default": 70},
            {"key": "saturation", "label": "Saturation", "type": "slider", "min": 0, "max": 255, "step": 1, "default": 210},
            {"key": "hueJitter", "label": "Hue Drift", "type": "slider", "min": 0, "max": 100, "step": 1, "default": 70},
        ]

    def render(self, ctx: EffectContext) -> List[Tuple[int, int, int]]:
        if len(self._base_hue) != ctx.led_count:
            self._allocate(ctx.led_count, ctx.effect_state)

        dt_ms = max(1, int(ctx.delta_ms) if ctx.delta_ms > 0 else 33)
        dt_sec = dt_ms / 1000.0
        speed_scale = max(0.25, min(3.0, float(ctx.speed_norm) * 2.0))

        breath_min = max(1, min(255, int(ctx.effect_state.get("breathMin", 20))))
        breath_max = max(1, min(255, int(ctx.effect_state.get("breathMax", 220))))
        lo = min(breath_min, breath_max)
        hi = max(breath_min, breath_max)
        val_range = max(1, hi - lo)

        sat = max(0, min(255, int(ctx.effect_state.get("saturation", 210))))
        hue_jitter = max(0, min(100, int(ctx.effect_state.get("hueJitter", 70))))
        brightness = max(0.0, min(1.0, float(ctx.brightness_norm)))

        pixels: List[Tuple[int, int, int]] = []
        for i in range(ctx.led_count):
            self._phase[i] += self._rate_hz[i] * dt_sec * speed_scale * math.tau
            if self._phase[i] > math.tau:
                self._phase[i] = self._phase[i] % math.tau

            wave = 0.5 + 0.5 * math.sin(self._phase[i])
            value = int(lo + val_range * wave)

            hue = self._base_hue[i]
            if hue_jitter > 0:
                hue += int(math.sin(self._phase[i] * 0.65) * hue_jitter)
            hue &= 0xFF

            r, g, b = self._hsv_to_rgb(hue, sat, value)
            pixels.append(
                (
                    max(0, min(255, int(r * brightness))),
                    max(0, min(255, int(g * brightness))),
                    max(0, min(255, int(b * brightness))),
                )
            )

        return pixels

    def _allocate(self, count: int, effect_state: dict) -> None:
        self._base_hue = [random.randint(0, 255) for _ in range(max(0, count))]
        self._phase = [random.uniform(0.0, math.tau) for _ in range(max(0, count))]
        self._rate_hz = [self._random_rate(effect_state) for _ in range(max(0, count))]

    @staticmethod
    def _random_rate(effect_state: dict) -> float:
        rate_min = max(5.0, min(120.0, float(effect_state.get("rateMin", 18.0)))) / 100.0
        rate_max = max(rate_min, min(140.0, float(effect_state.get("rateMax", 70.0)))) / 100.0
        return random.uniform(rate_min, rate_max)

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
