"""Assemble tests/manual/gifframes/f*.png into docs/screenshots/demo.gif.

Flat-colour MFC UI -> adaptive octree palette, no dithering (keeps solid fills crisp).
Key frames (dialog, recorded, dark theme) are held longer. Loops forever.
"""
import sys
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "tests" / "manual" / "gifframes"
OUT = ROOT / "docs" / "screenshots" / "demo.gif"

WIDTH = 760            # output width in px (height scales to keep aspect)
CROP_RIGHT = 7         # trim the 1px desktop-bleed sliver on the right edge

# label -> hold time in ms (order is the capture order f01..f10)
HOLD = {
    "main": 1400, "select": 900, "dialog": 1400, "recorded": 1700,
    "undo": 1100, "redo": 1100, "viewtab": 900, "dark": 1800,
    "light": 1100, "loop": 1400,
}

files = sorted(SRC.glob("f*.png"))
if not files:
    sys.exit(f"no frames in {SRC}")

frames, durations = [], []
for f in files:
    label = f.stem.split("-", 1)[1]
    im = Image.open(f).convert("RGB")
    im = im.crop((0, 0, im.width - CROP_RIGHT, im.height))
    h = round(im.height * WIDTH / im.width)
    im = im.resize((WIDTH, h), Image.LANCZOS)
    im = im.quantize(colors=256, method=Image.FASTOCTREE, dither=Image.Dither.NONE)
    frames.append(im)
    durations.append(HOLD.get(label, 1200))

OUT.parent.mkdir(parents=True, exist_ok=True)
frames[0].save(
    OUT, save_all=True, append_images=frames[1:],
    loop=0, duration=durations, disposal=2, optimize=True,
)
kb = OUT.stat().st_size / 1024
print(f"wrote {OUT} ({frames[0].size[0]}x{frames[0].size[1]}, {len(frames)} frames, {kb:.0f} KB)")
