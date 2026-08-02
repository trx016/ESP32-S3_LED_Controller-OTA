import os
import tkinter as tk

from preview_ui import PreviewUI


if __name__ == "__main__":
    root = tk.Tk()
    PreviewUI(root, None, os.path.dirname(__file__))
    root.mainloop()
