Use this workflow for rapid effect iteration:

1. Open the editor-preview folder in VS Code.
2. Edit the effect implementation in `effects/sample_effect.py` or create a new module.
3. Run `python preview.py` from the `editor-preview` folder.
4. To swap to a different effect module, set environment variables before launching:
   - `PREVIEW_EFFECT_MODULE=effects.sample_effect`
   - `PREVIEW_EFFECT_CLASS=SampleEffect`

This keeps firmware development separate from local animation prototyping.
