"""Convert retained M1 depots and spoil sites into per-tile native source art.

@file convert_facility_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups 23/26/37/38/79
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group48B shared immutable service enclosure.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint 8.4
Only authored source crops, palette conversion and registered equipment inserts
are performed. Packed IDs, content identity and synchronized state are untouched.
"""

import argparse
import hashlib
import json

from PIL import Image, ImageDraw

import convert_m1_art as base
from convert_portal_art import native_housing
from convert_terrain_art import png_bytes
from sprite_quantize import front_mask


RECORD = base.ASSETS / "facility-generation-record.json"
REPORT = base.OUTPUT / "facility-details-manifest.json"
STATES = ["construction", "ready", "working", "blocked"]


def verify_original(source):
    """Verify immutable original and reference hashes before selecting pixels.

    @param source Provenance record with repository path, hash and references.
    @return None; raise ValueError on a changed source or reference.
    """
    if base.digest(base.repository_path(source["path"])) != source["sha256"]:
        raise ValueError("immutable facility source changed")
    # References include geometry guides and retained earlier originals. Their
    # hashes make a source revision traceable even after a candidate is rejected.
    for reference in source["references"]:
        if base.digest(base.repository_path(reference["path"])) != reference["sha256"]:
            raise ValueError("immutable facility reference changed")


def select_cells(source):
    """Select every declared source cell with explicit, bounded crop coverage.

    @param source Immutable source grid and optional reviewed cell rectangles.
    @return Two-dimensional RGBA cells and flat original crop boxes.
    @throws ValueError If crops overlap, leave the image or omit a cell.
    """
    if "state_sources" in source:
        rows, boxes = [], []
        reused = source["cell_reuse"]
        authored = source["authored_tiles"]
        # Rock-only machinery comes from selected state sheets; shared hut,
        # paving and retaining/drain cells keep the exact Earth originals.
        # The mapping assembles actual native coordinates, never a fake rotation.
        for state_index, state in enumerate(STATES):
            layer = source["state_sources"][state]
            verify_original(layer)
            selected, selected_boxes = select_cells(layer)
            flat = [cell for row in selected for cell in row]
            # A short generated stock sheet keeps its genuine rows; separately
            # retained tail panels fill only missing roles instead of duplicates.
            for extra in layer.get("extras", []):
                verify_original(extra)
                extra_cells, extra_boxes = select_cells(extra)
                flat.extend(cell for row in extra_cells for cell in row)
                selected_boxes.extend(extra_boxes)
            indices = layer["source_indices"]
            if len(indices) != len(authored) or len(set(indices)) != len(indices) or any(index not in range(len(flat)) for index in indices):
                # Missing/duplicate source panels cannot complete independent
                # crusher/stock roles merely by reusing another tile's artwork.
                raise ValueError("receiver authored cell mapping is incomplete")
            row, row_boxes = [], []
            for tile in range(len(source["tile_cells"])):
                if str(tile) in reused:
                    reuse = reused[str(tile)]
                    verify_original(reuse)
                    original, original_boxes = select_cells(reuse)
                    column = reuse["source_column"]
                    original_row = reuse["state_rows"][state_index]
                    row.append(original[original_row][column])
                    row_boxes.append(original_boxes[original_row * reuse["grid"][0] + column])
                else:
                    index = indices[authored.index(tile)]
                    row.append(flat[index])
                    row_boxes.append(selected_boxes[index])
            rows.append(row)
            boxes.extend(row_boxes)
        return rows, boxes
    if "source_cell_boxes" not in source:
        return base.source_cells(source)
    image = Image.open(base.repository_path(source["path"])).convert("RGBA")
    columns, rows = source["grid"]
    boxes = source["source_cell_boxes"]
    if len(boxes) != columns * rows:
        raise ValueError("missing facility source cell")
    # Explicit crops are needed when authored rows have unequal gutters. Every
    # cell must remain independent; overlap would count another view's pixels.
    for index, box in enumerate(boxes):
        left, top, right, bottom = box
        if not (0 <= left < right <= image.width and 0 <= top < bottom <= image.height):
            raise ValueError("facility source cell leaves image")
        if image.crop(box).getchannel("A").getbbox() is None:
            raise ValueError("empty facility source cell")
        for previous in boxes[:index]:
            if max(left, previous[0]) < min(right, previous[2]) and max(top, previous[1]) < min(bottom, previous[3]):
                # Shared source pixels cannot prove independently authored cells.
                raise ValueError("overlapping facility source cells")
    return [[image.crop(boxes[row * columns + column]) for column in range(columns)] for row in range(rows)], boxes


