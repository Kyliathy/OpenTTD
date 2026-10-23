"""Quantise layer art onto the DOS palette and emit Action 5 sprite lines.

Magenta #FF00FF becomes index 0. Output is deterministic: same pixels,
same compression, same bytes.
@see docs/zz-reach2/upgrades/2026-10/r2mlm6-layer-graphics-assets.md
@see docs/zz-reach2/upgrades/2026-10/r2mlm13-layer-art-review-and-new-sprites.md
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
"""

import argparse
import json
import math
import os
import re
import sys
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageOps

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import png_indexed

PALETTE_FILE = "docs/zz-reach2/upgrades/2026-10/assets/dos_palette.txt"
SHEET = "media/baseset/openttd/map_layers.png"
PREVIEW = "docs/zz-reach2/upgrades/2026-10/assets/preview.png"
GAP = 2
MAGENTA = (255, 0, 255)

#Contract order: r2mlm6 then r2mlm13. Anchors precede transparent-margin cropping.
CONTRACT_RANGES = [
    (0, 4, 20, 20, 0, 0),
    (5, 6, 32, 32, 0, 0),
    (7, 11, 64, 31, -31, 0),
    (12, 27, 64, 64, -31, -33),
    (28, 43, 64, 48, -31, -17),
    (44, 45, 20, 20, 0, 0),
    (46, 46, 32, 32, 0, 0),
    (47, 49, 64, 31, -31, 0),
    (50, 65, 64, 48, -31, -17),
    #Offsets 66..73 require the user's optional Deep Underground housing decision.
]
CONTRACT = []
for first, last, width, height, rel_x, rel_y in CONTRACT_RANGES:
    CONTRACT.extend([(width, height, rel_x, rel_y)] * (last - first + 1))
assert CONTRACT_RANGES[0][0] == 0
assert all(row[0] == previous[1] + 1 for previous, row in zip(CONTRACT_RANGES, CONTRACT_RANGES[1:]))

#Vanilla shaft visibility frames, indexed NE, SE, SW, NW. A mouth cannot stand deeper.
SHAFT_FRAME = (12, 8, 8, 12)
GUIDE_RED = (220, 40, 40)

#Contract offsets remain stable. None crops icon margins; (part, count) crops
#one floor variant; (part, count, "aligned") retains the shared pair canvas.
RAW = "docs/zz-reach2/upgrades/2026-10/assets/raw/"
MANIFEST = [
    (0, RAW + "layer_icon_surface.png", None),
    (1, RAW + "layer_icon_underground.png", None),
    (2, RAW + "layer_icon_space_v2.png", None),
    (3, RAW + "build_icon_road_elevator_v2.png", None),
    (4, RAW + "build_icon_rail_portal.png", None),
    (5, RAW + "build_icon_road_elevator_v2.png", None),
    (6, RAW + "build_icon_rail_portal.png", None),
    (7, RAW + "underground_floor_sheet.png", (0, 3)),
    (8, RAW + "underground_floor_sheet.png", (1, 3)),
    (9, RAW + "underground_floor_sheet.png", (2, 3)),
    (10, RAW + "space_floor_sheet.png", (0, 2)),
    (11, RAW + "space_floor_sheet.png", (1, 2)),
]
for family, base in (("elevator_surface", 12), ("elevator_underground", 20),
                     ("rail_portal_surface", 28), ("rail_portal_underground", 36)):
    for direction, name in enumerate(("ne", "se", "sw", "nw")):
        for part in range(2):
            MANIFEST.append((base + direction * 2 + part,
                             RAW + f"{family}_{name}_pair_v4.png", (part, 2, "aligned")))

#Keep the original rows replayable through this same pipeline after extending the live contract.
LEGACY_MANIFEST = tuple(MANIFEST)
#Far-facing solid walls and roofs must cover an entering vehicle, not just the separately generated door frame.
#Select the whole AI-generated back panel for those eight live fronts; the legacy replay retains its original panels.
for base in (12, 20, 28, 36):
    for door in (0, 3):
        offset = base + door * 2 + 1
        _offset, path, _box = MANIFEST[offset]
        MANIFEST[offset] = (offset, path, (0, 2, "aligned"), {"part": "front", "door": door})
