from __future__ import annotations

import math
from typing import List, Tuple

from effect_runtime import EffectBase, EffectContext


class SampleEffect(EffectBase):
    def __init__(self) -> None:
        self.led_count = 120

    def editor_settings_schema(self):
        return [
            {"key": "red", "label": "Red", "type": "slider", "min": 0, "max": 255, "step": 1, "default": 255},
            {"key": "green", "label": "Green", "type": "slider", "min": 0, "max": 255, "step": 1, "default": 160},
            {"key": "blue", "label": "Blue", "type": "slider", "min": 0, "max": 255, "step": 1, "default": 48},
            {"key": "wave_speed", "label": "Wave Speed", "type": "slider", "min": 0.1, "max": 3.0, "step": 0.1, "default": 1.0},
            {"key": "wave_offset", "label": "Wave Offset", "type": "slider", "min": 0.0, "max": 6.28, "step": 0.1, "default": 0.0},
        ]

    def render(self, ctx: EffectContext) -> List[Tuple[int, int, int]]:
        pixels: List[Tuple[int, int, int]] = []
        wave_speed = float(ctx.effect_state.get("wave_speed", 1.0))
        wave_offset = float(ctx.effect_state.get("wave_offset", 0.0))
        brightness = float(ctx.global_state.get("brightness", 1.0))
        red = int(ctx.effect_state.get("red", 255))
        green = int(ctx.effect_state.get("green", 160))
        blue = int(ctx.effect_state.get("blue", 48))

        for i in range(ctx.led_count):
            t = (i / max(1, ctx.led_count - 1)) * 2.0 * math.pi
            wave = 0.5 + 0.5 * math.sin(t + ctx.phase_norm * 2.0 * math.pi * wave_speed + wave_offset)
            r = int(red * wave * brightness)
            g = int(green * (0.5 + 0.5 * math.sin(t + 1.0)) * brightness)
            b = int(blue + 180 * (0.5 + 0.5 * math.sin(t + 3.5)) * brightness)
            pixels.append((r, g, b))
        return pixels
