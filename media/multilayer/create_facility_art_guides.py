"""Render one-tile depot geometry for facility raster authoring.

@file create_facility_art_guides.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups 23 and 26
@see docs/zz-reach2/upgrades/2026-10/graphics/r2mlvs13-a-sprite-contract.md
These diagrams describe native placement; they never supply game artwork.
"""

from pathlib import Path

from PIL import Image, ImageDraw, PngImagePlugin


ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "media/multilayer/guides/m1"
DOORS = ["NE", "SE", "SW", "NW"]


def create_depot_guide():
    """Show fixed footprints and the visible walls for each real depot entry.

    @return Repository PNG path, with geometry-only owner metadata.
    """
    guide = Image.new("RGB", (1152, 368), (238, 241, 244))
    draw = ImageDraw.Draw(guide)
    draw.text((16, 12), "GEOMETRY ONLY: one square world tile; floor diamond 2:1; native canvas 64x64", fill="black")
    # A far entrance lies behind the visible near walls. These independent
    # views prevent a mirrored near doorway from impersonating a rear view.
    for index, direction in enumerate(DOORS):
        tile = Image.new("RGB", (64, 64), (238, 241, 244))
        pen = ImageDraw.Draw(tile)
        pen.polygon([(31, 33), (63, 48), (31, 63), (0, 48)], fill=(190, 199, 205), outline=(80, 90, 100))
        pen.polygon([(4, 20), (31, 34), (31, 61), (4, 47)], fill=(117, 132, 146))
        pen.polygon([(31, 34), (58, 20), (58, 47), (31, 61)], fill=(84, 103, 119))
        pen.polygon([(31, 6), (58, 20), (31, 34), (4, 20)], fill=(156, 174, 185), outline=(70, 80, 90))
        if direction == "SE":
            pen.polygon([(38, 35), (51, 28), (51, 46), (38, 53)], fill=(28, 35, 43))
        if direction == "SW":
            pen.polygon([(11, 28), (24, 35), (24, 53), (11, 46)], fill=(28, 35, 43))
        guide.paste(tile.resize((256, 256), Image.Resampling.NEAREST), (index * 288 + 16, 50))
        draw.text((index * 288 + 24, 320), direction + (": FAR entry; opaque near wall" if direction in ("NE", "NW") else ": NEAR visible open mouth"), fill="black")
    draw.text((16, 348), "Fixed camera and top-left light. Rail terminates inside ONE depot; no long plaza. Anchor (-31,-33).", fill="black")
    path = ASSETS / "depot-direction-guide.png"
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Groups 23/26; geometry reference only")
    guide.save(path, pnginfo=metadata)
    return path


def create_receiver_guide(kind):
    """Render fixed native receiver cells and their approved authoring roles.

    @param kind Earth or rock, selecting its measured native world layout.
    @return Repository PNG path with world-cell order and geometry metadata.
    """
    if kind == "earth":
        cells = [(0, 0), (0, 1), (0, 2), (1, 0), (1, 1), (1, 2)]
        roles = ["receiving hopper", "brown soil bank", "retaining wall/drain", "soil belt/pump", "flat receiving yard", "site service hut"]
        columns = 6
    elif kind == "rock":
        cells = [(x, y) for x in range(4) for y in range(4)]
        roles = ["intake hopper", "raw stone pile", "stone belt", "intake yard", "crusher jaws", "sorting drum", "aggregate belt", "rubble pile",
                 "aggregate pile", "residue pile", "dust/filter plant", "retaining wall", "site service hut", "flat paved yard", "stock bank", "drain/settling basin"]
        columns = 4
    else:
        raise ValueError("unknown receiver geometry")
    rows = (len(cells) + columns - 1) // columns
    guide = Image.new("RGB", (columns * 240, rows * 180 + 42), (238, 241, 244))
    draw = ImageDraw.Draw(guide)
    draw.text((12, 10), kind.upper() + " GEOMETRY ONLY: fixed layout; each independent cell is ONE tile / 64x64; no rotation", fill="black")
    # Labeled cells follow the native layout's original column-major order.
    # Artwork stays inside each diamond so assembled neighbours share corners.
    for index, (cell, role) in enumerate(zip(cells, roles)):
        left, top = (index % columns) * 240 + 24, (index // columns) * 180 + 40
        diamond = [(left + 96, top + 20), (left + 191, top + 67), (left + 96, top + 114), (left, top + 67)]
        draw.polygon(diamond, fill=(183, 192, 198), outline=(55, 75, 85))
        draw.text((left, top + 125), f"cell {index}: ({cell[0]},{cell[1]}) {role}", fill="black")
        draw.text((left, top + 144), "south tip aligned; top-left light", fill="black")
    path = ASSETS / (kind + "-receiver-cell-guide.png")
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs13-graphics-and-asset-pipeline.md Groups37-38; geometry reference only")
    metadata.add_text("WorldCells", str(cells))
    guide.save(path, pnginfo=metadata)
    return path


if __name__ == "__main__":
    print(create_depot_guide())
    print(create_receiver_guide("earth"))
    print(create_receiver_guide("rock"))