#Near fronts keep their existing frame and cover solid roof/wall pixels from the same generated back.
for base in (12, 20, 28, 36):
    for door in (1, 2):
        offset = base + door * 2 + 1
        _offset, path, _box = MANIFEST[offset]
        MANIFEST[offset] = (offset, path, (0, 2, "aligned"),
                            {"part": "front", "door": door, "depth": 1,
                             "full_span": True, "frame_panel": (1, 2, "aligned")})
MANIFEST.extend([
    (44, RAW + "layer_icon_deep_underground.png", (74, 109, 1105, 1105)),
    (45, RAW + "build_icon_road_tunnel.png", (56, 70, 1139, 1139)),
    (46, RAW + "build_icon_road_tunnel.png", (78, 93, 1094, 1094)),
    (47, RAW + "deep_floor_sheet_v3.png", (11, 288, 582, 282), {"project_ground": True}),
    (48, RAW + "deep_floor_sheet_v3.png", (609, 288, 582, 282), {"project_ground": True}),
    (49, RAW + "deep_floor_sheet_v3.png", (1207, 286, 583, 284), {"project_ground": True}),
])
#Every direction has its own top-left-lit source. Back/front share a box and a tile anchor.
for base, entries in (
    (50, (
        ("road_tunnel_surface_door_ne.png", (192, 64, 1065, 798)),
        ("road_tunnel_surface_door_se.png", (350, 224, 749, 561)),
        ("road_tunnel_surface_door_sw_v2.png", (349, 222, 749, 561)),
        ("road_tunnel_surface_door_nw.png", (193, 65, 1066, 799)),
    )),
    (58, (
        ("road_tunnel_underground_door_ne.png", (228, 120, 992, 744)),
        ("road_tunnel_underground_door_se.png", (228, 188, 982, 736)),
        ("road_tunnel_underground_door_sw.png", (229, 178, 989, 741)),
        ("road_tunnel_underground_door_nw.png", (229, 120, 992, 744)),
    )),
):
    for door, (name, box) in enumerate(entries):
        for part in range(2):
            options = {"part": "front" if part else "back", "door": door}
            if part and door in (1, 2):
                options["depth"] = 10
            MANIFEST.append((base + door * 2 + part, RAW + name, box, options))


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


