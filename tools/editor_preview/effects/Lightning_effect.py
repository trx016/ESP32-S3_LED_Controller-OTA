from __future__ import annotations

import math
import random
from typing import List, Tuple

from effect_runtime import EffectBase, EffectContext


class LightningEffect(EffectBase):
    def __init__(self) -> None:
        self.led_count = 120
        self._energy: List[float] = []
        self._pulses: List[dict] = []
        self._burst_remaining = 0
        self._burst_delay_ms = 0

    def editor_settings_schema(self):
        return [
            {"key": "activity", "label": "Activity", "type": "slider", "min": 1, "max": 100, "step": 1, "default": 24},
            {"key": "minSpread", "label": "Arc Spread Min", "type": "slider", "min": 2, "max": 60, "step": 1, "default": 8},
            {"key": "maxSpread", "label": "Arc Spread Max", "type": "slider", "min": 2, "max": 60, "step": 1, "default": 18},
            {"key": "softness", "label": "Softness", "type": "slider", "min": 10, "max": 100, "step": 1, "default": 82},
            {"key": "branching", "label": "Branching", "type": "slider", "min": 0, "max": 100, "step": 1, "default": 28},
        ]

    def render(self, ctx: EffectContext) -> List[Tuple[int, int, int]]:
        if len(self._energy) != ctx.led_count:
            self._energy = [0.0] * ctx.led_count
            self._pulses.clear()
            self._burst_remaining = 0
            self._burst_delay_ms = 0

        dt_ms = max(1, int(ctx.delta_ms) if ctx.delta_ms > 0 else 33)
        speed = max(0.25, min(3.0, float(ctx.speed_norm)))

        activity = max(1.0, min(100.0, float(ctx.effect_state.get("activity", 16.0)))) / 100.0
        min_spread = max(2.0, min(60.0, float(ctx.effect_state.get("minSpread", 8.0))))
        max_spread = max(min_spread, min(60.0, float(ctx.effect_state.get("maxSpread", 18.0))))
        softness = max(10.0, min(100.0, float(ctx.effect_state.get("softness", 82.0)))) / 100.0
        branching = max(0.0, min(100.0, float(ctx.effect_state.get("branching", 28.0)))) / 100.0
        timing_scale = max(0.25, min(3.0, speed))

        # Sleep-friendly, but visible in preview at default settings.
        strikes_per_second = (0.10 + (activity * 1.20)) * (0.60 + (0.80 * speed))
        spawn_chance = 1.0 - math.exp(-(strikes_per_second * (dt_ms / 1000.0)))

        if random.random() < spawn_chance:
            self._spawn_cluster(ctx.led_count, min_spread, max_spread, branching, activity, timing_scale)

        if self._burst_remaining > 0:
            self._burst_delay_ms -= max(1, int(dt_ms * timing_scale))
            if self._burst_delay_ms <= 0:
                self._spawn_cluster(
                    ctx.led_count,
                    max(2.0, min_spread * 0.9),
                    max(2.0, max_spread * 0.9),
                    branching * 0.8,
                    activity * 0.8,
                    timing_scale,
                )
                self._burst_remaining -= 1
                self._burst_delay_ms = max(12, int(random.uniform(40.0, 110.0) / timing_scale))

        target = [0.0] * ctx.led_count
        updated_pulses = []

        for pulse in self._pulses:
            pulse["age_ms"] += max(1, int(dt_ms * timing_scale))
            age = pulse["age_ms"]
            duration = pulse["duration_ms"]
            if age >= duration:
                continue

            t = age / duration
            envelope = self._envelope(t)
            flicker = 0.84 + (0.16 * math.sin((age / 1000.0) * pulse["flicker_hz"] * 2.0 * math.pi + pulse["phase"]))
            amp = pulse["peak"] * envelope * flicker

            sigma = max(1.0, pulse["radius"] * (0.35 + (0.60 * softness)))
            center = int(pulse["center"])

            for i in range(ctx.led_count):
                d = self._ring_distance(i, center, ctx.led_count)
                falloff = math.exp(-0.5 * ((d / sigma) ** 2))
                target[i] += amp * falloff

            updated_pulses.append(pulse)

        self._pulses = updated_pulses

        hold = max(0.55, min(0.92, 0.92 - ((speed - 0.25) * 0.14) + ((softness - 0.5) * 0.08)))
        rise_step = 0.020 + (0.070 * speed)
        fall_step = 0.015 + (0.050 * speed)
        fade_floor = 0.012

        pixels: List[Tuple[int, int, int]] = []
        brightness = max(0.0, min(1.0, float(ctx.brightness_norm)))

        for i in range(ctx.led_count):
            t = max(0.0, min(1.0, target[i]))
            e = (self._energy[i] * hold) + (t * (1.0 - hold))
            if e > self._energy[i]:
                e = min(e, self._energy[i] + rise_step)
            else:
                e = max(e, self._energy[i] - fall_step)

            self._energy[i] = max(0.0, min(1.0, e))
            if t <= 0.0005 and self._energy[i] < fade_floor:
                self._energy[i] = 0.0

            power = (self._energy[i] ** 0.72) * brightness
            white = int(255 * power)
            pixels.append((min(255, white), min(255, white), min(255, white)))

        return pixels

    def _spawn_cluster(
        self,
        led_count: int,
        min_spread: float,
        max_spread: float,
        branching: float,
        activity: float,
        timing_scale: float,
    ) -> None:
        center = random.randint(0, max(0, led_count - 1))
        base_spread = random.uniform(min_spread, max_spread)
        peak = 0.18 + (activity * 0.32) + random.uniform(0.02, 0.14)
        radius = max(2.0, base_spread * random.uniform(0.72, 1.18))
        duration = max(60, int(random.uniform(140.0, 360.0) / timing_scale))
        flicker_scale = max(0.75, min(1.5, 0.75 + (timing_scale * 0.25)))
        self._pulses.append(
            {
                "center": center,
                "peak": min(0.82, peak),
                "radius": radius,
                "duration_ms": duration,
                "age_ms": 0,
                "flicker_hz": random.uniform(10.0, 18.0) * flicker_scale,
                "phase": random.uniform(0.0, math.pi * 2.0),
            }
        )

        branch_count = 0
        if random.random() < branching:
            branch_count = 1 + (1 if random.random() < (branching * 0.45) else 0)

        for _ in range(branch_count):
            offset = int(random.uniform(-base_spread * 1.8, base_spread * 1.8))
            self._pulses.append(
                {
                    "center": (center + offset) % max(1, led_count),
                    "peak": min(0.65, peak * random.uniform(0.45, 0.72)),
                    "radius": max(2.0, radius * random.uniform(0.45, 0.75)),
                    "duration_ms": max(45, int(random.uniform(90.0, 240.0) / timing_scale)),
                    "age_ms": 0,
                    "flicker_hz": random.uniform(12.0, 22.0) * flicker_scale,
                    "phase": random.uniform(0.0, math.pi * 2.0),
                }
            )

        if random.random() < (0.18 + (activity * 0.45)):
            self._burst_remaining = random.randint(1, 2)
            self._burst_delay_ms = max(12, int(random.uniform(30.0, 90.0) / timing_scale))

    @staticmethod
    def _ring_distance(a: int, b: int, count: int) -> int:
        if count <= 0:
            return abs(a - b)
        d = abs(a - b) % count
        return min(d, count - d)

    @staticmethod
    def _envelope(t: float) -> float:
        t = max(0.0, min(1.0, t))
        if t < 0.12:
            return (t / 0.12) ** 0.65
        decay = (1.0 - ((t - 0.12) / 0.88))
        return max(0.0, decay) ** 1.6
