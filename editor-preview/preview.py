import importlib
import math
import os
import sys
import tkinter as tk
import time
from typing import List, Tuple

from effect_runtime import EffectBase, EffectContext, PreviewRunner


sys.path.insert(0, os.path.dirname(__file__))


class DemoEffect(EffectBase):
    def __init__(self) -> None:
        self.led_count = 120

    def render(self, ctx: EffectContext) -> List[Tuple[int, int, int]]:
        pixels: List[Tuple[int, int, int]] = []
        for i in range(ctx.led_count):
            t = (i / max(1, ctx.led_count - 1)) * 2.0 * math.pi
            wave = 0.5 + 0.5 * math.sin(t + ctx.phase_norm * 2.0 * math.pi)
            r = int(255 * wave)
            g = int(160 * (0.5 + 0.5 * math.sin(t + 2.0)))
            b = int(48 + 200 * (0.5 + 0.5 * math.sin(t + 4.0)))
            pixels.append((r, g, b))
        return pixels


class PreviewWindow:
    def __init__(self, root: tk.Tk, effect: EffectBase, led_count: int = 120) -> None:
        self.root = root
        self.runner = PreviewRunner(effect, led_count=led_count, fps=30)
        self.canvas = tk.Canvas(root, width=800, height=240, bg="black")
        self.canvas.pack(fill=tk.BOTH, expand=True)
        self.pixels: List[Tuple[int, int, int]] = []
        self._running = True
        self._last_time = time.time()
        self._animate()

    def _animate(self) -> None:
        now_ms = int(time.time() * 1000)
        self.pixels = self.runner.tick(now_ms)
        self._draw()
        self.root.after(16, self._animate)

    def _draw(self) -> None:
        self.canvas.delete("all")
        width = self.canvas.winfo_width()
        height = self.canvas.winfo_height()
        margin = 10
        bar_width = max(2, (width - margin * 2) // max(1, len(self.pixels)))
        for index, (r, g, b) in enumerate(self.pixels):
            x0 = margin + index * bar_width
            x1 = x0 + max(1, bar_width - 1)
            y0 = margin
            y1 = height - margin
            self.canvas.create_rectangle(x0, y0, x1, y1, fill="#%02x%02x%02x" % (r, g, b), outline="")


if __name__ == "__main__":
    module_name = os.environ.get("PREVIEW_EFFECT_MODULE", "effects.sample_effect")
    class_name = os.environ.get("PREVIEW_EFFECT_CLASS", "SampleEffect")
    module = importlib.import_module(module_name)
    effect_cls = getattr(module, class_name)

    root = tk.Tk()
    root.title("LED Effect Preview")
    root.geometry("900x260")
    PreviewWindow(root, effect_cls())
    root.mainloop()
