"""Losslessly partition approved orbital originals into real native world-cell parts.

@file orbital_tile_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13F
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group71.2
Feature adapters own approved geometry, originals and foreground masks. This
helper assigns each original pixel once; it creates no artwork or gameplay state.
"""
import hashlib
import json

from PIL import Image
import convert_space_art as art

PARTS = ("back", "front")


def signature(value):
    """Bind a reviewed source policy to its exact canonical metadata.

    @param value Approved feature-owned geometry, source and foreground policy.
    @return Canonical SHA256; this signature grants no native permission.
    """
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode("utf-8")).hexdigest()


def indexed_canvas(size):
    """Allocate transparent canonical storage for original native source pixels.

    @param size Exact approved whole or tile canvas dimensions.
    @return DOS-indexed image with transparent index0.
    """
    image = Image.new("P", size, 0)
    image.putpalette(art.base.PALETTE_BYTES)
    image.info["transparency"] = 0
    return image


def tile_owner(x, y, geometry):
    """Assign one source pixel to the nearest eligible native floor sorting contact.

    @param x Whole-model native source column.
    @param y Whole-model native source row.
    @param geometry Feature-verified real tile origins, dimensions and sorting contact.
    @return Stable world-cell index, breaking equal distances in published tile order.
    @throws ValueError On a source pixel outside all actual native tile canvases.
    """
    width, height = geometry["native_cell"]
    origins = geometry["native_cell_origins"]
    eligible = [i for i, (ox, oy) in enumerate(origins) if 0 <= x - ox < width and 0 <= y - oy < height]
    if not eligible:
        raise ValueError("orbital original pixel leaves actual tile canvases")
    cx, cy = geometry["native_sort_contact"]
    distances = [(abs(x - origins[i][0] - cx) + 2 * abs(y - origins[i][1] - cy), i) for i in eligible]
    return min(distances)[1]


def reassemble(pieces, geometry, foreground, expected_hash, state, frame=0):
    """Independently read stored part pixels through approved real world-cell origins.

    @param pieces Complete metadata/image pairs for all actual back/front cells.
    @param geometry Feature-verified immutable whole/tile geometry.
    @param foreground Exact original pixel coordinates belonging above a visiting craft.
    @param expected_hash Approved whole original indexed-pixel SHA256.
    @param state Exact physical source state being reconstructed.
    @param frame Declared native frame, including explicit still aliases.
    @return Reconstructed original image without duplicated, moved or missing pixels.
    @throws ValueError On geometry, pose, palette, coverage, foreground or original-pixel drift.
    """
    expected = {(tile, part) for tile in range(len(geometry["world_cells"])) for part in PARTS}
    actual = [(row["tile_variant"], row["part"]) for row, image in pieces]
    # Every real cell needs both unambiguous depth entries, including honestly
    # empty fronts. Duplicates or missing entries cannot count as source coverage.
    if len(actual) != len(expected) or set(actual) != expected:
        raise ValueError("missing or duplicate orbital native tile part")
    whole = indexed_canvas(tuple(geometry["native_canvas"]))
    foreground = {tuple(point) for point in foreground}
    observed_front = set()
    width, height = geometry["native_cell"]
    # Inspect stored pixels through actual native origins. A plausible local
    # checksum or metadata label cannot hide shifted seams or a different state.
    for row, image in pieces:
        tile = row["tile_variant"]
        origin = geometry["native_cell_origins"][tile]
        if (row["asset_id"] != geometry["asset_id"] or row["state"] != state or row["frame"] != frame
                or row["direction"] != "none" or row["native_layout"] != 0
                or row["world_cell"] != geometry["world_cells"][tile] or row["native_origin"] != origin
                or row["anchor"] != geometry["anchor"] or image.mode != "P"
                or list(image.size) != geometry["native_cell"]
                or bytes(image.getpalette()) != bytes(art.base.PALETTE_BYTES)):
            raise ValueError("orbital stored tile geometry, pose or palette changed")
        count = sum(value != 0 for value in image.get_flattened_data())
        # Source absence must describe the stored pixels, rather than hide a
        # missing physical back or claim transparent foreground as real hardware.
        if row["structurally_absent"] != (count == 0) or row["opaque_pixels"] != count:
            raise ValueError("orbital absent-part metadata differs from original pixels")
        if hashlib.sha256(image.tobytes()).hexdigest() != row["native_sha256"]:
            raise ValueError("orbital stored tile pixels changed")
        for y in range(height):
            for x in range(width):
                value = image.getpixel((x, y))
                if not value:
                    continue
                point = (origin[0] + x, origin[1] + y)
                # Recombining the right colours is insufficient if a cavity
                # becomes foreground or hardware moves to another sorting cell.
                if tile_owner(*point, geometry) != tile or ((point in foreground) != (row["part"] == "front")):
                    raise ValueError("orbital cuts change physical cell or foreground ownership")
                if whole.getpixel(point):
                    raise ValueError("orbital native cuts duplicate original pixels")
                whole.putpixel(point, value)
                if row["part"] == "front":
                    observed_front.add(point)
    if observed_front != foreground:
        raise ValueError("orbital foreground policy claims absent original hardware")
    if hashlib.sha256(whole.tobytes()).hexdigest() != expected_hash:
        raise ValueError("orbital cuts differ from accepted whole pixels")
    return whole


