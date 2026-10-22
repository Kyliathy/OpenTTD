"""Composite actual paired manifest sprites over transport guides and synthetic vehicles.

@see docs/zz-reach2/upgrades/2026-10/r2mlm6-layer-graphics-assets.md
"""

from pathlib import Path
import sys

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "docs/zz-reach2/upgrades/2026-10/tools"))
import contact_sheet
import sprite_quantize as sprites


def render_pair(offset, door, sprites_by_offset, palette, out):
    """Review the published back/front, without synthesising a different front mask.

    @param offset Back sprite contract offset.
    @param door NE, SE, SW or NW door index.
    @param sprites_by_offset Quantised manifest rows keyed by offset.
    @param palette DOS palette.
    @param out Review PNG path.
    @return None.
    """
    width, height, rel_x, rel_y = sprites.CONTRACT[offset]
    anchor = (-rel_x, -rel_y)
    frame = sprites.SHAFT_FRAME[door ^ 2]
    depths = list(range(0, frame, 2)) + [frame]
    cell_w, cell_h, padding = width + 32, height + 42, 16
    picture = Image.new("RGB", (cell_w * len(depths), cell_h * 2), (64, 64, 64))
    draw = ImageDraw.Draw(picture)
    for transport_index, transport in enumerate(("road", "rail")):
        for index, depth in enumerate(depths):
            ox = index * cell_w + padding
            oy = transport_index * cell_h + padding
            north = [(ox + x, oy + y) for x, y in
                     (sprites.tile_to_sprite(x, y, anchor) for x, y in ((0, 0), (16, 0), (16, 16), (0, 16)))]
            draw.polygon(north, fill=(115, 105, 90))
            #Transport guides expose the doorway's clear width; the coloured box models the vehicle volume.
            ends = ((0, 8), (16, 8)) if door in (0, 2) else ((8, 0), (8, 16))
            road = [sprites.tile_to_sprite(x, y, anchor) for x, y in ends]
            draw.line([(ox + x, oy + y) for x, y in road], fill=(50, 50, 50), width=8)
            if transport == "rail":
                for shift in (-3, 3):
                    draw.line([(ox + x, oy + y + shift) for x, y in road], fill=(190, 190, 180), width=1)
            sprites.paste_sprite(picture, sprites_by_offset[offset], palette, (ox, oy), (rel_x, rel_y))
            if depth < frame:
                cx, cy = ((depth, 8), (8, 16 - depth), (16 - depth, 8), (8, depth))[door]
                long_x = door in (0, 2)
                corners = []
                for dx in (-5 if long_x else -3, 5 if long_x else 3):
                    for dy in (-3 if long_x else -5, 3 if long_x else 5):
                        for z in (0, 6):
                            x, y = sprites.tile_to_sprite(cx + dx, cy + dy, anchor)
                            corners.append((ox + x, oy + y - z))
                draw.polygon(sprites.convex_hull(corners), fill=(255, 200, 0), outline=(255, 255, 128))
            sprites.paste_sprite(picture, sprites_by_offset[offset + 1], palette, (ox, oy), (rel_x, rel_y))
            label = f"{transport} {depth}" if depth < frame else f"{transport} hidden"
            draw.text((index * cell_w + 2, (transport_index + 1) * cell_h - 14), label, fill="white")
    picture.resize((picture.width * 4, picture.height * 4), Image.Resampling.NEAREST).save(out)


def main():
    """Write all sixteen directional pairs' road/rail geometry review strips.

    @return None.
    """
    folder = ROOT / "build/existing-pair-review"
    folder.mkdir(exist_ok=True)
    palette, forbidden = sprites.load_palette(sprites.PALETTE_FILE)
    quantised = {}
    for offset, raw, box, *rest in sprites.MANIFEST[12:44]:
        quantised[offset] = sprites.quantise_file(raw, box, *sprites.CONTRACT[offset], palette, forbidden,
                                                rest[0] if rest else None)
    for family, base in (("elevator-surface", 12), ("elevator-underground", 20),
                         ("rail-surface", 28), ("rail-underground", 36)):
        for door, name in enumerate(("ne", "se", "sw", "nw")):
            render_pair(base + door * 2, door, quantised, palette, folder / f"{family}-{name}.png")
    print("Rendered all 16 published back/front pairs over road and rail geometry guides.")


if __name__ == "__main__":
    main()
