from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Dict, List, Optional


def get_project_root(root_dir: Optional[Path] = None) -> Path:
    if root_dir is None:
        return Path(__file__).resolve().parent
    return Path(root_dir).resolve()


def get_effects_dir(root_dir: Optional[Path] = None) -> Path:
    return get_project_root(root_dir) / "effects"


def get_state_path(root_dir: Optional[Path] = None) -> Path:
    return get_project_root(root_dir) / ".preview_state.json"


def default_state() -> Dict[str, object]:
    return {
        "selected_effect_module": "effects.sample_effect",
        "global_settings": {
            "brightness": 1.0,
            "speed": 1.0,
            "led_count": 120,
        },
        "effect_settings": {},
    }


def load_state(root_dir: Optional[Path] = None) -> Dict[str, object]:
    path = get_state_path(root_dir)
    if not path.exists():
        return default_state()

    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return default_state()

    state = default_state()
    state.update(data)
    if "global_settings" in data and isinstance(data["global_settings"], dict):
        sanitized_globals = {
            key: data["global_settings"][key]
            for key in ("brightness", "speed", "led_count")
            if key in data["global_settings"]
        }
        state["global_settings"] = {**state["global_settings"], **sanitized_globals}
    if "effect_settings" in data and isinstance(data["effect_settings"], dict):
        state["effect_settings"] = data["effect_settings"]
    return state


def save_state(state: Dict[str, object], root_dir: Optional[Path] = None) -> None:
    path = get_state_path(root_dir)
    path.write_text(json.dumps(state, indent=2), encoding="utf-8")


def get_effect_files(root_dir: Optional[Path] = None) -> List[Dict[str, str]]:
    effects_dir = get_effects_dir(root_dir)
    files: List[Dict[str, str]] = []
    for path in sorted(effects_dir.glob("*.py")):
        if path.name in {"__init__.py", "template_effect.py"}:
            continue
        module_name = f"effects.{path.stem}"
        files.append({"name": path.stem, "module": module_name, "path": str(path)})
    return files


def create_new_effect(name: str, root_dir: Optional[Path] = None) -> str:
    safe_name = re.sub(r"[^a-zA-Z0-9_]+", "_", name).strip("_") or "new_effect"
    effects_dir = get_effects_dir(root_dir)
    effects_dir.mkdir(parents=True, exist_ok=True)

    candidate = safe_name
    suffix = 1
    while (effects_dir / f"{candidate}.py").exists():
        candidate = f"{safe_name}_{suffix}"
        suffix += 1

    target = effects_dir / f"{candidate}.py"
    target.write_text(
        TEMPLATE_EFFECT.format(class_name=candidate.title().replace("_", "") + "Effect"),
        encoding="utf-8",
    )
    return f"effects.{candidate}"


TEMPLATE_EFFECT = '''from __future__ import annotations

from typing import List, Tuple

from effect_runtime import EffectBase, EffectContext


class {class_name}(EffectBase):
    def __init__(self) -> None:
        self.led_count = 120

    def editor_settings_schema(self):
        return [
            {"key": "wave_speed", "label": "Wave Speed", "type": "slider", "min": 0.1, "max": 3.0, "step": 0.1, "default": 1.0},
            {"key": "wave_offset", "label": "Wave Offset", "type": "slider", "min": 0.0, "max": 6.28, "step": 0.1, "default": 0.0},
        ]

    def render(self, ctx: EffectContext) -> List[Tuple[int, int, int]]:
        pixels: List[Tuple[int, int, int]] = []
        wave_speed = float(ctx.effect_state.get("wave_speed", 1.0))
        wave_offset = float(ctx.effect_state.get("wave_offset", 0.0))
        brightness = float(ctx.global_state.get("brightness", 1.0))

        for i in range(ctx.led_count):
            t = (i / max(1, ctx.led_count - 1)) * 2.0 * 3.14159
            wave = 0.5 + 0.5 * (0.5 + 0.5 * (t + wave_offset + ctx.phase_norm * wave_speed * 2.0))
            r = int(255 * wave * brightness)
            g = int(120 * wave * brightness)
            b = int(80 + 120 * wave * brightness)
            pixels.append((r, g, b))
        return pixels
'''
