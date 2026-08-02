# External Editor Preview

This folder is a separate local preview workspace for developing and testing LED effects without uploading to the controller every time.

## Structure
- `effects/` - effect source files you can edit in VS Code
- `preview.py` - simple local animation preview using Python and Tkinter
- `effect_runtime.py` - shared helpers for time stepping and color math

## How to use
1. Edit an effect file under `effects/`.
2. Run `python preview.py`.
3. The preview window will animate the effect locally.

This keeps the firmware project separate from the editor workflow while allowing rapid iteration.
