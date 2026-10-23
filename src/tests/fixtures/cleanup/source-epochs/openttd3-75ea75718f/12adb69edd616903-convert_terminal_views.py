"""Register all six genuine native mine-terminal ready views from retained originals.

@file convert_terminal_views.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md task25.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Bounded reuse preserves genuine world orientation and native parent geometry.
No flat raster rotation, copied whole view, source repaint or runtime activation
can substitute for missing native bay/through originals.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw, PngImagePlugin

import convert_terminal_art as terminal
from tools.ground_terminal_contract import packed_anchor


OUTPUT = terminal.OUTPUT
PREFIX = "terminal-ready-views"


def component_record(record, selection):
    """Resolve exactly one immutable original component from its explicit selection.

    @param record Terminal original/provenance document.
    @param selection Native view/part/source/component selection.
    @return Exact original component registration; no source or metadata is changed.
    """
    sources = [row for row in record["candidates"] if row["id"] == selection["source"]]
    if len(sources) != 1:
        raise ValueError("terminal view original is missing or ambiguous")
    components = [row for row in sources[0].get("components", []) if row["id"] == selection["component"]]
    if len(components) != 1:
        raise ValueError("terminal view component is missing or ambiguous")
    return components[0]


def is_housing(layout, index):
    """Distinguish the actual loading housing from its independently sorted safety rails.

    @param layout Compiled native bay/through geometry.
    @param index Actual native sorting parent index.
    @return True for the single housing parent of this physical model.
    """
    return index == (0 if layout["drive_through"] else 1)


def front_visible(graphics):
    """Read the fixed-camera visibility of the genuinely oriented loading facade.

    @param graphics Actual native graphics0..5.
    @return True for +X/+Y loading facades; hidden -X/-Y fronts remain on the far face.
    """
    return graphics in (1, 2, 4)


def validate_reuse(record, selection, layout, geometry):
    """Allow whole-component reuse only at the same world orientation and physical footprint.

    @param record Immutable original selections and explicit component ancestry.
    @param selection Target native source view/parent selection.
    @param layout Target compiled native layout.
    @param geometry Complete six-layout native geometry producer.
    @return None; genuine new originals require no reuse exemption.
    """
    component = component_record(record, selection)
    reuse = component.get("reuse_of")
    if reuse is None:
        return
    original = component_record(record, reuse)
    source_layout = geometry["layouts"][reuse["graphics"]]
    target_index, source_index = int(selection["part"].split("-")[-1]), int(reuse["part"].split("-")[-1])
    target, source = layout["parents"][target_index], source_layout["parents"][source_index]
    # Identical horizontal geometry fixes genuine camera/world orientation.
    # A taller native through sorting box permits the same physical housing;
    # neither a different axis nor a lower box can inherit the original art.
    if (target is None or source is None or target["extent"][:2] != source["extent"][:2]
            or target["extent"][2] < source["extent"][2] or target["offset"] != source["offset"]
            or is_housing(layout, target_index) != is_housing(source_layout, source_index)
            or selection["source"] != reuse["source"]):
        raise ValueError("terminal reuse changes native orientation or physical parent geometry")
    # Same-axis housing geometry alone is insufficient: its front/back visible
    # hardware must agree. Moving front louvers onto an outside rear is not a
    # genuine rotated view of the selected model.
    if is_housing(layout, target_index) and front_visible(layout["graphics"]) != front_visible(source_layout["graphics"]):
        raise ValueError("terminal housing reuse changes actual facade visibility")
    dx, dy, dz = [a - b for a, b in zip(target["origin"], source["origin"])]
    displacement = [2 * (dy - dx), dx + dy - dz]
    box = original["native_box"]
    expected = [box[0] + displacement[0], box[1] + displacement[1], box[2] + displacement[0], box[3] + displacement[1]]
    # Original rectangles, resampling and native source positions must stay
    # exact under translation. No stretch, crop, mirror or rotated raster is
    # hidden behind a convenient source-reuse label.
    if (reuse["screen_translation"] != displacement or component["native_box"] != expected
            or any(component[key] != original[key] for key in ("source_window", "measured_alpha16_bbox", "resampling", "alpha_crop_threshold"))):
        raise ValueError("terminal reuse changes whole original pixels or native translation")


def mounting_contacts(layout, index):
    """Derive actual physical strip endpoints without copying native movement or sorting state.

    @param layout Compiled native source geometry.
    @param index Real housing/rail parent index.
    @return Two native world X/Y contact points for this physical source component.
    """
    parent = layout["parents"][index]
    x, y, _ = parent["origin"]
    width, depth, _ = parent["extent"]
    if is_housing(layout, index):
        return [(x, y + depth - 1), (x + width - 1, y + depth - 1)] if width > depth else [(x + width - 1, y), (x + width - 1, y + depth - 1)]
    # Through sorting strips are3 units wide, but this model's open railing
    # uses their outer1-unit edge. Native boxes remain unchanged and both real
    # road lanes retain their original physical width and entry permissions.
    if layout["drive_through"]:
        if width > depth:
            y += depth - 1 if y else 0
            depth = 1
        else:
            x += width - 1 if x else 0
            width = 1
    return [(x, y), (x + width - 1, y)] if width > depth else [(x, y), (x, y + depth - 1)]


def compose(parts):
    """Build a whole source comparison while keeping every native parent independently stored.

    @param parts Three ordered indexed parents; through parent2 is genuinely absent.
    @return Indexed64x64 comparison with no claim of native vehicle depth sorting.
    """
    whole = parts[0].copy()
    whole.putdata([0] * (64 * 64))
    # A source contact page follows native sequence order only. The actual
    # world sorter retains independent boxes and later evaluates road-vehicle
    # occlusion; no flattening occurs in the source atlas or packed contract.
    for sprite in parts:
        if sprite is not None:
            whole.putdata([new if new else old for old, new in zip(whole.get_flattened_data(), sprite.get_flattened_data())])
    return whole


def validate_view(parts, layout, check_original_glass=True):
    """Require native mounting contacts, genuine facade visibility and actual bay/rail access.

    @param parts Indexed original native parents, including true through absence.
    @param layout Actual compiled directed bay or undirected through geometry.
    @param check_original_glass Require original amber inventory before approved state-glass substitution.
    @return Whole source-only comparison after all active view checks pass.
    """
    # Structural absence belongs only to through parent2. Missing original
    # bay rails or a housing cannot inherit that native exemption.
    if len(parts) != 3 or any((sprite is None) != (parent is None) for sprite, parent in zip(parts, layout["parents"])):
        raise ValueError("terminal source loses actual native parent coverage")
    # Each independent sorting owner must retain legal pixels and both real
    # mounting endpoints. A visually plausible whole cannot hide a detached
    # rail, missing housing or equipment moved onto the wrong visible facade.
    for index, sprite in enumerate(parts):
        if sprite is None:
            continue
        if sprite.getbbox() is None or sprite.size != (64, 64) or sprite.mode != "P":
            raise ValueError("terminal view is missing a nonempty actual native parent")
        if set(sprite.get_flattened_data()) & (terminal.base.FORBIDDEN - {0}):
            raise ValueError("terminal view contains forbidden native palette indices")
        for wx, wy in mounting_contacts(layout, index):
            px, py = 31 + 2 * (wy - wx), 33 + wx + wy
            if not any(sprite.getpixel((x, y)) for x in range(max(0, px - 2), min(64, px + 3))
                       for y in range(max(0, py - 2), min(64, py + 3))):
                raise ValueError(f"terminal gfx{layout['graphics']} parent{index} misses native contact({px},{py})")
        # Original selection verifies amber front glass. A separately approved
        # state conversion may replace that exact fitting with retained status
        # material, while all native contacts, alpha and access remain checked.
        if check_original_glass and is_housing(layout, index):
            palette = sprite.getpalette()
            amber = any(value and palette[3 * value] >= 140 and palette[3 * value + 1] >= 60 and palette[3 * value + 2] < 80
                        for value in sprite.get_flattened_data())
            if amber != front_visible(layout["graphics"]):
                raise ValueError("terminal original places visible front glass on the wrong facade")
    whole = compose(parts)
    if not layout["drive_through"]:
        entrances = {0: [(x, y) for x in range(4) for y in (5, 9)],
                     1: [(x, y) for x in (5, 9) for y in range(12, 16)],
                     2: [(x, y) for x in range(12, 16) for y in (5, 9)],
                     3: [(x, y) for x in (5, 9) for y in range(4)]}
        # Both traffic sides retain their actual native bay mouths. Original
        # source shadows/crosspieces must not paint a new entrance obstruction.
        for wx, wy in entrances[layout["graphics"]]:
            if whole.getpixel((31 + 2 * (wy - wx), 33 + wx + wy)):
                raise ValueError("terminal original obstructs an actual directed bay entrance")
    else:
        # The through safety rail must preserve actual road lane/vehicle-centre
        # heights. The separately sorted native housing retains its physical
        # occlusion; decoded/ordinary-game proof evaluates that owner at10.4D.
        rail = parts[1]
        for longitudinal in range(16):
            for across in (5, 9):
                wx, wy = (longitudinal, across) if layout["axis"] == "X" else (across, longitudinal)
                px, floor = 31 + 2 * (wy - wx), 33 + wx + wy
                for height in range(9):
                    if rail.getpixel((px, floor - height)):
                        raise ValueError("terminal through rail obscures actual native vehicle centres")
    return whole


def review_board(poses):
    """Compare all six native original views and their independent parent ownership.

    @param poses Ordered layout/part/whole source tuples.
    @return RGB1x/4x whole and native parent comparison, with visibility labels.
    """
    board = Image.new("RGB", (1560, 704), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((12, 12), "SOURCE ONLY: six genuine native terminal views; independently sorted parents; preview/loading states and runtime pending", fill="black")
    # Whole views reveal model continuity while separate parents expose native
    # contact, facade and genuine through-absence mistakes that flattening hides.
    for column, (layout, parts, whole) in enumerate(poses):
        x = 12 + column * 256
        draw.text((x, 44), f"gfx{layout['graphics']} {layout['direction']} {'front' if front_visible(layout['graphics']) else 'rear'} housing", fill="black")
        rgba = whole.convert("RGBA")
        board.paste(rgba, (x, 68), rgba)
        large = rgba.resize((256, 256), Image.Resampling.NEAREST)
        board.paste(large, (x, 144), large)
        for index, sprite in enumerate(parts):
            draw.text((x, 420 + index * 88), f"parent-{index}" + (" ABSENT" if sprite is None else ""), fill="black")
            if sprite is not None:
                rgba = sprite.convert("RGBA")
                board.paste(rgba, (x, 440 + index * 88), rgba)
    return board


def build(record):
    """Build every real native ready0 terminal view from complete selected originals.

    @param record Terminal original/registration document with explicit ready-parent matrix.
    @return Partial family report and deterministic source-artifact mapping.
    """
    geometry = terminal.load_geometry()
    family = record["families"]["facility.road-terminal"]
    selections = family["ready_parts"]
    expected = {(layout["graphics"], f"parent-{index}") for layout in geometry["layouts"]
                for index, parent in enumerate(layout["parents"]) if parent is not None}
    keys = [(row["graphics"], row["part"]) for row in selections]
    if len(keys) != len(expected) or set(keys) != expected:
        raise ValueError("terminal genuine view parent matrix is missing or ambiguous")
    registrations = {str(row["graphics"]) + "/" + row["part"]: component_record(record, row) for row in selections}
    if terminal.signature(registrations) != family["approved_ready_component_geometry_sha256"]:
        raise ValueError("terminal ready native component registration changed after selection")
    poses, rows, files = [], [], {}
    atlas = Image.new("P", (192, 384), 0)
    atlas.putpalette(terminal.base.PALETTE_BYTES)
    # Every compiled cell consumes genuine directed/axis source parents. Bounded
    # same-orientation reuse is checked against actual native boxes and exact
    # whole-pixel translation before it can supply any required source slot.
    for layout in geometry["layouts"]:
        graphics = layout["graphics"]
        parts = []
        for index, parent in enumerate(layout["parents"]):
            selected = next((row for row in selections if (row["graphics"], row["part"]) == (graphics, f"parent-{index}")), None)
            sprite = None
            if parent is not None:
                validate_reuse(record, selected, layout, geometry)
                sprite = terminal.native_component(record, selected, layout)
                atlas.paste(sprite, (index * 64, graphics * 64))
            parts.append(sprite)
            rows.append({"native_graphics": graphics, "direction": layout["direction"], "state": "ready", "frame": 0,
                         "part": f"parent-{index}", "box": [index * 64, graphics * 64, 64, 64], "anchor": [-31, -33],
                         "native_packed_anchor": packed_anchor(layout, index), "structurally_absent": parent is None,
                         "selection": selected, "nonempty_pixels": sum(value != 0 for value in sprite.get_flattened_data()) if sprite is not None else 0})
        whole = validate_view(parts, layout)
        poses.append((layout, parts, whole))
        files[OUTPUT / f"{PREFIX}-gfx{graphics}-whole.png"] = terminal.png_bytes(whole)
    board = io.BytesIO()
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md task25.2; archi-map-layers.md; six native ready source views only")
    review_board(poses).save(board, format="PNG", pnginfo=metadata, optimize=False, compress_level=9)
    files[OUTPUT / f"{PREFIX}-atlas.png"] = terminal.png_bytes(atlas)
    files[OUTPUT / f"{PREFIX}-review.png"] = board.getvalue()
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md task25.2; archi-map-layers.md",
              "asset_id": "facility.road-terminal", "native_graphics": list(range(6)), "state": "ready", "frame": 0,
              "source_rows": rows, "nonempty_source_parts": 16, "structurally_absent_parts": 2,
              "source_coverage_complete": False, "runtime_activated": False,
              "reuse_policy": "Only actual same-world-orientation horizontal geometry and facade visibility; whole original pixels translate by native RemapCoords delta, no flat rotation or repaint",
              "remaining": "Preview/loading phases and complete288-slot source handoff25.3; native packing/decoded/game proof10.4D"}
    report["artifacts"] = [{"path": path.relative_to(terminal.ROOT).as_posix(), "sha256": hashlib.sha256(payload).hexdigest()} for path, payload in files.items()]
    files[OUTPUT / f"{PREFIX}-manifest.json"] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def main():
    """Publish the complete six-view source comparison without claiming all state coverage.

    @return None; every original/native view validates before artifact publication.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--review", action="store_true", required=True)
    parser.parse_args()
    record = json.loads(terminal.RECORD.read_text(encoding="utf-8"))
    public = json.loads(terminal.PUBLIC.read_text(encoding="utf-8"))
    terminal.validate_terminal_contract(next(row for row in public["assets"] if row["asset_id"] == "facility.road-terminal"))
    report, files = build(record)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    # Validate every source before replacing any accepted comparison artifact;
    # incomplete or contaminated views never publish a partial six-view set.
    for path, payload in files.items():
        path.write_bytes(payload)
    print(json.dumps({"native_graphics": report["native_graphics"], "nonempty_source_parts": 16,
                      "structurally_absent_parts": 2, "source_coverage_complete": False, "runtime_activated": False}))


if __name__ == "__main__":
    main()
