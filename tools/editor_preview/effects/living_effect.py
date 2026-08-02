from __future__ import annotations

import math
import random
from typing import List, Tuple

from effect_runtime import EffectBase, EffectContext


class LivingEffect(EffectBase):
    def __init__(self) -> None:
        self.led_count = 120
        self.frame_count = 0
        self.particles: List[dict] = []
        self.heat_buffer: List[int] = []

    def editor_settings_schema(self):
        return [
            {"key": "seeds", "label": "Seeds", "type": "slider", "min": 0, "max": 50, "step": 1, "default": 8},
            {"key": "min_size", "label": "Min Size", "type": "slider", "min": 1, "max": 50, "step": 1, "default": 6},
            {"key": "max_size", "label": "Max Size", "type": "slider", "min": 1, "max": 50, "step": 1, "default": 18},
            {"key": "flicker", "label": "Flicker", "type": "slider", "min": 0, "max": 100, "step": 1, "default": 50},
            {"key": "whitehot", "label": "White Hot %", "type": "slider", "min": 0, "max": 100, "step": 1, "default": 20},
            {"key": "bg_ember", "label": "BG Ember", "type": "slider", "min": 0, "max": 100, "step": 1, "default": 10},
            {"key": "brightness", "label": "Brightness", "type": "slider", "min": 0, "max": 100, "step": 1, "default": 100},
            {"key": "speed", "label": "Speed", "type": "slider", "min": 1, "max": 200, "step": 1, "default": 40},
            {"key": "delay", "label": "Delay", "type": "slider", "min": 0, "max": 120, "step": 1, "default": 14},
            {"key": "glow", "label": "Glow", "type": "slider", "min": 10, "max": 100, "step": 1, "default": 70},
            {"key": "density", "label": "Density", "type": "slider", "min": 5, "max": 100, "step": 1, "default": 40},
            {"key": "red", "label": "Red", "type": "slider", "min": 0, "max": 255, "step": 1, "default": 255},
            {"key": "green", "label": "Green", "type": "slider", "min": 0, "max": 255, "step": 1, "default": 77},
            {"key": "blue", "label": "Blue", "type": "slider", "min": 0, "max": 255, "step": 1, "default": 0},
        ]

    def _spawn_particle(self, count: int, target_particles: int, effect_state: dict) -> None:
        if len(self.particles) >= target_particles:
            return

        particle = {"pos": count - 1 if count > 0 else 0, "velocity": 0, "heat": random.randint(120, 254), "size": random.randint(effect_state.get("min_size", 6), effect_state.get("max_size", 18)), "life": random.randint(70, 219), "max_life": 0}
        if self.particles:
            anchor = self.particles[random.randrange(len(self.particles))]
            particle["pos"] = max(0, min(count - 1, anchor["pos"] + random.randint(0, 40) - 20))
        particle["max_life"] = particle["life"]
        self.particles.append(particle)

    def _update_particle(self, particle: dict, count: int, effect_state: dict, speed_scale: float) -> bool:
        if particle["life"] == 0:
            return False

        life_decay = max(1, int(round(speed_scale)))
        particle["life"] = max(0, particle["life"] - life_decay - (1 if random.random() < 0.2 else 0))
        particle["heat"] = max(0, particle["heat"] - life_decay - (1 if random.random() < 0.25 else 0))
        if random.random() < (0.12 / max(1.0, speed_scale)):
            particle["size"] = max(1, particle["size"] - 1)

        if random.random() < 0.15:
            particle["velocity"] += -1 if random.random() < 0.5 else 1
        particle["velocity"] = max(-2, min(2, particle["velocity"]))

        particle["pos"] += int(round(particle["velocity"] * speed_scale))
        particle["pos"] = max(0, min(count - 1, particle["pos"]))
        return particle["life"] > 0

    def _add_glow(self, buffer: List[int], particle: dict, count: int, effect_state: dict) -> None:
        if particle["life"] == 0:
            return
        spread = int((particle["size"] - 6) * 5 / 12 + 1)
        glow_strength = int((effect_state.get("glow", 70) - 10) * 7 / 90 + 1)
        radius = max(1, spread + glow_strength // 2)
        start = max(0, particle["pos"] - radius)
        end = min(count - 1, particle["pos"] + radius)
        for idx in range(start, end + 1):
            dist = abs(idx - particle["pos"])
            falloff = 0 if dist >= radius else int(255 - (dist * 255 / (radius + 1)))
            amount = int(particle["heat"] * falloff / 255)
            if particle["life"] < particle["max_life"] / 3:
                fade = particle["max_life"] / 3
                amount = int(amount * max(0, min(255, particle["life"] * 255 / max(1, fade))) / 255)
            buffer[idx] = min(255, buffer[idx] + amount)

    def render(self, ctx: EffectContext) -> List[Tuple[int, int, int]]:
        led_count = max(1, ctx.led_count)
        if len(self.heat_buffer) != led_count:
            self.heat_buffer = [0] * led_count
        self.heat_buffer = [0] * led_count
        effect_state = ctx.effect_state

        global_speed = max(0.25, float(ctx.global_state.get("speed", 1.0)))
        effect_speed = max(0.25, min(3.0, float(effect_state.get("speed", 40)) / 40.0))
        speed_scale = max(0.25, min(3.0, global_speed * effect_speed))

        target_particles = max(4, min(96, int((effect_state.get("seeds", 8) * 2) + (effect_state.get("density", 40) / 8))))
        spawn_every = max(1, int(round((10 - (effect_state.get("delay", 14) // 12)) / speed_scale)))

        if self.frame_count % spawn_every == 0:
            self._spawn_particle(led_count, target_particles, effect_state)

        new_particles = []
        for particle in self.particles:
            if self._update_particle(particle, led_count, effect_state, speed_scale):
                self._add_glow(self.heat_buffer, particle, led_count, effect_state)
                new_particles.append(particle)
        self.particles = new_particles

        pixels: List[Tuple[int, int, int]] = []
        for idx in range(led_count):
            self.heat_buffer[idx] = min(255, self.heat_buffer[idx] + effect_state.get("bg_ember", 10))
            if self.heat_buffer[idx] > 0:
                flicker = 255
                if effect_state.get("flicker", 50) > 0 and random.random() * 100 < effect_state.get("flicker", 50):
                    flicker = random.randint(180, 254)
                level = int(self.heat_buffer[idx] * flicker / 255)
                level = int(level * effect_state.get("brightness", 100) / 100)
                red = int(effect_state.get("red", 255))
                green = int(effect_state.get("green", 160))
                blue = int(effect_state.get("blue", 48))
                if effect_state.get("whitehot", 20) > 0:
                    white_amount = int(level * effect_state.get("whitehot", 20) / 100)
                    red += white_amount
                    green += white_amount
                    blue += white_amount
                red = int(red * level / 255)
                green = int(green * level / 255)
                blue = int(blue * level / 255)
                pixels.append((red, green, blue))
            else:
                pixels.append((0, 0, 0))

        self.frame_count += 1
        return pixels
