from __future__ import annotations

import math
import random
from typing import List, Tuple

from effect_runtime import EffectBase, EffectContext


class Ember2Effect(EffectBase):
    def __init__(self) -> None:
        self.led_count = 120
        self.frame_count = 0
        self.particles: List[dict] = []
        self.heat_buffer: List[int] = []

    def editor_settings_schema(self):
        return [
            {"key": "seeds", "label": "Max Active Seeds", "type": "slider", "min": 1, "max": 200, "step": 1, "default": 8},
            {"key": "min_size", "label": "Min Size", "type": "slider", "min": 1, "max": 50, "step": 1, "default": 6},
            {"key": "max_size", "label": "Max Size", "type": "slider", "min": 1, "max": 200, "step": 1, "default": 18},
            {"key": "flicker", "label": "Flicker", "type": "slider", "min": 0, "max": 100, "step": 1, "default": 50},
            {"key": "whitehot", "label": "White Hot %", "type": "slider", "min": 0, "max": 100, "step": 1, "default": 20},
            {"key": "bg_ember", "label": "BG Ember", "type": "slider", "min": 0, "max": 100, "step": 1, "default": 10},
            {"key": "speed", "label": "Speed", "type": "slider", "min": 1, "max": 200, "step": 1, "default": 40},
            {"key": "delay", "label": "Delay", "type": "slider", "min": 0, "max": 120, "step": 1, "default": 14},
            {"key": "glow", "label": "Glow", "type": "slider", "min": 10, "max": 100, "step": 1, "default": 70},
            {"key": "density", "label": "Density", "type": "slider", "min": 5, "max": 100, "step": 1, "default": 40},
            {"key": "red", "label": "Red", "type": "slider", "min": 0, "max": 255, "step": 1, "default": 255},
            {"key": "green", "label": "Green", "type": "slider", "min": 0, "max": 255, "step": 1, "default": 77},
            {"key": "blue", "label": "Blue", "type": "slider", "min": 0, "max": 255, "step": 1, "default": 0},
        ]

    def _wrap_center(self, value: float, count: int) -> float:
        if count <= 0:
            return 0.0
        return value % float(count)

    def _ring_distance(self, a: float, b: float, count: int) -> float:
        if count <= 0:
            return abs(a - b)
        diff = abs(a - b) % float(count)
        return min(diff, float(count) - diff)

    def _signed_ring_delta(self, src: float, dst: float, count: int) -> float:
        if count <= 0:
            return dst - src
        delta = (dst - src) % float(count)
        if delta > (count / 2.0):
            delta -= float(count)
        return delta

    def _spawn_particle(self, count: int, target_particles: int, effect_state: dict) -> None:
        if len(self.particles) >= target_particles:
            return

        min_size = int(effect_state.get("min_size", 6))
        max_size = int(effect_state.get("max_size", 18))
        min_gap = max(1, int((count / max(1, target_particles)) * 1.35))

        # Prefer sparse zones by picking the candidate farthest from existing ember anchors.
        selected_pos = random.randint(0, max(0, count - 1))
        if self.particles:
            best_score = -1.0
            for _ in range(32):
                candidate = random.randint(0, max(0, count - 1))
                nearest = min(
                    self._ring_distance(float(candidate), float(p.get("anchor_pos", p["pos"])), count)
                    for p in self.particles
                )
                if nearest > best_score:
                    best_score = float(nearest)
                    selected_pos = candidate
            if best_score < min_gap:
                for _ in range(24):
                    candidate = random.randint(0, max(0, count - 1))
                    if all(
                        self._ring_distance(float(candidate), float(p.get("anchor_pos", p["pos"])), count) >= min_gap
                        for p in self.particles
                    ):
                        selected_pos = candidate
                        break

        peak_size = random.randint(min_size, max_size)
        start_size = max(0.25, min_size * random.uniform(0.15, 0.35))
        life = random.randint(80, 340) * 4
        birth_frames = random.randint(20, 90)
        target_heat = random.randint(150, 255)
        particle = {
            "pos": selected_pos,
            "anchor_pos": float(selected_pos),
            "center_pos": float(selected_pos),
            "heat": random.randint(8, 40),
            "target_heat": target_heat,
            "size": float(start_size),
            "start_size": float(start_size),
            "peak_size": float(peak_size),
            "life": life,
            "max_life": life,
            "age": 0,
            "birth_frames": birth_frames,
            "ignite_rate": random.uniform(2.0, 8.0),
            "merge_boost_cooldown": 0,
            "merge_boost_count": 0,
            "life_drift": random.uniform(0.65, 1.45),
            "stall_chance": random.uniform(0.02, 0.12),
            "burst_chance": random.uniform(0.04, 0.18),
            "burst_strength": random.uniform(1.0, 2.8),
            "fade_accel": random.uniform(0.85, 1.55),
            "ripple_phase": random.uniform(0.0, math.tau),
            "ripple_rate": random.uniform(0.035, 0.09),
            "ripple_amp": random.uniform(0.6, 2.1),
            "drift_dir": random.choice([-1.0, 1.0]),
            "drift_span": random.uniform(0.4, 2.2),
        }
        self.particles.append(particle)

    def _update_particle(self, particle: dict, effect_state: dict, speed_scale: float) -> bool:
        if particle["life"] == 0:
            return False

        particle["age"] = int(particle.get("age", 0)) + 1

        if particle.get("merge_boost_cooldown", 0) > 0:
            particle["merge_boost_cooldown"] = max(0, int(particle["merge_boost_cooldown"]) - 1)

        life_decay = max(1, int(round(0.55 * speed_scale * float(particle.get("life_drift", 1.0)))))
        if random.random() < float(particle.get("stall_chance", 0.06)):
            life_decay = max(0, life_decay - 1)
        if random.random() < float(particle.get("burst_chance", 0.1)):
            life_decay += int(round(float(particle.get("burst_strength", 1.5))))
        particle["life"] = max(0, particle["life"] - life_decay - (1 if random.random() < 0.2 else 0))
        life_ratio = particle["life"] / max(1, particle["max_life"])

        birth_frames = max(1, int(particle.get("birth_frames", 30)))
        birth_ratio = min(1.0, float(particle.get("age", 0)) / float(birth_frames))

        if life_ratio > 0.55:
            growth = (1.0 - life_ratio) / 0.45
        else:
            growth = max(0.0, life_ratio / 0.55)
        growth *= 0.35 + (0.65 * birth_ratio)

        start_size = float(particle.get("start_size", 1.0))
        peak_size = float(particle.get("peak_size", start_size + 1.0))
        particle["size"] = max(1.0, start_size + (peak_size - start_size) * growth)

        progress = 1.0 - life_ratio
        phase = (self.frame_count * float(particle.get("ripple_rate", 0.05))) + float(particle.get("ripple_phase", 0.0))
        peak_for_motion = max(1.0, float(particle.get("peak_size", particle.get("size", 1.0))))
        size_ratio = max(0.0, min(1.0, float(particle.get("size", 1.0)) / peak_for_motion))
        motion_scale = 0.12 + (size_ratio * size_ratio * 0.88)
        ripple = math.sin(phase) * float(particle.get("ripple_amp", 1.0)) * motion_scale
        drift = float(particle.get("drift_dir", 1.0)) * float(particle.get("drift_span", 1.2)) * (progress * progress) * motion_scale
        particle["center_pos"] = float(particle.get("anchor_pos", particle["pos"])) + ripple + drift
        particle["pos"] = int(round(particle["center_pos"]))

        target_heat = int(particle.get("target_heat", 220))
        if particle["heat"] < target_heat and birth_ratio < 1.0:
            particle["heat"] = min(target_heat, int(particle["heat"] + particle.get("ignite_rate", 4.0) * (0.5 + 0.5 * birth_ratio)))

        heat_drop = max(
            1,
            int(
                round(
                    (1.2 - life_ratio)
                    * speed_scale
                    * float(particle.get("fade_accel", 1.0))
                )
            ),
        )
        if random.random() < float(particle.get("stall_chance", 0.06)) * 0.5:
            heat_drop = max(1, heat_drop - 1)
        particle["heat"] = max(0, particle["heat"] - heat_drop - (1 if random.random() < 0.12 else 0))

        return particle["life"] > 0 and particle["heat"] > 0 and particle["size"] >= 1.0

    def _particle_radius(self, particle: dict, effect_state: dict) -> int:
        spread = int((float(particle["size"]) - 6) * 5 / 12 + 1)
        glow_strength = int((effect_state.get("glow", 70) - 10) * 7 / 90 + 1)
        return max(1, spread + glow_strength // 2)

    def _cohere_particles(self, effect_state: dict, speed_scale: float, led_count: int, target_particles: int) -> None:
        if len(self.particles) < 2:
            return

        max_size = float(effect_state.get("max_size", 18))

        for i in range(len(self.particles) - 1):
            left = self.particles[i]
            left_center = float(left.get("center_pos", left["pos"]))
            left_radius = self._particle_radius(left, effect_state)

            for j in range(i + 1, len(self.particles)):
                right = self.particles[j]
                right_center = float(right.get("center_pos", right["pos"]))
                right_radius = self._particle_radius(right, effect_state)

                join_distance = left_radius + right_radius
                distance = self._ring_distance(right_center, left_center, led_count)
                if distance > join_distance:
                    continue

                closeness = 1.0 - (distance / max(0.001, float(join_distance)))

                # Pull both embers toward a shared center to create smooth organism-like joining.
                to_right = self._signed_ring_delta(left_center, right_center, led_count)
                midpoint = self._wrap_center(left_center + (to_right * 0.5), led_count)
                pull = (0.05 + 0.18 * closeness) * min(1.0, max(0.35, speed_scale / 2.0))
                left_center = self._wrap_center(
                    left_center + (self._signed_ring_delta(left_center, midpoint, led_count) * pull),
                    led_count,
                )
                right_center = self._wrap_center(
                    right_center + (self._signed_ring_delta(right_center, midpoint, led_count) * pull),
                    led_count,
                )

                left["center_pos"] = left_center
                right["center_pos"] = right_center

                # Equalize heat gradually so nearby embers feel like one organism.
                heat_gap = float(left["heat"] - right["heat"])
                transfer = int(abs(heat_gap) * (0.06 + 0.12 * closeness))
                if transfer > 0:
                    if heat_gap > 0:
                        left["heat"] = max(0, int(left["heat"]) - transfer)
                        right["heat"] = min(255, int(right["heat"]) + transfer)
                    else:
                        right["heat"] = max(0, int(right["heat"]) - transfer)
                        left["heat"] = min(255, int(left["heat"]) + transfer)

                growth_bonus = 0.04 + (0.22 * closeness)
                left["peak_size"] = min(max_size, float(left.get("peak_size", left["size"])) + growth_bonus)
                right["peak_size"] = min(max_size, float(right.get("peak_size", right["size"])) + growth_bonus)

                # Absorb very-close embers gradually instead of deleting one instantly.
                can_absorb = len(self.particles) > max(6, int(target_particles * 0.55))
                if can_absorb and distance < max(0.8, min(left_radius, right_radius) * 0.22):
                    dominant, weaker = (left, right) if int(left["heat"]) >= int(right["heat"]) else (right, left)
                    siphon = max(1, int((0.02 + 0.09 * closeness) * int(weaker["heat"])))
                    weaker["heat"] = max(0, int(weaker["heat"]) - siphon)
                    dominant["heat"] = min(255, int(dominant["heat"]) + int(siphon * 0.4))
                    weaker["life"] = max(0, int(weaker["life"]) - max(1, int(speed_scale * (0.35 + (0.45 * closeness)))))

                    # Apply a one-time boosted lifespan when a merge reaches tight overlap.
                    if int(dominant.get("merge_boost_cooldown", 0)) <= 0:
                        life_factor = random.uniform(2.0, 7.0)
                        dominant["life"] = min(20000, int(max(1, dominant["life"]) * life_factor))
                        dominant["max_life"] = min(20000, int(max(1, dominant["max_life"]) * life_factor))
                        dominant["merge_boost_cooldown"] = 60
                        dominant["merge_boost_count"] = int(dominant.get("merge_boost_count", 0)) + 1

        for particle in self.particles:
            center = self._wrap_center(float(particle.get("center_pos", particle["pos"])), led_count)
            particle["center_pos"] = center
            particle["pos"] = int(round(center)) % max(1, led_count)

    def _add_glow(self, buffer: List[int], particle: dict, count: int, effect_state: dict) -> None:
        if particle["life"] == 0:
            return
        spread = int((particle["size"] - 6) * 5 / 12 + 1)
        glow_strength = int((effect_state.get("glow", 70) - 10) * 7 / 90 + 1)
        radius = max(1, spread + glow_strength // 2)
        center = self._wrap_center(float(particle.get("center_pos", particle["pos"])), count)
        birth_frames = max(1, int(particle.get("birth_frames", 30)))
        birth_t = min(1.0, float(particle.get("age", 0)) / float(birth_frames))
        birth_ease = birth_t * birth_t * (3.0 - (2.0 * birth_t))
        center_idx = int(round(center))
        for offset in range(-radius, radius + 1):
            idx = (center_idx + offset) % max(1, count)
            dist = abs(offset)
            falloff = 0 if dist >= radius else int(255 - (dist * 255 / (radius + 1)))
            amount = int((particle["heat"] * falloff / 255) * birth_ease)
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

        seeds_cap = max(1, int(effect_state.get("seeds", 8)))
        large_strip_cap = max(96, led_count // 6)
        target_particles = max(1, min(large_strip_cap, seeds_cap))
        spawn_every = max(1, int(round(10 - (effect_state.get("delay", 14) // 12))))

        if self.frame_count % spawn_every == 0:
            deficit = max(0, target_particles - len(self.particles))
            spawn_batch = min(12, max(1, 1 + (deficit // 20)))
            for _ in range(spawn_batch):
                self._spawn_particle(led_count, target_particles, effect_state)

        new_particles = []
        for particle in self.particles:
            if self._update_particle(particle, effect_state, speed_scale):
                new_particles.append(particle)
        self.particles = new_particles

        # Hard-enforce current seeds cap immediately when the user lowers it.
        if len(self.particles) > target_particles:
            self.particles.sort(key=lambda p: int(p.get("heat", 0)), reverse=True)
            self.particles = self.particles[:target_particles]

        self._cohere_particles(effect_state, speed_scale, led_count, target_particles)

        for particle in self.particles:
            self._add_glow(self.heat_buffer, particle, led_count, effect_state)

        pixels: List[Tuple[int, int, int]] = []
        for idx in range(led_count):
            self.heat_buffer[idx] = min(255, self.heat_buffer[idx] + effect_state.get("bg_ember", 10))
            if self.heat_buffer[idx] > 0:
                flicker = 255
                if effect_state.get("flicker", 50) > 0 and random.random() * 100 < effect_state.get("flicker", 50):
                    flicker = random.randint(180, 254)
                level = int(self.heat_buffer[idx] * flicker / 255)
                level = int(level * max(0.0, min(1.0, ctx.brightness_norm)))
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
