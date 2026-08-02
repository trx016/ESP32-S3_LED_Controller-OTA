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
    global_state: dict
    effect_state: dict


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
        self._global_state = {
            "brightness": 1.0,
            "speed": 1.0,
            "led_count": led_count,
        }
        self._effect_state = {}

    def set_global_state(self, **kwargs) -> None:
        if "led_count" in kwargs:
            try:
                self.led_count = max(1, int(kwargs["led_count"]))
            except (TypeError, ValueError):
                pass
        self._global_state.update(kwargs)

    def set_effect_state(self, **kwargs) -> None:
        self._effect_state.update(kwargs)

    def build_context(self, now_ms: int) -> EffectContext:
        if self._last_time == 0:
            delta_ms = 0
        else:
            delta_ms = max(0, now_ms - self._last_time)
        self._last_time = now_ms
        self._frame += 1
        self._phase += int(delta_ms * self._global_state.get("speed", 1.0))
        phase_norm = (self._phase % 65536) / 65536.0
        led_count = int(self._global_state.get("led_count", self.led_count))
        self.led_count = led_count

        return EffectContext(
            now_ms=now_ms,
            delta_ms=delta_ms,
            phase=self._phase,
            phase_norm=phase_norm,
            frame=self._frame,
            speed_norm=self._global_state.get("speed", 1.0),
            brightness_norm=self._global_state.get("brightness", 1.0),
            red=int(self._global_state.get("red", 255)),
            green=int(self._global_state.get("green", 160)),
            blue=int(self._global_state.get("blue", 48)),
            led_count=led_count,
            global_state=self._global_state,
            effect_state=self._effect_state,
        )

    def tick(self, now_ms: int) -> List[Tuple[int, int, int]]:
        ctx = self.build_context(now_ms)
        return self.effect.render(ctx)