def downscale(pixels, width, height, colour_type, palette, box, dest_w, dest_h, mirror=False):
    """Area-filter a source box down to the contract size.

    @param pixels Flat pixel buffer.
    @param width Source width.
    @param height Source height.
    @param colour_type PNG colour type.
    @param palette Indexed palette, or None.
    @param box Source rectangle (x, y, w, h).
    @param dest_w Contract width.
    @param dest_h Contract height.
    @param mirror Read the box right to left, retaining the tile anchor.
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
                    source_x = origin_x + box_w - 1 - (sx - origin_x) if mirror else sx
                    colour = _rgb_of(pixels, width, colour_type, palette, source_x, sy)
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


def quantise_file(path, box, dest_w, dest_h, rel_x, rel_y, palette, forbidden, options=None):
    """Turn one raw image box into one DOS sprite.

    @param path Raw image.
    @param box Source rectangle, None for an opaque crop, or a sheet-panel descriptor.
    @param dest_w Contract width.
    @param dest_h Contract height.
    @param rel_x Contract x offset.
    @param rel_y Contract y offset.
    @param palette DOS palette.
    @param forbidden Indices to skip.
    @param options Optional mirror, part, door, depth, full_span, frame_panel and project_ground. Legacy rows retain their pixels.
    @return Indices, width, height, rel x and rel y.
    """
    options = options or {}
    with Image.open(path) as raw:
        source = raw.convert("RGBA")
    #Key before filtering so background colour cannot contaminate sprite edges.
    red, green, blue, alpha = source.split()
    key = ImageChops.multiply(red.point(lambda value: 255 if value >= 220 else 0),
                             blue.point(lambda value: 255 if value >= 220 else 0))
    key = ImageChops.multiply(key, green.point(lambda value: 255 if value <= 40 else 0))
    source.putalpha(ImageChops.multiply(alpha, ImageOps.invert(key)))
    source = crop_source(source, box)
    if options.get("mirror", False):
        source = ImageOps.mirror(source)
    projected_ground = options.get("project_ground", False)
    if projected_ground:
        if (dest_w, dest_h) != (64, 31):
            raise ValueError("project_ground requires the 64x31 floor contract")
        scaled = project_ground(source)
        #Flat tiles omit their zero-area north/south tips; the first drawn row is one below the north corner.
        rel_y += 1
    else:
        scaled = source.resize((dest_w, dest_h), Image.Resampling.BOX)
    #Floor sprites occupy exactly the tile diamond, with no slab sides off tile.
    if (dest_w, dest_h) == (64, 31) and not projected_ground:
        mask = Image.new("L", (64, 31), 0)
        ImageDraw.Draw(mask).polygon(((31, 0), (63, 15), (31, 30), (0, 15)), fill=255)
        scaled.putalpha(ImageChops.multiply(scaled.getchannel("A"), mask))
    cache = {}
    indices = []
    for red, green, blue, alpha in scaled.get_flattened_data():
        #Ground edge pixels have fractional area coverage but must meet their neighbouring tile without transparent seams.
        colour = (red, green, blue) if alpha >= (1 if projected_ground else 128) else None
        if colour not in cache:
            cache[colour] = nearest(colour, palette, forbidden)
        indices.append(cache[colour])
    #The mirrored front uses its own door mask, so SE keeps the SW picture's depth.
    if options.get("part") == "front":
        keep = front_mask(options["door"], options.get("depth", 1), dest_w, dest_h, (-rel_x, -rel_y),
                          options.get("full_span", False))
        indices = [value if (index % dest_w, index // dest_w) in keep else 0
                   for index, value in enumerate(indices)]
        if "frame_panel" in options:
            frame, frame_w, frame_h, frame_x, frame_y = quantise_file(
                path, options["frame_panel"], dest_w, dest_h, rel_x, rel_y, palette, forbidden,
                {"mirror": options.get("mirror", False)})
            #Retain the generated jambs at their shared anchor; the masked back supplies the solid housing coverage.
            for row in range(frame_h):
                for column in range(frame_w):
                    value = frame[row * frame_w + column]
                    if value:
                        x, y = column + frame_x - rel_x, row + frame_y - rel_y
                        indices[y * dest_w + x] = value
    elif options.get("part", "back") != "back":
        raise ValueError("part must be back or front")
    return crop_and_shift(indices, dest_w, dest_h, rel_x, rel_y)


def project_ground(source):
    """Resample a flat floor to the native 2:1 tile footprint without transparent seams.

    A 16-unit tile spans 64 columns and 32 projected rows. Its half-pixel north
    corner has no area at row 0 or 32; rows 1..31 form the 64x31 real sprite.
    This is opt-in so the original 44 sprites still rebuild byte for byte.
    @param source Keyed RGBA floor variant, cropped to its source diamond.
    @return RGBA floor covering projected rows 1..31; its north-corner offset gains 1.
    """
    scaled = source.resize((64, 33), Image.Resampling.BOX)
    mask = Image.new("L", scaled.size, 0)
    draw = ImageDraw.Draw(mask)
    #At each row the diamond widens by four pixels, then narrows symmetrically after its widest row.
    for y in range(1, 32):
        radius = 2 * min(y, 32 - y)
        left = max(0, math.ceil(31.5 - radius))
        right = min(63, math.floor(31.5 + radius))
        draw.line((left, y, right, y), fill=255)
    scaled.putalpha(ImageChops.multiply(scaled.getchannel("A"), mask))
    return scaled.crop((0, 1, 64, 32))


def crop_source(source, box):
    """Select a source region while retaining aligned back/front registration.

    @param source RGBA image whose transparency has already been keyed.
    @param box None, a four-coordinate rectangle, or a sheet-panel descriptor.
    @return Cropped RGBA image; aligned panels retain their transparent margins.
    """
    aligned = False
    if box is not None:
        if len(box) == 4:
            x, y, width, height = box
            return source.crop((x, y, x + width, y + height))
        part, count = box[:2]
        source = source.crop((source.width * part // count, 0,
                              source.width * (part + 1) // count, source.height))
        aligned = len(box) == 3 and box[2] == "aligned"
    #Only independent icons and floor textures may lose their source margins.
    if not aligned:
        bounds = source.getchannel("A").getbbox()
        if bounds is None:
            raise ValueError("source sprite is entirely transparent")
        source = source.crop(bounds)
    return source


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


def tile_to_sprite(x, y, anchor):
    """Project a ground point at normal zoom, retaining the half-pixel north corner.

    @param x In-tile X, from 0 to 16.
    @param y In-tile Y, from 0 to 16.
    @param anchor North corner in the contract canvas.
    @return Floating-point column and row.
    """
    return anchor[0] + 0.5 + 2 * (y - x), anchor[1] + x + y


def front_mask(door, depth, width, height, anchor, full_span=False):
    """Keep the mouth wall for near doors, or the entire building for far doors.

    @param door Door index NE=0, SE=1, SW=2, NW=3.
    @param depth Mouth inset in tile units, no deeper than the visibility frame.
    @param width Contract width.
    @param height Contract height.
    @param anchor North corner in the contract canvas.
    @param full_span Extend the cut plane across the canvas to retain solid walls beyond the mouth width.
    @return Set of contract pixel coordinates to keep.
    """
    if door not in range(4):
        raise ValueError("door must be 0..3")
    if door in (0, 3):
        return {(column, row) for row in range(height) for column in range(width)}
    shaft = door ^ 2
    if not 1 <= depth <= SHAFT_FRAME[shaft]:
        raise ValueError(f"mouth depth {depth} must be 1..{SHAFT_FRAME[shaft]} for door {door}")
    ends = ((tile_to_sprite(16 - depth, 0, anchor), tile_to_sprite(16 - depth, 16, anchor))
            if door == 2 else
            (tile_to_sprite(0, 16 - depth, anchor), tile_to_sprite(16, 16 - depth, anchor)))
    (left_x, left_y), (right_x, right_y) = sorted(ends)
    keep = set()
    #Pixel centres are integer coordinates around the half-pixel north corner.
    #Rounding both endpoints inward keeps SE and SW masks exact mirrors.
    columns = range(width) if full_span else range(max(0, math.ceil(left_x)), min(width, math.floor(right_x) + 1))
    for column in columns:
        line_row = left_y + (right_y - left_y) * (column - left_x) / max(1.0, right_x - left_x)
        keep.update((column, row) for row in range(max(0, min(height, int(line_row) + 1))))
    return keep


def fit_box(path, offset, region=None):
    """Fit an icon with margin, or a tile subject to its south corner and tile width.

    @param path Raw PNG, including RGB, indexed or alpha images.
    @param offset Contract offset.
    @param region Optional x,y,w,h search region for a floor variant.
    @return Source box and number of subject rows cut from its top.
    """
    with Image.open(path) as raw:
        source = raw.convert("RGBA")
    rx, ry, rw, rh = region or (0, 0, source.width, source.height)
    if rw <= 0 or rh <= 0 or rx < 0 or ry < 0 or rx + rw > source.width or ry + rh > source.height:
        raise ValueError("fit region leaves the picture")
    pixels = source.load()
    left, top, right, bottom = source.width, source.height, 0, 0
    for y in range(ry, ry + rh):
        for x in range(rx, rx + rw):
            red, green, blue, alpha = pixels[x, y]
            #Copied guide crosses and the magenta key must not stretch the subject.
            if alpha < 128 or (red >= 220 and blue >= 220 and green <= 40) or all(
                    abs(c - guide) <= 30 for c, guide in zip((red, green, blue), GUIDE_RED)):
                continue
            left, top, right, bottom = min(left, x), min(top, y), max(right, x + 1), max(bottom, y + 1)
    if right <= left or bottom <= top:
        raise ValueError("fit region contains no subject")
    dest_w, dest_h, _rel_x, _rel_y = CONTRACT[offset]
    if dest_w == dest_h:
        side = (max(right - left, bottom - top) * dest_w + dest_w - 3) // (dest_w - 2)
        box = ((left + right - side) // 2, (top + bottom - side) // 2, side, side)
        cut = 0
    else:
        box_w = right - left
        box_h = box_w * dest_h // dest_w
        box = (left, bottom - box_h, box_w, box_h)
        cut = max(0, box[1] - top)
    if box[0] < 0 or box[1] < 0 or box[0] + box[2] > source.width or box[1] + box[3] > source.height:
        raise ValueError("the box leaves the picture; regenerate with more magenta around the subject")
    return box, cut


def candidate(spec, offset):
    """Parse raw[@x,y,w,h][@mirror][@project-ground], fitting a box when it is omitted.

    @param spec Candidate argument.
    @param offset Contract offset.
    @return Path, source box and mirror option.
    """
    path, *suffixes = spec.split("@")
    box, options = None, {}
    for suffix in suffixes:
        if suffix == "mirror":
            options["mirror"] = True
        elif suffix == "project-ground":
            options["project_ground"] = True
        else:
            box = tuple(int(value) for value in suffix.split(","))
            if len(box) != 4:
                raise ValueError("box must be x,y,w,h")
    if box is None:
        box, cut = fit_box(path, offset)
        print(f"{path}: box {' '.join(map(str, box))} cut {cut}", file=sys.stderr)
    return path, box, options


def sprite_image(sprite, palette):
    """Expand a cropped DOS sprite to RGB on a neutral background.

    @param sprite Indices, width, height, rel x and rel y.
    @param palette DOS palette.
    @return RGB Pillow image.
    """
    indices, width, height, _rel_x, _rel_y = sprite
    result = Image.new("RGB", (width, height))
    result.putdata([palette[index] if index else (96, 96, 96) for index in indices])
    return result


def ground_patch(indices, width, height, rel_x, rel_y):
    """Assemble a 3 by 3 tile patch to expose floor seams.

    @param indices Cropped DOS sprite.
    @param width Cropped width.
    @param height Cropped height.
    @param rel_x North-corner X offset.
    @param rel_y North-corner Y offset.
    @return Patch width, height and flat indices.
    """
    patch_w, patch_h = 256, 96 + height
    patch = [0] * (patch_w * patch_h)
    for i in range(3):
        for j in range(3):
            left, top = 128 + rel_x + 32 * (j - i), rel_y + 16 * (i + j)
            for y in range(height):
                for x in range(width):
                    if indices[y * width + x] and 0 <= top + y < patch_h and 0 <= left + x < patch_w:
                        patch[(top + y) * patch_w + left + x] = indices[y * width + x]
    return patch_w, patch_h, patch


def save_image(image, path):
    """Create the output directory and save a review PNG.

    @param image Pillow image.
    @param path Destination path.
    @return None.
    """
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    image.save(path)


def compare(offset, out, specs, palette, forbidden):
    """Compare candidates at 1x and 4x, plus floor patches where applicable.

    @param offset Contract offset.
    @param out Review PNG path.
    @param specs Raw candidate arguments.
    @param palette DOS palette.
    @param forbidden Forbidden DOS indices.
    @return None.
    """
    width, height, rel_x, rel_y = CONTRACT[offset]
    is_ground = (width, height) == (64, 31)
    #Leave room for the raw file names, so comparison labels cannot overlap.
    label_w = max((len(Path(spec.split("@")[0]).stem) + 15) * 6 for spec in specs)
    cell_w = max(width * 5 + 24, 256 if is_ground else 0, label_w)
    cell_h = height * 4 + 38 + (96 + height if is_ground else 0)
    sheet = Image.new("RGB", (cell_w * len(specs), cell_h), (64, 64, 64))
    draw = ImageDraw.Draw(sheet)
    for index, spec in enumerate(specs):
        path, box, options = candidate(spec, offset)
        sprite = quantise_file(path, box, width, height, rel_x, rel_y, palette, forbidden, options)
        picture = sprite_image(sprite, palette)
        x = index * cell_w
        sheet.paste(picture, (x + 4, 22))
        sheet.paste(picture.resize((picture.width * 4, picture.height * 4), Image.Resampling.NEAREST), (x + width + 12, 22))
        draw.text((x + 4, 3), f"{offset}: {Path(path).stem}" + (" mirror" if options.get("mirror") else ""), fill="white")
        if is_ground:
            patch_w, patch_h, pixels = ground_patch(*sprite)
            patch = sprite_image((pixels, patch_w, patch_h, 0, 0), palette)
            sheet.paste(patch, (x, height * 4 + 38))
    save_image(sheet, out)


def composite(offset, door, depth, out, spec, palette, forbidden):
    """Simulate a bus passing between the whole back and cut front at each depth.

    @param offset Structure contract offset.
    @param door Door direction 0..3.
    @param depth Mouth inset in tile units.
    @param out Strip PNG path.
    @param spec Raw candidate argument.
    @param palette DOS palette.
    @param forbidden Forbidden DOS indices.
    @return None.
    """
    width, height, rel_x, rel_y = CONTRACT[offset]
    anchor = (-rel_x, -rel_y)
    path, box, options = candidate(spec, offset)
    back = quantise_file(path, box, width, height, rel_x, rel_y, palette, forbidden, options)
    front = quantise_file(path, box, width, height, rel_x, rel_y, palette, forbidden,
                          dict(options, part="front", door=door, depth=depth))
    depths = list(range(0, SHAFT_FRAME[door ^ 2], 2)) + [SHAFT_FRAME[door ^ 2]]
    cell_w, cell_h, padding = width + 32, height + 42, 16
    strip = Image.new("RGB", (cell_w * len(depths), cell_h), (64, 64, 64))
    draw = ImageDraw.Draw(strip)
    for index, travelled in enumerate(depths):
        origin_x = index * cell_w + padding
        origin_y = padding
        ground = [tile_to_sprite(x, y, anchor) for x, y in ((0, 0), (16, 0), (16, 16), (0, 16))]
        draw.polygon([(origin_x + x, origin_y + y) for x, y in ground], fill=(120, 120, 120))
        paste_sprite(strip, back, palette, (origin_x, origin_y), (rel_x, rel_y))
        if travelled < SHAFT_FRAME[door ^ 2]:
            cx, cy = ((travelled, 8), (8, 16 - travelled), (16 - travelled, 8), (8, travelled))[door]
            long_x = door in (0, 2)
            corners = []
            for dx in (-5 if long_x else -3, 5 if long_x else 3):
                for dy in (-3 if long_x else -5, 3 if long_x else 5):
                    for z in (0, 6):
                        x, y = tile_to_sprite(cx + dx, cy + dy, anchor)
                        corners.append((origin_x + x, origin_y + y - z))
            draw.polygon(convex_hull(corners), fill=(255, 200, 0), outline=(255, 255, 128))
        paste_sprite(strip, front, palette, (origin_x, origin_y), (rel_x, rel_y))
        draw.text((index * cell_w + 4, cell_h - 15), f"depth {travelled}" if travelled < SHAFT_FRAME[door ^ 2] else "hidden", fill="white")
    save_image(strip.resize((strip.width * 4, strip.height * 4), Image.Resampling.NEAREST), out)


def paste_sprite(image, sprite, palette, origin, contract_rel):
    """Composite a cropped indexed sprite back onto its contract canvas.

    @param image RGB destination.
    @param sprite Cropped sprite and anchor.
    @param palette DOS palette.
    @param origin Destination contract-canvas origin.
    @param contract_rel Uncropped contract anchor offsets.
    @return None.
    """
    indices, width, height, rel_x, rel_y = sprite
    picture = Image.new("RGBA", (width, height))
    picture.putdata([(*palette[value], 255) if value else (0, 0, 0, 0) for value in indices])
    image.paste(picture, (origin[0] + rel_x - contract_rel[0], origin[1] + rel_y - contract_rel[1]), picture)


def convex_hull(points):
    """Return the outer corners of a projected vehicle box.

    @param points Projected corners.
    @return Convex polygon in perimeter order.
    """
    points = sorted(set(points))
    def cross(a, b, c):
        """Signed turn of three points.

        @param a First point.
        @param b Second point.
        @param c Third point.
        @return Signed cross product.
        """
        return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])
    halves = []
    for sequence in (points, list(reversed(points))):
        half = []
        for point in sequence:
            while len(half) >= 2 and cross(half[-2], half[-1], point) <= 0:
                half.pop()
            half.append(point)
        halves.append(half[:-1])
    return halves[0] + halves[1]


def integrated_count():
    """Read the published sprite count; unapproved extensions remain scratch outputs.

    @return C++ MAP_LAYER_SPRITE_COUNT.
    """
    source = Path("src/table/sprites.h").read_text(encoding="utf-8")
    match = re.search(r"MAP_LAYER_SPRITE_COUNT\s*=\s*(\d+)", source)
    if not match:
        raise ValueError("cannot find MAP_LAYER_SPRITE_COUNT in sprites.h")
    return int(match[1])


def main(argv):
    """Quantise the integrated manifest or prepare isolated candidate reviews.

    @param argv Arguments after the script name.
    @return None.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--write-nfo", action="store_true")
    parser.add_argument("--preview", action="store_true")
    parser.add_argument("--out", default=SHEET)
    manifest_args = parser.add_mutually_exclusive_group()
    manifest_args.add_argument("--manifest", help="Candidate JSON rows; publishing still requires the C++ count")
    manifest_args.add_argument("--legacy44", action="store_true", help="Replay the original 44 rows into a scratch --out")
    parser.add_argument("--fit", nargs="+", metavar="ARG")
    parser.add_argument("--compare", nargs="+", metavar="ARG")
    parser.add_argument("--composite", nargs=5, metavar="ARG")
    args = parser.parse_args(argv)
    palette, forbidden = load_palette(PALETTE_FILE)
    if args.self_test:
        self_test(palette, forbidden)
        return
    if args.fit:
        if len(args.fit) not in (2, 3):
            parser.error("--fit needs offset raw [x,y,w,h]")
        offset, raw = int(args.fit[0]), args.fit[1]
        region = tuple(map(int, args.fit[2].split(","))) if len(args.fit) == 3 else None
        box, cut = fit_box(raw, offset, region)
        print("box", *box, "cut", cut)
        return
    if args.compare:
        if len(args.compare) < 3:
            parser.error("--compare needs offset out.png raw ...")
        compare(int(args.compare[0]), args.compare[1], args.compare[2:], palette, forbidden)
        return
    if args.composite:
        offset, door, depth, out, raw = args.composite
        composite(int(offset), int(door), int(depth), out, raw, palette, forbidden)
        return
    manifest = LEGACY_MANIFEST if args.legacy44 else MANIFEST
    if args.manifest:
        manifest = json.loads(Path(args.manifest).read_text(encoding="utf-8"))
    integrated = Path(args.out).resolve() == Path(SHEET).resolve()
    count = integrated_count() if integrated else len(manifest)
    offsets = [row[0] for row in manifest]
    missing = sorted(set(range(count)) - set(offsets))
    unexpected = sorted(set(offsets) - set(range(count)))
    if not manifest or count > len(CONTRACT) or missing or unexpected or len(set(offsets)) != len(offsets):
        raise SystemExit(f"manifest must cover offsets 0..{count - 1} exactly once; missing {missing}, unexpected {unexpected}")
    if args.write_nfo and not integrated:
        parser.error("--write-nfo requires the integrated sheet destination")
    sprites = []
    for offset, path, box, *rest in sorted(manifest, key=lambda row: row[0]):
        sprites.append(quantise_file(path, box, *CONTRACT[offset], palette, forbidden, rest[0] if rest else None))
    sheet_w, sheet_h, sheet, origins = build_sheet([(item[0], item[1], item[2]) for item in sprites])
    if not args.dry_run:
        Path(args.out).parent.mkdir(parents=True, exist_ok=True)
        png_indexed.write_indexed_png(args.out, sheet_w, sheet_h, sheet, palette)
    lines = sprite_lines(origins, sprites)
    sys.stdout.write("\n".join(lines) + "\n")
    if args.write_nfo and not args.dry_run:
        header = Path("media/baseset/openttd/bridge_decks.nfo").read_text(encoding="utf-8").splitlines()[:5]
        header += ["//@see docs/zz-reach2/upgrades/2026-10/r2mlm6-layer-graphics-assets.md, docs/zz-reach2/upgrades/2026-10/r2mlm13-layer-art-review-and-new-sprites.md",
                   '   -1 * 0 0C "Map layer graphics"',
                   "//NFORenum 6.2.0 predates this fork's Action 5 type 1C; suppress only that feature diagnostic.",
                   "//@@WARNING DISABLE 44", f"   -1 * 3 05 1C \\b {count} //MAP_LAYER_SPRITE_COUNT",
                   "//@@WARNING ENABLE 44", ""]
        Path("media/baseset/openttd/map_layers.nfo").write_text("\n".join(header + lines) + "\n", encoding="utf-8")
    if args.preview:
        preview = bytes(sheet[(y // 4) * sheet_w + x // 4] for y in range(sheet_h * 4) for x in range(sheet_w * 4))
        png_indexed.write_indexed_png(PREVIEW, sheet_w * 4, sheet_h * 4, preview, palette)
        print(f"wrote {PREVIEW}", file=sys.stderr)


if __name__ == "__main__":
    main(sys.argv[1:])
