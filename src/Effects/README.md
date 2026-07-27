# LED Effects Engine Contract

This folder contains effect modules for the LED runtime in `src/led/effects_engine.cpp`.

The engine is designed so new effects can be added without changing core engine files.

## What Is Already Handled By The Engine
- Global timing tick and frame pacing
- Global brightness scaling and strip output
- Active LED count clamping
- Pattern selection and effect lifecycle (`onActivate`/`onDeactivate`)
- Effect catalog for web UI (`/api/effects/catalog`)
- Optional per-effect settings + preset API plumbing

Effects should only generate pixel data into the provided `CRGB leds[]` buffer.

## Required Interface
Implement `IEffect` from `src/Effects/IEffect.h`.

Required method:
- `render(const EffectContext &ctx, CRGB *leds, uint16_t count)`

Optional methods:
- `begin(CRGB *leds, uint16_t count)` for one-time init
- `onActivate(const LedEffectState &state)` when effect becomes active
- `onDeactivate()` when effect is replaced
- `settingsSchemaJson()`, `settingsStateJson()`, `setSetting(...)`, `resetSettings()`
- `savePreset(slot)`, `loadPreset(slot)` for effect-local presets

## EffectContext Fields
From `src/Effects/EffectTypes.h`:
- `ctx.state`: global state snapshot (`powerOn`, `brightness`, `speed`, `fps`, `dither`, base `red/green/blue`, etc.)
- `ctx.nowMs`: current `millis()` at render time
- `ctx.deltaMs`: elapsed milliseconds since previous rendered frame
- `ctx.phase`: engine-managed phase accumulator influenced by `speed`
- `ctx.frame`: monotonically increasing frame index
- `ctx.ledCount`: active LED count for this frame

Use `ctx.deltaMs` for time-based animation and `ctx.ledCount` for bounds-safe indexing.

## Registration
Each module self-registers with:

```cpp
REGISTER_EFFECT(YourEffectClass, 7, "Your Label");
```

Rules:
- Effect IDs must be unique (`uint8_t`)
- Keep labels short for dropdown UI
- Registration is controlled by `EFFECTS_ENABLE_REGISTRATION` (enabled in `platformio.ini`)

## Per-Effect Settings Schema
The effects page builds controls from `settingsSchemaJson()`.

Supported control types:
- `slider`: uses `min`, `max`, `step`
- `toggle`: checkbox
- `dropdown`/`select`/`dlist`: with `options`

Example schema:

```json
[
	{"key":"density","label":"Density","type":"slider","min":1,"max":50,"step":1},
	{"key":"mirror","label":"Mirror","type":"toggle"},
	{"key":"palette","label":"Palette","type":"dropdown","options":["warm","cool","neon"]}
]
```

`settingsStateJson()` must return a JSON object keyed by the same setting keys.

## Presets
Use `EffectPresetStore.h` for compact binary preset storage and names:
- `saveEffectPresetBytes(effectId, slot, data, length)`
- `loadEffectPresetBytes(effectId, slot, data, length)`
- `saveEffectPresetName(effectId, slot, name)`
- `loadEffectPresetName(effectId, slot)`

Preset slots are `1..5`.

## Hard Rules For Effect Authors
- Do not call `FastLED.show()` or transport output methods from effects
- Do not block (`delay`, long loops, network IO)
- Do not allocate large heap buffers per frame
- Always respect `count` / `ctx.ledCount` bounds
- Render all needed LEDs every frame (engine may skip duplicate fully-black flushes)

## File Template (Copy/Paste)

```cpp
#include "EffectRegistry.h"
#include <FastLED.h>

namespace {

class ExampleEffect : public IEffect {
 public:
	void onActivate(const LedEffectState &state) override {
		(void)state;
	}

	void render(const EffectContext &ctx, CRGB *leds, uint16_t count) override {
		for (uint16_t i = 0; i < count; ++i) {
			const uint8_t wave = static_cast<uint8_t>((sin8((i * 8 + ctx.phase) & 0xFF)));
			leds[i] = CHSV(wave, 255, 255);
		}
	}

	String settingsSchemaJson() const override {
		return "[]";
	}
};

REGISTER_EFFECT(ExampleEffect, 7, "Example");

}  // namespace
```

## LLM Prompt Starter
Use this prompt with any LLM to generate a new module:

```text
Create one new C++ LED effect module for this project in src/Effects/<Name>.cpp.
Constraints:
- Derive from IEffect and self-register with REGISTER_EFFECT.
- Do not modify engine files.
- Do not call FastLED.show() or delay().
- Render only inside render(ctx, leds, count).
- Keep all indexing within count.
- Include settingsSchemaJson/settingsStateJson/setSetting/resetSettings when useful.
- If presets are used, store with EffectPresetStore slot 1..5.
- Return compile-ready code only.
```
