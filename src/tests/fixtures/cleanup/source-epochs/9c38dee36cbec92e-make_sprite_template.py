"""Draw 4x placement guides and copy style references for the layer sprites.

Guides are RGB PNGs on magenta #FF00FF. The diamond's top point is the
tile's north corner, which is the sprite anchor.
@see docs/zz-reach2/upgrades/2026-10/r2mlm13-layer-art-review-and-new-sprites.md
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import png_indexed

SCALE = 4
MAGENTA = (255, 0, 255)
INK = (32, 32, 32)
ANCHOR = (220, 40, 40)
GUIDE_DIR = "docs/zz-reach2/upgrades/2026-10/assets"
REFERENCE_DIR = "docs/zz-reach2/upgrades/2026-10/assets/reference"

#Contract boxes, drawn at 4x. Anchor is the north corner inside the box.
GUIDES = [
    ("guide_icon_20.png", 20, 20, None),
    ("guide_cursor_32.png", 32, 32, None),
    ("guide_ground.png", 64, 31, (31, 0)),
    ("guide_elevator.png", 64, 64, (31, 33)),
    ("guide_portal.png", 64, 48, (31, 17)),
]

#Crops from the extra GRF, scaled 4x, so the AI sees the existing style.
REFERENCES = [
    ("media/baseset/openttd/tunnel_portals.png", 5, 88, 35, 33, "tunnel_portal.png"),
    ("media/baseset/openttd/roadstops.png", 34, 8, 13, 23, "road_stop.png"),
    ("media/baseset/openttd/tramtracks_bare_depot.png", 80, 0, 62, 64, "depot_building.png"),
    ("media/baseset/openttd/openttdgui_convert_road.png", 0, 0, 56, 32, "toolbar_icon_and_cursor.png"),
]


def _put(pixels, width, height, x, y, colour):
    """Paint one pixel when it lies inside the image.

    @param pixels Mutable RGB buffer.
    @param width Image width.
    @param height Image height.
    @param x Pixel column.
    @param y Pixel row.
    @param colour RGB triple.
    """
    if 0 <= x < width and 0 <= y < height:
        index = (y * width + x) * 3
        pixels[index:index + 3] = bytes(colour)


def _line(pixels, width, height, x0, y0, x1, y1, colour):
    """Draw a one-pixel line.

    @param pixels Mutable RGB buffer.
    @param width Image width.
    @param height Image height.
    @param x0 Start column.
    @param y0 Start row.
    @param x1 End column.
    @param y1 End row.
    @param colour RGB triple.
    """
    steps = max(abs(x1 - x0), abs(y1 - y0), 1)
    for step in range(steps + 1):
        x = x0 + (x1 - x0) * step // steps
        y = y0 + (y1 - y0) * step // steps
        _put(pixels, width, height, x, y, colour)


def _diamond(pixels, width, height, box_w, box_h, anchor):
    """Outline the isometric tile and mark the anchor.

    @param pixels Mutable RGB buffer.
    @param width Image width.
    @param height Image height.
    @param box_w Contract width before scaling.
    @param box_h Contract height before scaling.
    @param anchor North-corner position in contract pixels, or None for a plain box.
    """
    left, top = 0, 0
    right, bottom = box_w * SCALE - 1, box_h * SCALE - 1
    _line(pixels, width, height, left, top, right, top, INK)
    _line(pixels, width, height, right, top, right, bottom, INK)
    _line(pixels, width, height, right, bottom, left, bottom, INK)
    _line(pixels, width, height, left, bottom, left, top, INK)
    if anchor is None:
        return
    ax, ay = anchor[0] * SCALE, anchor[1] * SCALE
    #2:1 diamond. North is the anchor; east and west sit halfway down a 31-tall tile.
    mid_y = ay + (31 * SCALE) // 2
    east_x = ax + 31 * SCALE
    west_x = ax - 31 * SCALE
    south_y = ay + 30 * SCALE
    _line(pixels, width, height, ax, ay, east_x, mid_y, INK)
    _line(pixels, width, height, east_x, mid_y, ax, south_y, INK)
    _line(pixels, width, height, ax, south_y, west_x, mid_y, INK)
    _line(pixels, width, height, west_x, mid_y, ax, ay, INK)
    _line(pixels, width, height, ax - 3, ay, ax + 3, ay, ANCHOR)
    _line(pixels, width, height, ax, max(0, ay - 3), ax, ay + 3, ANCHOR)


def write_guides():
    """Write one guide PNG per contract group.

    @return None.
    """
    os.makedirs(GUIDE_DIR, exist_ok=True)
    for name, box_w, box_h, anchor in GUIDES:
        width, height = box_w * SCALE, box_h * SCALE
        pixels = bytearray(MAGENTA * (width * height))
        _diamond(pixels, width, height, box_w, box_h, anchor)
        png_indexed.write_rgb_png(os.path.join(GUIDE_DIR, name), width, height, bytes(pixels))
        print(f"wrote {name}")


def _scale_crop(path, x, y, crop_w, crop_h):
    """Crop an indexed sprite and enlarge it 4x with nearest neighbours.

    @param path Source PNG.
    @param x Crop left.
    @param y Crop top.
    @param crop_w Crop width.
    @param crop_h Crop height.
    @return Enlarged width, height and RGB bytes.
    """
    src_w, _src_h, colour_type, palette, pixels = png_indexed.read_png(path)
    if colour_type != 3:
        raise ValueError(f"{path} is not indexed")
    out_w, out_h = crop_w * SCALE, crop_h * SCALE
    out = bytearray(out_w * out_h * 3)
    for oy in range(out_h):
        sy = y + oy // SCALE
        for ox in range(out_w):
            sx = x + ox // SCALE
            index = pixels[sy * src_w + sx]
            red, green, blue = palette[index]
            dest = (oy * out_w + ox) * 3
            out[dest:dest + 3] = bytes((red, green, blue))
    return out_w, out_h, bytes(out)


def write_references():
    """Copy tunnel, road-stop and depot sprites at 4x into the reference folder.

    The depot crop is the extra-GRF depot building. The original road depot
    lives in the base GRF, which is not a PNG in this tree.

    @return None.
    """
    os.makedirs(REFERENCE_DIR, exist_ok=True)
    for path, x, y, crop_w, crop_h, name in REFERENCES:
        width, height, rgb = _scale_crop(path, x, y, crop_w, crop_h)
        png_indexed.write_rgb_png(os.path.join(REFERENCE_DIR, name), width, height, rgb)
        print(f"wrote {name}")


def main():
    """Write the guides and the style references.

    @return None.
    """
    write_guides()
    write_references()


if __name__ == "__main__":
    main()
