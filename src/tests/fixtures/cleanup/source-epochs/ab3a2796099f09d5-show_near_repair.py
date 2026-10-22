"""Compare authentic PrintWindow captures at matching game zoom and tile centre."""

from pathlib import Path
from PIL import Image, ImageDraw

folder = Path("build/makinpr2-review")
board = Image.new("RGB", (640, 548), (64, 64, 64))
draw = ImageDraw.Draw(board)
draw.text((8, 8), "Debug game: original foreground on left, repaired foreground on right.", fill="white")
for row, (door, after_name) in enumerate((("sw", "railtunnel-surface-door-sw-tile-50778-z1.png"),
                                        ("se", "railtunnel-surface-door-se-z1.png"))):
    #The old 4x zoom capture is cropped twice as wide, then reduced to the captured 2x zoom.
    before = Image.open(folder / f"rail-{door}-before.png").crop((408, 196, 1048, 676))
    before = before.resize((320, 240), Image.Resampling.NEAREST).convert("RGB")
    after = Image.open(folder / "static-repaired" / after_name).convert("RGB")
    top = 40 + row * 254
    draw.text((8, top), f"Door {door.upper()}: tracks no longer draw across the solid wall.", fill="white")
    board.paste(before, (0, top + 14))
    board.paste(after, (320, top + 14))
board.save("docs/zz-reach2/upgrades/2026-10/assets/reviews/r2mlm13-e-near-front-repair.png")