def indicator(source):
    """Reduce genuine authored lamp glass to fixed native equipment components.

    @param source Four-state/four-phase immutable square-lamp component sheet.
    @return State-indexed lists of four opaque 3x3 RGBA lamp phases.
    @throws ValueError If the original or phase coverage is invalid.
    """
    verify_original(source)
    if source["grid"] != [4, 4] or source["states"] != STATES or source["frames"] != 4:
        # The component's rows/columns mean existing facility state and visual
        # phase; a missing lamp cannot be filled with a duplicate phase zero.
        raise ValueError("facility indicator coverage mismatch")
    cells, _boxes = select_cells(source)
    result = []
    # Only the authored inner glass changes. The dark component rim is excluded
    # so an insert cannot replace the selected building's own wall mounting.
    for row in cells:
        phases = []
        for cell in row:
            box = (round(cell.width * .16), round(cell.height * .16), round(cell.width * .84), round(cell.height * .84))
            phase = cell.crop(box).resize((3, 3), Image.Resampling.BOX)
            if phase.getchannel("A").getextrema()[0] < 128:
                raise ValueError("facility lamp glass is transparent")
            phases.append(phase)
        result.append(phases)
    return result


def insert_indicator(housing, component, target):
    """Insert selected lamp pixels while retaining native alpha and contacts.

    @param housing Selected fixed 64x64 RGBA source housing or site cell.
    @param component One genuinely authored opaque 3x3 equipment phase.
    @param target Native x,y,width,height of the lamp, or None for static cells.
    @return New RGBA pose; pixels outside the declared equipment stay exact.
    @throws ValueError If registration leaves the canvas or replaces transparency.
    """
    pose = housing.copy()
    if target is None:
        return pose
    x, y, width, height = target
    if (width, height) != (3, 3) or not (0 <= x <= 61 and 0 <= y <= 61):
        # A status lamp occupies its reviewed wall fitting; changing its size or
        # leaving the tile would alter the building's physical sorting geometry.
        raise ValueError("facility lamp target leaves native fitting")
    # All nine insert pixels are on existing opaque material. Ground edges and
    # holes stay untouched, preventing visual animation from moving contacts.
    for dy in range(3):
        for dx in range(3):
            original = pose.getpixel((x + dx, y + dy))
            if original[3] < 128:
                raise ValueError("facility lamp target is not opaque")
            authored = component.getpixel((dx, dy))
            pose.putpixel((x + dx, y + dy), (*authored[:3], original[3]))
    return pose


