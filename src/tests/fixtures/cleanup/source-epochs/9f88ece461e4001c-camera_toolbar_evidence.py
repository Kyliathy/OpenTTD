"""Arrange unaltered game captures for the toolbar and camera verification records."""

from pathlib import Path
from PIL import Image, ImageDraw

folder = Path("build/toolbar-review")
destination = Path("docs/zz-reach2/upgrades/2026-10")


def sheet(name, frames, columns=2):
    """Place captures below captions without resampling their pixels."""
    sources = [(label, Image.open(folder / filename).convert("RGB")) for label, filename in frames]
    width = max(frame.width for _, frame in sources)
    height = max(frame.height for _, frame in sources) + 24
    board = Image.new("RGB", (columns * width, ((len(sources) + columns - 1) // columns) * height), (64, 64, 64))
    draw = ImageDraw.Draw(board)
    for index, (label, frame) in enumerate(sources):
        x, y = index % columns * width, index // columns * height
        draw.text((x + 4, y + 4), label, fill="white")
        board.paste(frame, (x, y + 24))
    board.save(destination / name)


sheet("camera-save-review.png", [
    ("Expected Surface plane position", "camera-surface-before.png"),
    ("Before correction: Underground save loads at Surface edge", "camera-loaded-before-clean.png"),
    ("After correction: Underground save loads here on Surface", "camera-underground-loaded.png"),
    ("After correction: Deep Underground save loads here on Surface", "camera-deep-loaded.png"),
    ("After correction: Space save loads here on Surface", "camera-space-loaded.png"),
    ("Portal/station fixture round trip on Surface", "portal-roundtrip-surface.png"),
])
sheet("scenario-layer-review.png", [
    ("Editor dropdown, display order", "editor-dropdown.png"),
    ("Surface", "editor-surface.png"),
    ("Underground", "editor-underground.png"),
    ("Deep Underground", "editor-deep.png"),
    ("Outer Space", "editor-space.png"),
])
sheet("subsidies-menu-review.png", [
    ("14-button lower page: Subsidies entry", "14-lower-towns.png"),
    ("Entry opens the subsidies list", "14-subsidies.png"),
    ("15-button lower page: Subsidies entry", "15-lower-towns.png"),
    ("16-button lower page: no duplicate menu entry", "16-lower-towns.png"),
    ("Full toolbar: no duplicate menu entry", "full-towns.png"),
    ("14-button upper page: F6 opens the list", "14-upper-f6.png"),
])