def cut_cells(original, geometry, foreground, state, frame=0):
    """Cut one approved original using fixed real cells and genuine depth ownership.

    @param original Complete indexed original in the approved whole-model canvas.
    @param geometry Feature-verified immutable native world-cell geometry.
    @param foreground Exact original hardware pixels approved as foreground.
    @param state Physical source state; this helper never chooses gameplay state.
    @param frame Declared frame; callers own permitted phases and static aliases.
    @return All real metadata/image pairs, independently verified against the original.
    @throws ValueError On invalid palette/canvas, absent foreground, clipping or lost world cells.
    """
    # Geometry/palette mismatches cannot be disguised as a new tiling scheme;
    # only original DOS pixels in the feature's approved canvas enter cutting.
    if (original.mode != "P" or list(original.size) != geometry["native_canvas"]
            or bytes(original.getpalette()) != bytes(art.base.PALETTE_BYTES)):
        raise ValueError("orbital whole original canvas or palette changed")
    foreground = {tuple(point) for point in foreground}
    # Foreground must be real original hardware, including valid coordinates;
    # neither transparent reservations nor points outside the source are art.
    if any(not (0 <= x < original.width and 0 <= y < original.height) or not original.getpixel((x, y))
           for x, y in foreground):
        raise ValueError("orbital foreground includes absent original hardware")
    images = {(tile, part): indexed_canvas(tuple(geometry["native_cell"]))
              for tile in range(len(geometry["world_cells"])) for part in PARTS}
    # Assign each opaque original pixel exactly once. Actual world tiles own
    # the sorting cuts; no cell receives another copy of the whole machine.
    for y in range(original.height):
        for x in range(original.width):
            value = original.getpixel((x, y))
            if not value:
                continue
            tile = tile_owner(x, y, geometry)
            ox, oy = geometry["native_cell_origins"][tile]
            part = "front" if (x, y) in foreground else "back"
            images[tile, part].putpixel((x - ox, y - oy), value)
    pieces = []
    # Every paid footprint cell retains real back artwork; empty front entries
    # explicitly disclose the lack of physical foreground rather than fake art.
    for (tile, part), image in images.items():
        count = sum(value != 0 for value in image.get_flattened_data())
        if part == "back" and count == 0:
            raise ValueError("orbital source omits a real footprint back")
        row = dict(asset_id=geometry["asset_id"], state=state, frame=frame, direction="none", native_layout=0,
                   tile_variant=tile, world_cell=list(geometry["world_cells"][tile]), part=part,
                   native_origin=list(geometry["native_cell_origins"][tile]), anchor=list(geometry["anchor"]),
                   structurally_absent=count == 0, opaque_pixels=count,
                   native_sha256=hashlib.sha256(image.tobytes()).hexdigest())
        pieces.append((row, image))
    reassemble(pieces, geometry, foreground, hashlib.sha256(original.tobytes()).hexdigest(), state, frame)
    return pieces
