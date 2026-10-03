"""Quantise layer art onto the DOS palette and emit Action 5 sprite lines.

Magenta #FF00FF becomes index 0. Output is deterministic: same pixels,
same compression, same bytes.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import png_indexed

PALETTE_FILE = "docs/zz-reach2/upgrades/2026-10/assets/dos_palette.txt"
SHEET = "media/baseset/openttd/map_layers.png"
PREVIEW = "docs/zz-reach2/upgrades/2026-10/assets/preview.png"
GAP = 2
MAGENTA = (255, 0, 255)

#Contract order. rel is the anchor before transparent margins are cropped.
CONTRACT = []
for offset in range(44):
    if offset <= 4:
        CONTRACT.append((20, 20, 0, 0))
    elif offset <= 6:
        CONTRACT.append((32, 32, 0, 0))
    elif offset <= 11:
        CONTRACT.append((64, 31, -31, 0))
    elif offset <= 27:
        CONTRACT.append((64, 64, -31, -33))
    else:
        CONTRACT.append((64, 48, -31, -17))

#Filled as raw art arrives. Each item is offset, path, and a source box
#(x, y, w, h) inside that file. An empty list means the art is not staged.
MANIFEST = []


def load_palette(path):
    """Read the DOS palette and the forbidden index ranges.

    @param path dos_palette.txt.
    @return 256 RGB triples and a set of indices that must not be chosen.
    """
    colours = [None] * 256
    forbidden = set()
    for line in open(path, encoding="utf-8"):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("forbid"):
            span = line.split()[1]
            start, end = span.split("-")
            forbidden.update(range(int(start), int(end) + 1))
            continue
        index, red, green, blue = (int(part) for part in line.split())
        colours[index] = (red, green, blue)
    if any(colour is None for colour in colours):
        raise SystemExit("palette file is missing colours")
    return colours, forbidden


def _rgb_of(pixels, width, colour_type, palette, x, y):
    """One source pixel as RGB, with magenta treated as empty.

    @param pixels Flat pixel buffer.
    @param width Source width.
    @param colour_type 2 for RGB, 3 for indexed.
    @param palette Indexed palette, or None.
    @param x Column.
    @param y Row.
    @return RGB triple, or None when the pixel is the magenta key.
    """
    if colour_type == 3:
        colour = palette[pixels[y * width + x]]
    else:
        index = (y * width + x) * 3
        colour = tuple(pixels[index:index + 3])
    #Exact magenta is the key. JPEG raws drift, so a near-magenta pixel is empty too.
    if colour[0] >= 220 and colour[2] >= 220 and colour[1] <= 40:
        return None
    return colour


def downscale(pixels, width, height, colour_type, palette, box, dest_w, dest_h):
    """Area-filter a source box down to the contract size.

    @param pixels Flat pixel buffer.
    @param width Source width.
    @param height Source height.
    @param colour_type PNG colour type.
    @param palette Indexed palette, or None.
    @param box Source rectangle (x, y, w, h).
    @param dest_w Contract width.
    @param dest_h Contract height.
    @return List of RGB triples or None for a keyed pixel, row major.
    """
    origin_x, origin_y, box_w, box_h = box
    out = []
    for y in range(dest_h):
        y0 = origin_y + y * box_h // dest_h
        y1 = origin_y + (y + 1) * box_h // dest_h
        if y1 <= y0:
            y1 = y0 + 1
        for x in range(dest_w):
            x0 = origin_x + x * box_w // dest_w
            x1 = origin_x + (x + 1) * box_w // dest_w
            if x1 <= x0:
                x1 = x0 + 1
            total = [0, 0, 0]
            count = 0
            #Average only the opaque pixels of this output pixel's box.
            for sy in range(y0, min(y1, origin_y + box_h, height)):
                for sx in range(x0, min(x1, origin_x + box_w, width)):
                    colour = _rgb_of(pixels, width, colour_type, palette, sx, sy)
                    if colour is None:
                        continue
                    total[0] += colour[0]
                    total[1] += colour[1]
                    total[2] += colour[2]
                    count += 1
            if count == 0:
                out.append(None)
            else:
                out.append((total[0] // count, total[1] // count, total[2] // count))
    return out


def nearest(colour, palette, forbidden):
    """Closest allowed DOS index for one colour.

    @param colour RGB triple, or None for transparency.
    @param palette 256 RGB triples.
    @param forbidden Indices that must not be chosen.
    @return Palette index.
    """
    if colour is None:
        return 0
    best_index = 1
    best_distance = 1 << 30
    for index, candidate in enumerate(palette):
        if index in forbidden:
            continue
        distance = (colour[0] - candidate[0]) ** 2 + (colour[1] - candidate[1]) ** 2 + (colour[2] - candidate[2]) ** 2
        if distance < best_distance:
            best_distance = distance
            best_index = index
    return best_index


def crop_and_shift(indices, width, height, rel_x, rel_y):
    """Drop transparent margins and move the anchor with them.

    @param indices Contract-sized palette indices.
    @param width Contract width.
    @param height Contract height.
    @param rel_x Contract x offset.
    @param rel_y Contract y offset.
    @return Cropped indices, width, height, rel x and rel y.
    """
    opaque = [index for index, value in enumerate(indices) if value != 0]
    if not opaque:
        return indices, width, height, rel_x, rel_y
    columns = [index % width for index in opaque]
    rows = [index // width for index in opaque]
    left, right = min(columns), max(columns)
    top, bottom = min(rows), max(rows)
    cropped = []
    for y in range(top, bottom + 1):
        start = y * width + left
        cropped.extend(indices[start:start + (right - left + 1)])
    return cropped, right - left + 1, bottom - top + 1, rel_x + left, rel_y + top


def quantise_file(path, box, dest_w, dest_h, rel_x, rel_y, palette, forbidden):
    """Turn one raw image box into one DOS sprite.

    @param path Raw image.
    @param box Source rectangle.
    @param dest_w Contract width.
    @param dest_h Contract height.
    @param rel_x Contract x offset.
    @param rel_y Contract y offset.
    @param palette DOS palette.
    @param forbidden Indices to skip.
    @return Indices, width, height, rel x and rel y.
    """
    width, height, colour_type, source_palette, pixels = png_indexed.read_png(path)
    scaled = downscale(pixels, width, height, colour_type, source_palette, box, dest_w, dest_h)
    indices = [nearest(colour, palette, forbidden) for colour in scaled]
    return crop_and_shift(indices, dest_w, dest_h, rel_x, rel_y)


def build_sheet(sprites):
    """Pack sprites left to right with a two-pixel gap.

    @param sprites Sequence of (indices, width, height).
    @return Sheet width, height, index buffer and the origin of each sprite.
    """
    height = max(sprite[2] for sprite in sprites)
    width = sum(sprite[1] for sprite in sprites) + GAP * (len(sprites) - 1)
    sheet = bytearray(width * height)
    origins = []
    cursor = 0
    for indices, sprite_w, sprite_h in sprites:
        origins.append((cursor, 0))
        for y in range(sprite_h):
            start = y * width + cursor
            row = indices[y * sprite_w:(y + 1) * sprite_w]
            sheet[start:start + sprite_w] = bytes(row)
        cursor += sprite_w + GAP
    return width, height, bytes(sheet), origins


def sprite_lines(origins, sprites):
    """Real-sprite lines in contract order.

    @param origins Sheet position of each sprite.
    @param sprites (indices, width, height, rel x, rel y) per sprite.
    @return NFO lines.
    """
    lines = []
    for (x, y), sprite in zip(origins, sprites):
        _indices, width, height, rel_x, rel_y = sprite
        lines.append(f"   -1 sprites/map_layers.png 8bpp {x:4d} {y:4d} {width:3d} {height:3d} {rel_x:4d} {rel_y:4d} normal")
    return lines


def _synthetic():
    """One magenta image with a grey block, used to prove the writer is stable.

    @return Width, height and RGB bytes.
    """
    width, height = 40, 40
    pixels = bytearray(MAGENTA * (width * height))
    for y in range(10, 30):
        for x in range(10, 30):
            index = (y * width + x) * 3
            pixels[index:index + 3] = b"\x64\x64\x64"
    return width, height, bytes(pixels)


def self_test(palette, forbidden):
    """Quantise a synthetic image twice and compare the PNG bytes.

    @param palette DOS palette.
    @param forbidden Indices to skip.
    @return None. Raises SystemExit when the two files differ.
    """
    folder = "docs/zz-reach2/upgrades/2026-10/assets"
    os.makedirs(folder, exist_ok=True)
    raw_path = os.path.join(folder, "quantize_selftest_raw.png")
    width, height, rgb = _synthetic()
    png_indexed.write_rgb_png(raw_path, width, height, rgb)
    blobs = []
    for name in ("quantize_selftest_a.png", "quantize_selftest_b.png"):
        indices, sprite_w, sprite_h, _rel_x, _rel_y = quantise_file(
            raw_path, (0, 0, width, height), 20, 20, 0, 0, palette, forbidden)
        dest = os.path.join(folder, name)
        png_indexed.write_indexed_png(dest, sprite_w, sprite_h, bytes(indices), palette)
        blobs.append(open(dest, "rb").read())
    if blobs[0] != blobs[1]:
        raise SystemExit("quantise output was not byte-identical")
    print("self-test byte-identical", len(blobs[0]))


def main(argv):
    """Quantise the manifest or run the determinism self-test.

    @param argv Command arguments after the script name.
    @return None.
    """
    palette, forbidden = load_palette(PALETTE_FILE)
    if "--self-test" in argv:
        self_test(palette, forbidden)
        return
    if len(MANIFEST) != 44:
        raise SystemExit("manifest does not list all 44 contract sprites yet")
    sprites = []
    for offset, path, box in sorted(MANIFEST, key=lambda item: item[0]):
        dest_w, dest_h, rel_x, rel_y = CONTRACT[offset]
        indices, sprite_w, sprite_h, rel_x, rel_y = quantise_file(
            path, box, dest_w, dest_h, rel_x, rel_y, palette, forbidden)
        sprites.append((indices, sprite_w, sprite_h, rel_x, rel_y))
    sheet_w, sheet_h, sheet, origins = build_sheet([(item[0], item[1], item[2]) for item in sprites])
    os.makedirs(os.path.dirname(SHEET), exist_ok=True)
    png_indexed.write_indexed_png(SHEET, sheet_w, sheet_h, sheet, palette)
    lines = sprite_lines(origins, sprites)
    sys.stdout.write("\n".join(lines) + "\n")
    if "--preview" in argv:
        preview = bytearray()
        scale = 4
        preview_w, preview_h = sheet_w * scale, sheet_h * scale
        for y in range(preview_h):
            for x in range(preview_w):
                preview.append(sheet[(y // scale) * sheet_w + (x // scale)])
        os.makedirs(os.path.dirname(PREVIEW), exist_ok=True)
        png_indexed.write_indexed_png(PREVIEW, preview_w, preview_h, bytes(preview), palette)
        print(f"wrote {PREVIEW}", file=sys.stderr)


if __name__ == "__main__":
    main(sys.argv[1:])
