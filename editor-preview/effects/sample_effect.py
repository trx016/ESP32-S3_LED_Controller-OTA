from __future__ import annotations

import math
from typing import List, Tuple

from effect_runtime import EffectBase, EffectContext


class SampleEffect(EffectBase):
    def __init__(self) -> None:
        self.led_count = 120

    def render(self, ctx: EffectContext) -> List[Tuple[int, int, int]]:
        pixels: List[Tuple[int, int, int]] = []
        for i in range(ctx.led_count):
            t = (i / max(1, ctx.led_count - 1)) * 2.0 * math.pi
            wave = 0.5 + 0.5 * math.sin(t + ctx.phase_norm * 2.0 * math.pi)
            r = int(255 * wave)
            g = int(160 * (0.5 + 0.5 * math.sin(t + 1.0)))
            b = int(48 + 180 * (0.5 + 0.5 * math.sin(t + 3.5)))
            pixels.append((r, g, b))
        return pixels
