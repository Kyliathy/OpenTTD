"""Review retained dated generator originals on the verified native world floor.

@file convert_generator_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups55-56.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Original pixels are sampled and losslessly partitioned; runtime activation remains separate.
"""

import argparse
import hashlib
import json

from PIL import Image, ImageDraw

import convert_tube_art as tube


RECORD = tube.ROOT / "media/multilayer/source/ground/geothermal-generation-record.json"
CANVAS = (320, 224)
CELLS = [[x, y] for y in range(4) for x in range(4)]
ORIGINS = [(129 + 32 * (y - x), 31 + 16 * (x + y)) for x, y in CELLS]
FLOOR = [[160, 64], [288, 128], [160, 192], [32, 128]]
MODELS = {"facility.vent-powerhouse": ("VentTap", 1980), "facility.magma-powerhouse": ("MagmaLoop", 2000)}


def indexed_canvas(size):
    """Create an empty native DOS canvas without inventing any source pixels.

    @param size Actual native canvas dimensions, independent of an original's scale.
    @return Indexed image with the shared palette and transparent index zero.
    """
    canvas = Image.new("P", size, 0)
    canvas.putpalette(tube.base.PALETTE_BYTES)
    canvas.info["transparency"] = 0
    return canvas


def registration_signature(source):
    """Identify the reviewed original pixels and integer native placement.

    @param source Retained whole model/view metadata from the shared original record.
    @return SHA256 of the exact source geometry/selection, excluding workflow status.
    """
    fields = ("key", "asset_id", "direction", "path", "sha256", "native_crop",
              "native_subject_size", "native_position", "native_floor_corners", "native_geometry")
    selected = {field: source[field] for field in fields}
    return hashlib.sha256(json.dumps(selected, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def native_base(key, document=None, spec=None):
    """Sample one checked original into its complete native world-floor canvas.

    @param key Explicit retained original/view registration, never the newest cache file.
    @param document Optional decoded source record for isolated refusal checks.
    @param spec Optional corresponding public family row for isolated contract checks.
    @return Indexed whole model and checked original registration.
    @throws ValueError If provenance, geometry, crop or native registration disagrees.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    source = tube.retained_candidate(key, document)
    asset_id = source["asset_id"]
    if asset_id not in MODELS:
        raise ValueError("generator original belongs to another source family")
    if spec is None:
        public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
        spec = next(row for row in public["assets"] if row["asset_id"] == asset_id)
    model, year = MODELS[asset_id]
    geometry = spec.get("geometry_notes", {}).get("native_generator_geometry", {})
    expected = {"footprint": [4, 4], "tile_cells": CELLS, "cell_size": [64, 64], "anchor": [-31, -33]}
    # A whole original represents one physical industry across sixteen native
    # cells. Neither raster registration nor future metadata grants construction.
    if (spec["asset_id"] != asset_id or spec["consumer"] != 18 or spec["footprint"] != [4, 4]
            or spec["native_size"] != [64, 64] or spec["anchor"] != [-31, -33]
            or spec["geometry"] != "per_tile" or spec["tile_variants"] != 16
            or source.get("native_geometry") != expected or source.get("native_floor_corners") != FLOOR
            or geometry.get("model") != model or geometry.get("introduction_year") != year
            or geometry.get("world_cells") != CELLS or geometry.get("construction_stages") != [0, 1, 2, 3]):
        raise ValueError("generator source differs from verified native geometry")
    # A view excluded during source review cannot re-enter through an otherwise
    # valid crop or hash; directions are genuine renders, never raster rotations.
    if source.get("direction") not in spec["directions"] or source["direction"] in source.get("excluded_views", []):
        raise ValueError("generator original view is missing or excluded")
    approved = spec.get("geometry_notes", {}).get("approved_generator_registrations", {}).get(key)
    # Once a pose is reviewed, a valid raw hash alone cannot authorize shifting
    # or rescaling its machinery for later state composition or native packing.
    if approved is not None and registration_signature(source) != approved:
        raise ValueError("generator native registration differs from reviewed original")
    width, height = source["native_subject_size"]
    x, y = source["native_position"]
    # The measured whole-source scale preserves the 256x128 floor and low
    # machinery height. Exact per-cell fit is independently checked below.
    if not (width == 256 and 128 <= height <= 160 and x in (32, 33) and 31 <= y <= 64 and y + height <= 192):
        raise ValueError("generator original leaves native floor envelope")
    with Image.open(tube.base.repository_path(source["path"])) as original:
        original = original.convert("RGBA")
        left, top, right, bottom = source["native_crop"]
        # Crops must select complete original models within real source bytes;
        # transparent padding cannot conceal a missing edge or invalid window.
        if not (0 <= left < right <= original.width and 0 <= top < bottom <= original.height):
            raise ValueError("generator crop leaves immutable original")
        selected = original.crop((left, top, right, bottom))
    canvas = Image.new("RGBA", CANVAS)
    canvas.paste(selected.resize((width, height), Image.Resampling.BOX), (x, y))
    return tube.base.quantize(canvas, {}), source


def partition_model(campus, source):
    """Give every original indexed pixel exactly one real native cell owner.

    @param campus Checked whole original on its native320x224 canvas.
    @param source Checked immutable source identity and measured registration.
    @return Sixteen64x64 indexed pieces and lossless partition provenance.
    @throws ValueError If any pixel escapes, duplicates or leaves a physical cell absent.
    """
    # Native partitioning accepts only the canonical indexed canvas produced
    # by source conversion, so another image mode cannot reinterpret pixel zero.
    if campus.mode != "P" or campus.size != CANVAS:
        raise ValueError("generator whole canvas changes actual native projection")
    pieces = [indexed_canvas((64, 64)) for unused in CELLS]
    # Nearest physical floor contact assigns visible source pixels once, across
    # the overlapping native canvases. It creates no independent tile machine.
    for py in range(campus.height):
        for px in range(campus.width):
            value = campus.getpixel((px, py))
            if value == 0:
                continue
            eligible = [index for index, (ox, oy) in enumerate(ORIGINS) if 0 <= px - ox < 64 and 0 <= py - oy < 64]
            if not eligible:
                raise ValueError(f"generator original pixel leaves native cells: {px},{py}")
            owner = min(eligible, key=lambda index: (abs(px - ORIGINS[index][0] - 31)
                                                    + 2 * abs(py - ORIGINS[index][1] - 48), index))
            ox, oy = ORIGINS[owner]
            pieces[owner].putpixel((px - ox, py - oy), value)
    rebuilt = indexed_canvas(CANVAS)
    # Reconstruct stored pieces independently: a simultaneous whole-image copy
    # would conceal wrong offsets, duplicate ownership or dropped original pixels.
    for piece, (ox, oy) in zip(pieces, ORIGINS):
        for ly in range(64):
            for lx in range(64):
                value = piece.getpixel((lx, ly))
                if value == 0:
                    continue
                point = (ox + lx, oy + ly)
                if rebuilt.getpixel(point) != 0:
                    raise ValueError("generator cells duplicate original pixels")
                rebuilt.putpixel(point, value)
    # All sixteen floor cells must remain visible and the complete original
    # must survive. A giant single-cell fallback cannot satisfy source coverage.
    if rebuilt.tobytes() != campus.tobytes() or any(piece.getbbox() is None for piece in pieces):
        raise ValueError("generator partition loses original pixels or native cells")
    provenance = dict(source)
    provenance.update({"cell_order": CELLS, "native_cell_origins": [list(point) for point in ORIGINS],
                       "partition_policy": "unique_nearest_native_floor_contact_within_actual_cell_canvas",
                       "original_native_sha256": hashlib.sha256(tube.indexed_png(campus)).hexdigest(),
                       "cell_opaque_pixels": [sum(value != 0 for value in piece.get_flattened_data()) for piece in pieces]})
    return pieces, provenance


def ready_models(asset_id, document=None, public=None):
    """Resolve and verify all four genuine completed-equipment source views.

    @param asset_id Exact dated generator family, independent of runtime industry IDs.
    @param document Optional decoded retained source record for isolated refusal checks.
    @param public Optional decoded public Sprite Contract from the same operation.
    @return Ordered direction mapping to native whole image, lossless pieces and provenance.
    @throws ValueError If any direction is absent, duplicated, foreign or misregistered.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    if public is None:
        public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in document["families"] if row["asset_id"] == asset_id)
    spec = next(row for row in public["assets"] if row["asset_id"] == asset_id)
    selections = family.get("whole_view_selections", {})
    # Every physical orientation needs its own selected original registration;
    # repeating a source key cannot masquerade as another actual model view.
    if set(selections) != {"NE", "SE", "SW", "NW"} or len(set(selections.values())) != 4:
        raise ValueError("generator needs four distinct selected view registrations")
    models = {}
    # All views share one physical contract and retain newly visible faces.
    # Pixel signatures independently reject a duplicate raster under a new key.
    for direction in ("NE", "SE", "SW", "NW"):
        campus, source = native_base(selections[direction], document, spec)
        if source["direction"] != direction:
            raise ValueError("generator selected source has the wrong actual direction")
        pieces, provenance = partition_model(campus, source)
        models[direction] = (campus, pieces, provenance)
    if len({row[2]["original_native_sha256"] for row in models.values()}) != 4:
        raise ValueError("generator views repeat the same native original")
    return models


def review_ready_views(asset_id):
    """Bake a combined native/enlarged continuity review of the four selected views.

    @param asset_id Exact dated generator family whose completed hardware is selected.
    @return Review receipt path; source states and loaded runtime remain pending.
    """
    models = ready_models(asset_id)
    tube.REVIEW.mkdir(parents=True, exist_ok=True)
    stem = MODELS[asset_id][0].lower() + "-ready-views"
    page = Image.new("RGB", (1960, 1040), (235, 239, 243))
    draw = ImageDraw.Draw(page)
    # Adjacent panels compare the same fixed projection at native and2x size.
    # Any view-dependent footprint drift remains visible before state authoring.
    for index, (direction, (campus, pieces, source)) in enumerate(models.items()):
        ox, oy = (index % 2) * 980, (index // 2) * 520
        draw.text((ox + 12, oy + 12), asset_id + " / " + direction + " / " + source["key"], fill=(30, 40, 50))
        rgba = campus.convert("RGBA")
        large = rgba.resize((640, 448), Image.Resampling.NEAREST)
        page.paste(large, (ox + 8, oy + 30), large.getchannel("A"))
        page.paste(rgba, (ox + 650, oy + 50), rgba.getchannel("A"))
        draw.text((ox + 660, oy + 300), "Actual native320x224 whole view", fill=(30, 40, 50))
        draw.text((ox + 660, oy + 324), "Sixteen64x64 sections; anchor(-31,-33)", fill=(30, 40, 50))
    target = tube.REVIEW / (stem + ".png")
    page.save(target)
    receipt = tube.REVIEW / (stem + ".json")
    report = {"_documentation": "r2mlvs13-graphics-and-asset-pipeline.md task55.2/56.2; actual whole-model continuity review",
              "asset_id": asset_id, "native_size": [64, 64], "anchor": [-31, -33], "footprint": [4, 4],
              "views": {direction: source for direction, (unused, unused_parts, source) in models.items()},
              "contact": {"path": target.relative_to(tube.ROOT).as_posix(), "sha256": hashlib.sha256(target.read_bytes()).hexdigest()},
              "source_coverage_complete": False, "runtime_activated": False}
    receipt.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return receipt


def review_model(key):
    """Bake a native/full-cell visual review without claiming complete state source.

    @param key Explicit whole model/view candidate whose immutable bytes are retained.
    @return Model review receipt path with native/contact/partition artifact identities.
    """
    campus, source = native_base(key)
    pieces, partition = partition_model(campus, source)
    tube.REVIEW.mkdir(parents=True, exist_ok=True)
    native_path = tube.REVIEW / (key + "-native.png")
    contact_path = tube.REVIEW / (key + "-contact.png")
    cells_path = tube.REVIEW / (key + "-cells.png")
    native_path.write_bytes(tube.indexed_png(campus))
    rgba = campus.convert("RGBA")
    visible = Image.new("RGB", CANVAS, (235, 239, 243))
    visible.paste(rgba, mask=rgba.getchannel("A"))
    outline = visible.copy()
    pen = ImageDraw.Draw(outline)
    # World-cell outlines reveal the actual floor registration independently
    # of whole-model appeal; each real native cell remains a distinct section.
    for x, y in CELLS:
        cx, cy = 160 + 32 * (y - x), 80 + 16 * (x + y)
        pen.line([(cx, cy - 16), (cx + 32, cy), (cx, cy + 16), (cx - 32, cy), (cx, cy - 16)], fill=(30, 160, 185))
    page = Image.new("RGB", (1320, 780), (235, 239, 243))
    draw = ImageDraw.Draw(page)
    draw.text((14, 12), source["asset_id"] + " / " + source["direction"] + " / BASE REVIEW: 4x4, sixteen actual native cells", fill=(30, 40, 50))
    page.paste(visible.resize((960, 672), Image.Resampling.NEAREST), (12, 42))
    page.paste(outline, (984, 48))
    page.paste(visible, (984, 310))
    draw.text((984, 562), "Native64x64 / anchor(-31,-33)", fill=(30, 40, 50))
    draw.text((984, 586), "Whole source and runtime remain pending", fill=(30, 40, 50))
    page.save(contact_path)
    cells_page = Image.new("RGB", (1080, 1080), (235, 239, 243))
    cells_draw = ImageDraw.Draw(cells_page)
    # Show every lossless cell at4x with its physical coordinate. These crops
    # are source sections, not sixteen independently generated plant designs.
    for index, (piece, cell) in enumerate(zip(pieces, CELLS)):
        px, py = (index % 4) * 270, (index // 4) * 270
        large = piece.convert("RGBA").resize((256, 256), Image.Resampling.NEAREST)
        cells_page.paste(large, (px, py + 14), large.getchannel("A"))
        cells_draw.text((px + 4, py), "WORLD " + str(cell), fill=(30, 40, 50))
    cells_page.save(cells_path)
    report = {"_documentation": "r2mlvs13-graphics-and-asset-pipeline.md Groups55-56; native original/model review only",
              "source": partition, "source_coverage_complete": False, "runtime_activated": False,
              "artifacts": [{"path": path.relative_to(tube.ROOT).as_posix(), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
                            for path in (native_path, contact_path, cells_path)]}
    receipt = tube.REVIEW / (key + "-review.json")
    receipt.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return receipt


def main():
    """Review explicitly selected originals without selecting or activating them.

    @return None; print the exact review receipt path after all partition checks pass.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--candidate", help="Retained dated generator original/view key")
    mode.add_argument("--ready-views", choices=tuple(MODELS), help="Four selected completed-equipment views")
    arguments = parser.parse_args()
    print(review_ready_views(arguments.ready_views) if arguments.ready_views else review_model(arguments.candidate))


if __name__ == "__main__":
    main()
