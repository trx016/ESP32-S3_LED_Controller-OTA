from __future__ import annotations

import importlib
import json
import math
import os
import sys
import tkinter as tk
from tkinter import ttk
from typing import Dict, List, Optional, Tuple

from editor_state import create_new_effect, get_effect_files, load_state, save_state
from effect_runtime import EffectBase, PreviewRunner


class PreviewUI:
    def __init__(self, root: tk.Tk, effect_cls: Optional[type[EffectBase]], root_dir: Optional[str] = None) -> None:
        self.root = root
        self.root_dir = root_dir
        self.state = load_state(root_dir)
        self.effect_cls = effect_cls
        self.effect = None
        self.runner = None
        self.file_menu = None
        self.effect_schema = []
        self.effect_controls = {}

        self._load_effect_instance(effect_cls)

        self.root.title("LED Effect Preview")
        self.root.geometry("1100x620")

        self.left = ttk.Frame(root, width=380)
        self.left.pack(side=tk.LEFT, fill=tk.Y)
        self.left.pack_propagate(False)

        self.right = ttk.Frame(root)
        self.right.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        self._build_sidebar()
        self._build_canvas()

    def _effect_key(self) -> str:
        return self.effect.__class__.__name__.lower()

    def _load_effect_instance(self, effect_cls: Optional[type[EffectBase]]) -> None:
        if effect_cls is None:
            module_name = self.state.get("selected_effect_module", "effects.sample_effect")
            module = importlib.import_module(module_name)
            class_name = next(
                name for name in dir(module)
                if name != "EffectBase" and name.endswith("Effect") and hasattr(getattr(module, name), "__call__")
            )
            effect_cls = getattr(module, class_name)

        self.effect = effect_cls()
        self.runner = PreviewRunner(self.effect, led_count=120, fps=30)
        self.runner.set_global_state(**self.state.get("global_settings", {}))
        self.runner.set_effect_state(**self.state.get("effect_settings", {}).get(self._effect_key(), {}))

    def _build_sidebar(self) -> None:
        selected_tab_label = "Global"
        if hasattr(self, "sidebar_notebook") and self.sidebar_notebook.winfo_exists():
            try:
                current_tab = self.sidebar_notebook.select()
                if current_tab:
                    selected_tab_label = self.sidebar_notebook.tab(current_tab, "text")
            except tk.TclError:
                selected_tab_label = "Global"

        for child in self.left.winfo_children():
            child.destroy()

        top_row = ttk.Frame(self.left)
        top_row.pack(fill=tk.X, padx=8, pady=(8, 4))
        ttk.Button(top_row, text="Refresh Effect", command=self._refresh_current_effect).pack(fill=tk.X)

        self.sidebar_notebook = ttk.Notebook(self.left)
        self.sidebar_notebook.pack(fill=tk.BOTH, expand=True, padx=6, pady=(6, 0))

        global_tab = ttk.Frame(self.sidebar_notebook)
        var_tab = ttk.Frame(self.sidebar_notebook)
        self.sidebar_notebook.add(global_tab, text="Global")
        self.sidebar_notebook.add(var_tab, text="Var")

        ttk.Label(global_tab, text="Runtime Controls", font=("Segoe UI", 10, "bold")).pack(anchor=tk.W, padx=8, pady=(8, 4))
        self._add_slider(global_tab, "brightness", 0.1, 3.0, 0.1, self.state.get("global_settings", {}).get("brightness", 1.0), callback=self._update_global)
        self._add_slider(global_tab, "speed", 0.1, 3.0, 0.1, self.state.get("global_settings", {}).get("speed", 1.0), callback=self._update_global)
        self._add_slider(global_tab, "led_count", 8, 2000, 1, self.state.get("global_settings", {}).get("led_count", 120), callback=self._update_global)
        self._add_slider(global_tab, "pixel_scale", 0.5, 4.0, 0.1, self.state.get("global_settings", {}).get("pixel_scale", 1.0), callback=self._update_global)

        ttk.Label(var_tab, text="Effect Variables", font=("Segoe UI", 10, "bold")).pack(anchor=tk.W, padx=8, pady=(8, 4))
        self.effect_schema = []
        if hasattr(self.effect, "editor_settings_schema"):
            self.effect_schema = self.effect.editor_settings_schema()

        self.effect_controls = {}
        effect_state = self.state.get("effect_settings", {}).get(self._effect_key(), {})

        var_canvas = tk.Canvas(var_tab, highlightthickness=0)
        var_scrollbar = ttk.Scrollbar(var_tab, orient=tk.VERTICAL, command=var_canvas.yview)
        var_canvas.configure(yscrollcommand=var_scrollbar.set)
        var_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        var_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(8, 4), pady=(0, 4))

        var_inner = ttk.Frame(var_canvas)
        var_window = var_canvas.create_window((0, 0), window=var_inner, anchor="nw")

        def on_var_inner_configure(event) -> None:
            var_canvas.configure(scrollregion=var_canvas.bbox("all"))

        def on_var_canvas_configure(event) -> None:
            var_canvas.itemconfigure(var_window, width=event.width)
            var_canvas.configure(scrollregion=var_canvas.bbox("all"))

        def on_var_mousewheel(event) -> str:
            pointer_x, pointer_y = self.root.winfo_pointerxy()
            canvas_x = var_canvas.winfo_rootx()
            canvas_y = var_canvas.winfo_rooty()
            canvas_w = var_canvas.winfo_width()
            canvas_h = var_canvas.winfo_height()
            if not (canvas_x <= pointer_x <= canvas_x + canvas_w and canvas_y <= pointer_y <= canvas_y + canvas_h):
                return ""
            step = -1 if event.delta > 0 else 1
            var_canvas.yview_scroll(step, "units")
            return "break"

        def bind_var_mousewheel(_event) -> None:
            self.root.bind_all("<MouseWheel>", on_var_mousewheel)

        def unbind_var_mousewheel(_event) -> None:
            self.root.unbind_all("<MouseWheel>")

        var_inner.bind("<Configure>", on_var_inner_configure)
        var_canvas.bind("<Configure>", on_var_canvas_configure)
        var_canvas.bind("<Enter>", bind_var_mousewheel)
        var_canvas.bind("<Leave>", unbind_var_mousewheel)
        var_inner.bind("<Enter>", bind_var_mousewheel)
        var_inner.bind("<Leave>", unbind_var_mousewheel)

        for item in self.effect_schema:
            self.effect_controls[item["key"]] = self._add_slider(
                var_inner,
                item["key"],
                float(item.get("min", 0)),
                float(item.get("max", 1)),
                float(item.get("step", 1)),
                float(effect_state.get(item["key"], item.get("default", 0))),
                callback=self._update_effect,
            )

        ttk.Separator(self.left, orient=tk.HORIZONTAL).pack(fill=tk.X, padx=8, pady=8)
        ttk.Label(self.left, text="Effect Files", font=("Segoe UI", 10, "bold")).pack(anchor=tk.W, padx=8, pady=(0, 2))

        self.file_var = tk.StringVar(value=self.state.get("selected_effect_module", "effects.living_effect"))
        self.file_menu = ttk.Combobox(self.left, textvariable=self.file_var, state="readonly")
        self.file_menu.pack(fill=tk.X, padx=8, pady=(0, 6))
        self.file_menu.bind("<<ComboboxSelected>>", lambda _: self._load_selected_effect())

        self.new_name_var = tk.StringVar()
        ttk.Entry(self.left, textvariable=self.new_name_var).pack(fill=tk.X, padx=8, pady=(0, 6))
        ttk.Button(self.left, text="New Effect", command=self._create_new_effect).pack(fill=tk.X, padx=8, pady=(0, 8))

        if selected_tab_label.lower() == "var":
            self.sidebar_notebook.select(var_tab)
        else:
            self.sidebar_notebook.select(global_tab)

        self._populate_effect_files()

    def _build_canvas(self) -> None:
        self.canvas = tk.Canvas(self.right, bg="black")
        self.canvas.pack(fill=tk.BOTH, expand=True)
        self.pixels = []
        led_count = self.runner.led_count if self.runner is not None else 120
        self.layout_positions = self._build_perimeter_positions(led_count)
        self._animate()

    def _add_slider(self, parent, key: str, min_value: float, max_value: float, step: float, default: float, callback) -> ttk.Scale:
        frame = ttk.Frame(parent)
        frame.pack(fill=tk.X, padx=8, pady=(0, 4))
        label = ttk.Label(frame, text=key.replace("_", " ").title(), font=("Segoe UI", 9))
        label.pack(anchor=tk.W)
        value_var = tk.StringVar(value=str(default))
        var = tk.DoubleVar(value=default)
        scale = ttk.Scale(frame, from_=min_value, to=max_value, variable=var, orient=tk.HORIZONTAL)
        scale.pack(fill=tk.X)
        value_label = ttk.Label(frame, textvariable=value_var, foreground="#4a4a4a", font=("Segoe UI", 8))
        value_label.pack(anchor=tk.E)

        def update_value(*args, k=key, v=var, value_widget=value_var, cb=callback):
            val = v.get()
            if step >= 1:
                val = int(round(val))
            else:
                val = round(val, 2)
            value_widget.set(str(val))
            cb(k, val)

        var.trace_add("write", update_value)
        scale.bind("<ButtonRelease-1>", lambda event, k=key, v=var, cb=callback: update_value(k=k, v=v, cb=cb))
        scale.bind("<Motion>", lambda event, k=key, v=var, cb=callback: update_value(k=k, v=v, cb=cb))
        return scale

    def _update_global(self, key: str, value: float) -> None:
        self.runner.set_global_state(**{key: value})
        global_settings = self.state.setdefault("global_settings", {})
        if key in {"brightness", "speed", "led_count", "pixel_scale"}:
            global_settings[key] = value
        for legacy_key in ("red", "green", "blue"):
            global_settings.pop(legacy_key, None)
        if key == "led_count":
            self.layout_positions = self._build_perimeter_positions(int(value))
        self._save_state()

    def _update_effect(self, key: str, value: float) -> None:
        self.runner.set_effect_state(**{key: value})
        self.state.setdefault("effect_settings", {}).setdefault(self._effect_key(), {})[key] = value
        self._save_state()

    def _save_state(self) -> None:
        save_state(self.state, self.root_dir)

    def _build_perimeter_positions(self, led_count: int) -> List[Tuple[int, int]]:
        if led_count <= 0:
            return []

        size = max(3, int(math.ceil((max(led_count, 1) + 4) / 4)))
        perimeter = []
        for y in range(size):
            perimeter.append((0, y))
        for x in range(1, size):
            perimeter.append((x, size - 1))
        for y in range(size - 2, -1, -1):
            perimeter.append((size - 1, y))
        for x in range(size - 2, 0, -1):
            perimeter.append((x, 0))

        return perimeter[:led_count]

    def _animate(self) -> None:
        try:
            now_ms = int(__import__("time").time() * 1000)
            self.pixels = self.runner.tick(now_ms)
            self._draw()
        except Exception as exc:
            print(f"Preview animation error: {exc}")
        self.root.after(16, self._animate)

    def _draw(self) -> None:
        self.canvas.delete("all")
        width = self.canvas.winfo_width()
        height = self.canvas.winfo_height()
        if width <= 1 or height <= 1:
            return

        margin = 24
        usable_width = max(1, width - margin * 2)
        usable_height = max(1, height - margin * 2)
        if not self.layout_positions:
            return

        max_coord = 1
        for row, col in self.layout_positions:
            max_coord = max(max_coord, row + 1, col + 1)

        cols = max(1, max_coord)
        rows = max(1, max_coord)
        base_cell_width = usable_width / max(1, cols)
        base_cell_height = usable_height / max(1, rows)
        pixel_scale = float(self.state.get("global_settings", {}).get("pixel_scale", 1.0))
        pixel_scale = max(0.5, min(4.0, pixel_scale))
        scaled_width = max(1.0, base_cell_width * pixel_scale)
        scaled_height = max(1.0, base_cell_height * pixel_scale)
        offset_x = margin
        offset_y = margin

        for index, (r, g, b) in enumerate(self.pixels):
            if index >= len(self.layout_positions):
                continue
            row, col = self.layout_positions[index]
            center_x = offset_x + ((col + 0.5) * base_cell_width)
            center_y = offset_y + ((row + 0.5) * base_cell_height)

            width_factor = 1.0
            height_factor = 1.0
            if col == 0 or col == (cols - 1):
                width_factor = 1.25
            if row == 0 or row == (rows - 1):
                height_factor = 1.25

            half_w = max(0.5, ((scaled_width * width_factor) * 0.5) - 1.0)
            half_h = max(0.5, ((scaled_height * height_factor) * 0.5) - 1.0)

            x0 = max(margin, center_x - half_w)
            y0 = max(margin, center_y - half_h)
            x1 = min(width - margin, center_x + half_w)
            y1 = min(height - margin, center_y + half_h)
            if x1 <= x0 or y1 <= y0:
                continue
            safe_r = max(0, min(255, int(r)))
            safe_g = max(0, min(255, int(g)))
            safe_b = max(0, min(255, int(b)))
            self.canvas.create_rectangle(x0, y0, x1, y1, fill="#%02x%02x%02x" % (safe_r, safe_g, safe_b), outline="")

    def _populate_effect_files(self) -> None:
        files = get_effect_files(self.root_dir)
        self.file_menu["values"] = [item["module"] for item in files]
        if self.state.get("selected_effect_module") in [item["module"] for item in files]:
            self.file_var.set(self.state.get("selected_effect_module"))

    def _load_selected_effect(self) -> None:
        module_name = self.file_var.get()
        if not module_name:
            return
        self.state["selected_effect_module"] = module_name
        self._save_state()
        self._reload_effect(module_name)

    def _create_new_effect(self) -> None:
        name = self.new_name_var.get().strip()
        if not name:
            return
        module_name = create_new_effect(name, self.root_dir)
        self.state["selected_effect_module"] = module_name
        self._save_state()
        self._populate_effect_files()
        self._reload_effect(module_name, force_reload=True)

    def _refresh_current_effect(self) -> None:
        module_name = self.file_var.get() if hasattr(self, "file_var") else self.state.get("selected_effect_module", "")
        if not module_name:
            return
        self._reload_effect(module_name, force_reload=True)

    def _reload_effect(self, module_name: str, force_reload: bool = False) -> None:
        if force_reload and module_name in sys.modules:
            module = importlib.reload(sys.modules[module_name])
        else:
            module = importlib.import_module(module_name)
        class_name = next(
            name for name in dir(module)
            if name != "EffectBase" and name.endswith("Effect") and hasattr(getattr(module, name), "__call__")
        )
        effect_cls = getattr(module, class_name)
        self.effect = effect_cls()
        led_count = int(self.state.get("global_settings", {}).get("led_count", 120))
        self.runner = PreviewRunner(self.effect, led_count=led_count, fps=30)
        self.runner.set_global_state(**self.state.get("global_settings", {}))
        self.runner.set_effect_state(**self.state.get("effect_settings", {}).get(self._effect_key(), {}))
        self.layout_positions = self._build_perimeter_positions(led_count)
        self._build_sidebar()


if __name__ == "__main__":
    root = tk.Tk()
    PreviewUI(root, None)
    root.mainloop()
