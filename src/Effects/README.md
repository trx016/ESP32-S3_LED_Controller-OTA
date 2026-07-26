# Effects Plugin Folder

Each effect lives in its own file in this folder and self-registers with the engine.

## Add a new effect
1. Create a new `*.cpp` file in this folder.
2. Implement a class derived from `IEffect`.
3. Register it with `REGISTER_EFFECT(YourClass, yourId, "Your Label")`.

The engine automatically discovers registered effects through the registry and serves them to the web UI via `/api/effects/catalog`.

## Rules
- Do not call `FastLED.show()` inside effect files.
- Render only into the `leds[]` buffer in `render(...)`.
- Keep long delays out of effects; timing is controlled by the LED core engine loop.
