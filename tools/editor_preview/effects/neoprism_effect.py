from __future__ import annotations

from typing import List, Tuple
import random

from effect_runtime import EffectBase, EffectContext


class NeoPrismEffect(EffectBase):
    def __init__(self) -> None:
        self.led_count = 120
        self._active_base_hue = random.randint(0, 255)
        self._target_base_hue = random.randint(0, 255)
        self._phase_ms = 0
        self._transitioning = False

    def editor_settings_schema(self):
        return [
            {"key": "cycleSeconds", "label": "Color Hold sec", "type": "slider", "min": 1, "max": 60, "step": 1, "default": 10},
            {"key": "transitionMs", "label": "Transition ms", "type": "slider", "min": 120, "max": 5000, "step": 10, "default": 1600},
            {"key": "saturation", "label": "Saturation", "type": "slider", "min": 0, "max": 255, "step": 1, "default": 230},
            {"key": "value", "label": "Color Intensity", "type": "slider", "min": 1, "max": 255, "step": 1, "default": 255},
        ]

    def render(self, ctx: EffectContext) -> List[Tuple[int, int, int]]:
        if ctx.led_count <= 0:
            return []

        dt_ms = max(1, int(ctx.delta_ms) if ctx.delta_ms > 0 else 33)
        speed_scale = max(0.2, min(3.0, float(ctx.speed_norm) * 2.0))
        self._phase_ms += max(1, int(dt_ms * speed_scale))

        hold_ms = max(500, int(ctx.effect_state.get("cycleSeconds", 10)) * 1000)
        blend_ms = max(120, int(ctx.effect_state.get("transitionMs", 1600)))

        if not self._transitioning and self._phase_ms >= hold_ms:
            self._transitioning = True
            self._phase_ms = 0
            self._target_base_hue = random.randint(0, 255)
            while self._target_base_hue == self._active_base_hue:
                self._target_base_hue = random.randint(0, 255)

        if self._transitioning and self._phase_ms >= blend_ms:
            self._active_base_hue = self._target_base_hue
            self._transitioning = False
            self._phase_ms = 0

        base_hue = self._compute_base_hue(blend_ms)
        sat = max(0, min(255, int(ctx.effect_state.get("saturation", 230))))
        val = max(1, min(255, int(ctx.effect_state.get("value", 255))))
        brightness = max(0.0, min(1.0, float(ctx.brightness_norm)))

        span = 196
        pixels: List[Tuple[int, int, int]] = []
        denom = max(1, ctx.led_count - 1)
        for i in range(ctx.led_count):
            pos_hue = (base_hue + int((i * span) / denom)) & 0xFF
            r, g, b = self._hsv_to_rgb(pos_hue, sat, val)
            pixels.append(
                (
                    max(0, min(255, int(r * brightness))),
                    max(0, min(255, int(g * brightness))),
                    max(0, min(255, int(b * brightness))),
                )
            )
        return pixels

    def _compute_base_hue(self, blend_ms: int) -> int:
        if not self._transitioning:
            return self._active_base_hue
        t = max(0.0, min(1.0, self._phase_ms / float(max(1, blend_ms))))
        diff = self._target_base_hue - self._active_base_hue
        return int((self._active_base_hue + diff * t)) & 0xFF

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