def derive(document=None, source_record=RECORD, output_dir=base.OUTPUT):
    """Derive exact state/direction/cell/part/phase coverage from retained art.

    @param document Optional explicit selected source/component records for bounded model reuse.
    @param source_record Actual owning provenance path; defaults to the existing M1 record.
    @param output_dir Actual repository proof directory; defaults to existing M1 derived art.
    @return Native source report and mapping of output paths to PNG bytes.
    @throws ValueError If public geometry or immutable source coverage disagrees.
    """
    if document is None:
        document = json.loads(source_record.read_text(encoding="utf-8"))
    public = json.loads((base.PUBLIC).read_text(encoding="utf-8"))
    families = {row["asset_id"]: row for row in public["assets"]}
    components = {row["key"]: indicator(row) for row in document["components"]}
    identities = [row["asset_id"] for row in document["families"]]
    if len(identities) != len(set(identities)):
        raise ValueError("duplicate facility source family")
    for rejected in document["candidates"]:
        verify_original(rejected)
    outputs, files, cache = [], {}, {}
    # Each family's actual native cells keep their world coordinates. A six- or
    # sixteen-cell receiver is never squeezed into a single building sprite.
    for source in document["families"]:
        verify_original(source)
        family = families[source["asset_id"]]
        directions, tile_cells = source["directions"], source["tile_cells"]
        count = len(tile_cells)
        if family["directions"] != directions or family["states"] != source["states"] or family["frames"] != 4:
            # The same public source contract supplies conversion and later
            # packing; unsupported rotations or new state names cannot slip in.
            raise ValueError("facility state/direction/phase contract mismatch")
        if family["footprint"] != source["footprint"] or family["tile_variants"] != count or family["anchor"] != [-31, -33]:
            # Footprint/anchor belong to the built native object, not its artwork.
            raise ValueError("facility native geometry contract mismatch")
        if source["grid"] != [len(directions) * count, len(STATES)] or len(source["cue_targets"]) != len(directions) * count:
            raise ValueError("facility cell grid is incomplete")
        if len(set(map(tuple, tile_cells))) != count:
            raise ValueError("duplicate facility world cell")
        width, height = source["footprint"]
        if set(map(tuple, tile_cells)) != {(x, y) for y in range(height) for x in range(width)}:
            raise ValueError("facility world-cell footprint has gaps")
        cells, boxes = select_cells(source)
        sheet = Image.new("P", (len(directions) * count * 2 * 64, 16 * 64), 0)
        sheet.putpalette(base.PALETTE_BYTES)
        sprites = []
        phases = components[source["component"]]
        # Every physical cell retains one source housing across visual phases;
        # only a reviewed lamp fitting changes. Static paving/walls deliberately
        # repeat while the family's real equipment supplies phase differences.
        for state_index, state in enumerate(STATES):
            for view, direction in enumerate(directions):
                for tile, world_cell in enumerate(tile_cells):
                    column = view * count + tile
                    fixed = native_housing(cells[state_index][column])
                    for frame in range(4):
                        back = base.quantize(insert_indicator(fixed, phases[state_index][frame], source["cue_targets"][column]), cache)
                        door = ["NE", "SE", "SW", "NW"].index(direction) if direction != "none" else 1
                        keep = front_mask(door, 1, 64, 64, (31, 33), full_span=True)
                        front = back.copy()
                        front.putdata([value if (index % 64, index // 64) in keep else 0 for index, value in enumerate(back.get_flattened_data())])
                        aperture = source.get("front_apertures", {}).get(direction)
                        if aperture is not None and state in ("ready", "working"):
                            # Only real near door mouths are cleared; roofs,
                            # jambs and all far opaque walls retain their cover.
                            ImageDraw.Draw(front).polygon([tuple(point) for point in aperture], fill=0)
                        for part, image in [("back", back), ("front", front)]:
                            x = (column * 2 + (part == "front")) * 64
                            y = (state_index * 4 + frame) * 64
                            sheet.paste(image, (x, y))
                            sprites.append({"asset_id": source["asset_id"], "direction": direction, "tile_variant": tile,
                                            "world_cell": world_cell, "state": state, "frame": frame, "part": part,
                                            "anchor": [-31, -33], "source_cell": [column, state_index], "box": [x, y, 64, 64]})
        path = output_dir / (source["key"] + "-authored.png")
        encoded = png_bytes(sheet)
        files[path] = encoded
        outputs.append({"asset_id": source["asset_id"], "path": path.relative_to(base.ROOT).as_posix(),
                        "sha256": hashlib.sha256(encoded).hexdigest(), "source": source["path"], "source_sha256": source["sha256"],
                        "source_boxes": boxes, "footprint": source["footprint"], "tile_cells": tile_cells, "size": list(sheet.size),
                        "native_cell": [64, 64], "sprites": sprites})
    return {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md Groups23/26/37-38; source art only, native rendering pending",
            "schema_version": 1, "runtime_activated": False, "source_record": source_record.relative_to(base.ROOT).as_posix(),
            "atlases": outputs, "pose_count": sum(len(row["sprites"]) for row in outputs)}, files


def publish(report, files):
    """Publish exact native source art, provenance and per-family review sheets.

    @param report Derived complete family/physical-cell coverage.
    @param files Deterministic encoded native atlas bytes indexed by output path.
    @return None; write repository artifacts without activating runtime payloads.
    """
    base.REVIEW.mkdir(parents=True, exist_ok=True)
    base.OUTPUT.mkdir(parents=True, exist_ok=True)
    for path, encoded in files.items():
        path.write_bytes(encoded)
    REPORT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    source_rows = {row["asset_id"]: row for row in json.loads(RECORD.read_text(encoding="utf-8"))["families"]}
    public_path = base.PUBLIC
    public = json.loads(public_path.read_text(encoding="utf-8"))
    # Source hashes/counts are published beside untouched packing identities;
    # native runtime and actual payload loading remain their later checkpoints.
    for atlas in report["atlases"]:
        source = source_rows[atlas["asset_id"]]
        family = next(row for row in public["assets"] if row["asset_id"] == atlas["asset_id"])
        originals = list(dict.fromkeys(family.get("authoring", {}).get("originals", []) + [source["path"]]))
        family["source"] = {"status": "authored", "path": source["path"], "sha256": source["sha256"],
                            "provenance": "first_party_original_builtin_image_gen", "rights": "first_party_generated_original"}
        family["authoring"] = {"date": source["date"], "status": "source_cell_and_phase_coverage_verified_runtime_pending",
                               "source_record": report["source_record"], "originals": originals,
                               "atlas_manifest": REPORT.relative_to(base.ROOT).as_posix(), "native_atlas": atlas["path"],
                               "native_sha256": atlas["sha256"], "staged_pose_count": len(atlas["sprites"]),
                               "coverage_complete": True, "runtime_activated": False}
        make_review(atlas, source)
    public_path.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def make_review(atlas, source):
    """Assemble native/enlarged views and a contact-stable animated site review.

    @param atlas Derived indexed atlas with explicit physical world-cell placement.
    @param source Selected family geometry, state and direction mapping.
    @return None; write per-family PNG and per-state four-phase GIF artifacts.
    """
    sheet = Image.open(base.repository_path(atlas["path"])).convert("RGBA")
    width, height = source["footprint"]
    canvas_size = ((width + height) * 32, (width + height - 2) * 16 + 64)
    review = Image.new("RGB", (len(source["directions"]) * canvas_size[0] * 3 + 24, len(STATES) * (canvas_size[1] * 3 + 32)), (44, 47, 51))
    draw = ImageDraw.Draw(review)
    # Whole-site assemblies use the same tile projection that the runtime will
    # sort. This reveals missing cells and seams that isolated icons would hide.
    for state_index, state in enumerate(STATES):
        frames = []
        for frame in range(4):
            strip = Image.new("RGB", (len(source["directions"]) * canvas_size[0] * 3, canvas_size[1] * 3), (44, 47, 51))
            for view, direction in enumerate(source["directions"]):
                site = Image.new("RGBA", canvas_size)
                sprites = [row for row in atlas["sprites"] if row["state"] == state and row["frame"] == frame and row["direction"] == direction and row["part"] == "back"]
                for sprite in sorted(sprites, key=lambda row: (sum(row["world_cell"]), row["world_cell"][0])):
                    x, y = sprite["world_cell"]
                    sx, sy, sw, sh = sprite["box"]
                    pixels = sheet.crop((sx, sy, sx + sw, sy + sh))
                    site.alpha_composite(pixels, ((height - 1 + x - y) * 32, (x + y) * 16))
                large = site.resize((canvas_size[0] * 3, canvas_size[1] * 3), Image.Resampling.NEAREST)
                strip.paste(large, (view * canvas_size[0] * 3, 0), large)
            frames.append(strip)
        y = state_index * (canvas_size[1] * 3 + 32)
        draw.text((12, y + 6), atlas["asset_id"] + " " + state + " | " + "/".join(source["directions"]) + " | 4 authored display phases; runtime pending", fill="white")
        review.paste(frames[0], (12, y + 24))
        frames[0].save(base.REVIEW / (source["key"] + "-" + state + "-review.gif"), save_all=True, append_images=frames[1:], duration=240, loop=0, optimize=False)
    review.save(base.REVIEW / (source["key"] + "-details-review.png"))


def check(report, files, asset_id=None):
    """Check regenerated bytes, public coverage and real display phase diversity.

    @param report Fresh expected source report and per-sprite identities.
    @param files Fresh reproducible native atlas bytes.
    @param asset_id Optional exact family to validate in a focused burst.
    @return None; print source handoff evidence or reject stale/invalid artifacts.
    """
    retained = json.loads(REPORT.read_text(encoding="utf-8"))
    public = json.loads((base.PUBLIC).read_text(encoding="utf-8"))
    checked = []
    for atlas in report["atlases"]:
        if asset_id is not None and atlas["asset_id"] != asset_id:
            continue
        path = base.repository_path(atlas["path"])
        # Published pixels and their exact physical-cell mapping must both
        # match retained source evidence before consumers can pack the family.
        if path.read_bytes() != files[path] or atlas not in retained["atlases"]:
            raise ValueError("facility conversion is not reproducible")
        image = Image.open(path)
        # DOS palette/index-zero alpha are shared by every selected M1 family.
        if image.mode != "P" or image.info.get("transparency") != 0 or image.getpalette() != base.PALETTE_BYTES:
            raise ValueError("facility native palette/transparency mismatch")
        if set(image.get_flattened_data()) & (base.FORBIDDEN - {0}):
            raise ValueError("facility uses reserved palette colors")
        family = next(row for row in public["assets"] if row["asset_id"] == atlas["asset_id"])
        expected_count = len(family["directions"]) * len(family["states"]) * family["frames"] * family["tile_variants"] * len(family["parts"])
        keys = {(row["direction"], row["state"], row["frame"], row["tile_variant"], row["part"]) for row in atlas["sprites"]}
        # A duplicate identity cannot fill a missing direction, phase or cell.
        if len(keys) != expected_count or len(atlas["sprites"]) != expected_count:
            raise ValueError("facility source coverage is incomplete")
        # Public source hashes/counts must describe the actual staged payload.
        if family["authoring"]["native_sha256"] != atlas["sha256"] or family["authoring"]["staged_pose_count"] != expected_count:
            raise ValueError("public facility source binding changed")
        # Family phase diversity is measured over the full site; deliberately
        # static paving cells need no fake moving pixel to satisfy this check.
        for direction in family["directions"]:
            for state in STATES:
                signatures = []
                for frame in range(4):
                    data = []
                    for row in atlas["sprites"]:
                        # Only the matching physical rear cells form this
                        # whole-site phase; front overlays are not extra cells.
                        if row["direction"] == direction and row["state"] == state and row["frame"] == frame and row["part"] == "back":
                            x, y, width, height = row["box"]
                            pose = image.crop((x, y, x + width, y + height))
                            if not any(pose.get_flattened_data()):
                                raise ValueError("empty facility cell")
                            data.append(pose.tobytes())
                    signatures.append(b"".join(data))
                if len(set(signatures)) != 4:
                    raise ValueError("facility display phases were duplicated")
        checked.append(atlas)
    if not checked:
        raise ValueError("requested facility family is absent")
    print(f"Facility source conversion: {len(checked)} families / {sum(len(row['sprites']) for row in checked)} cell parts; exact regeneration, public coverage, palette and real phases pass; runtime pending")


def main():
    """Publish or read-only validate the currently selected M1 facility sources.

    @return None; return a failing process status for any source/geometry error.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--family", help="focused exact public asset_id")
    args = parser.parse_args()
    report, files = derive()
    if not args.check:
        publish(report, files)
    check(report, files, args.family)


if __name__ == "__main__":
    main()
