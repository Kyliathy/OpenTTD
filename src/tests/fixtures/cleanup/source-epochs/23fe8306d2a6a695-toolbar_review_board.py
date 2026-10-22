"""Arrange authentic toolbar captures without changing their game-scale pixels."""

from pathlib import Path
from PIL import Image, ImageDraw

folder = Path("build/toolbar-review")
board = Image.new("RGB", (2080, 638), (64, 64, 64))
draw = ImageDraw.Draw(board)
for row, slots in enumerate(range(14, 21)):
    for column, page in enumerate(("upper", "lower")):
        screenshot = Image.open(folder / f"{slots}-{page}.png")
        toolbar = screenshot.crop((8, 31, 8 + slots * 52, 85))
        x, y = column * 1040, row * 80
        draw.text((x + 4, y + 3), f"{slots} buttons, {page} page, client {slots * 52} x 600, interface 250%", fill="white")
        board.paste(toolbar, (x, y + 20))
full = Image.open(folder / "full-visible.png").crop((8, 31, 1758, 85))
draw.text((4, 563), "Single row, client 1750 x 600, interface 250%", fill="white")
board.paste(full, (0, 580))
board.save("docs/zz-reach2/upgrades/2026-10/assets/reviews/toolbar-width-review.png")
