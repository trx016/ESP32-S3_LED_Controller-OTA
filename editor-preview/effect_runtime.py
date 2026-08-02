from __future__ import annotations

import math
import time
from dataclasses import dataclass
from typing import Callable, List, Tuple


@dataclass
class EffectContext:
    now_ms: int
    delta_ms: int
    phase: int
    phase_norm: float
    frame: int
    speed_norm: float
    brightness_norm: float
    red: int
    green: int
    blue: int
    led_count: int


class EffectBase:
    def begin(self, led_count: int) -> None:
        self.led_count = led_count

    def render(self, ctx: EffectContext) -> List[Tuple[int, int, int]]:
        raise NotImplementedError


class PreviewRunner:
    def __init__(self, effect: EffectBase, led_count: int = 120, fps: int = 30) -> None:
        self.effect = effect
        self.led_count = led_count
        self.fps = fps
        self._last_time = 0
        self._phase = 0
        self._frame = 0

    def tick(self, now_ms: int) -> List[Tuple[int, int, int]]:
        if self._last_time == 0:
            delta_ms = 0
        else:
            delta_ms = max(0, now_ms - self._last_time)
        self._last_time = now_ms
        self._frame += 1
        self._phase += int(delta_ms * 1.0)
        phase_norm = (self._phase % 65536) / 65536.0

        ctx = EffectContext(
            now_ms=now_ms,
            delta_ms=delta_ms,
            phase=self._phase,
            phase_norm=phase_norm,
            frame=self._frame,
            speed_norm=1.0,
            brightness_norm=1.0,
            red=255,
            green=160,
            blue=48,
            led_count=self.led_count,
        )
        return self.effect.render(ctx)
