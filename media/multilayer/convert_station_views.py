"""Register independently authored conventional station parents on actual native cells.

@file convert_station_views.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md task24.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The current stage covers ready0 graphics0..3 only. Native back/front owners fix
their placement; source processing copies/resamples original pixels and never
opens rock, changes rail geometry or counts missing canopy/state art as complete.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw, PngImagePlugin

import convert_station_art as loader
from convert_terrain_art import png_bytes
from tools.ground_station_contract import load_geometry, native_packed_anchor, validate_station_contract


OUTPUT = loader.OUTPUT
PREFIX = "station-base-views"
PARTS = ("back", "front", "roof", "glass")


def native_component_rgba(record, selection, layout):
    """Register one genuine immutable component against its declared native owner.

    @param record Station source/provenance document, read only.
    @param selection Explicit source/component and native part for this cell.
    @param layout Actual compiled native graphics/axis/role geometry.
    @return RGBA64x64 original component with native anchor(-31,-33), before palette selection.
    @throws ValueError Before publication on missing, rejected or altered inputs.
    """
    found = [row for row in record["candidates"] if row["id"] == selection["source"]]
    if len(found) != 1:
        raise ValueError("station component original is missing or ambiguous")
    source = found[0]
    if source["selection"]["status"] not in ("pending_native_component_review", "approved_native_components"):
        raise ValueError("rejected station original cannot supply a component")
    if loader.base.digest(loader.base.repository_path(source["path"])) != source["sha256"]:
        raise ValueError("immutable station component original changed")
    # Geometry/design references remain immutable. A rejected whole pose may
    # be a declared edit reference, but cannot silently supply delivered pixels.
    for reference in source["references"]:
        if loader.base.digest(loader.base.repository_path(reference["path"])) != reference["sha256"]:
            raise ValueError("immutable station component reference changed")
    if selection["component"] in source.get("excluded_components", []):
        raise ValueError("excluded station component cannot provide source coverage")
    components = [row for row in source.get("components", []) if row["id"] == selection["component"]]
    if len(components) != 1:
        raise ValueError("station component transform is missing or ambiguous")
    transform = components[0]
    # A native parent is an independently drawn source owner. Its genuine axis
    # and applicable roles must match; relabeling an X original cannot supply Y.
    if (transform["axis"] != layout["axis"] or layout["role"] not in transform["native_roles"]
            or transform["part"] != selection["part"] or transform["native_size"] != [64, 64]
            or transform["anchor"] != [-31, -33] or transform["resampling"] != "BOX"
            or transform["alpha_crop_threshold"] != 16):
        raise ValueError("station component disagrees with its actual native parent")
    with Image.open(loader.base.repository_path(source["path"])) as original:
        image = original.convert("RGBA")
    if list(image.size) != source["image_size"]:
        raise ValueError("station component original dimensions changed")
    window, box = transform["source_window"], transform["native_box"]
    # Complete ordinary rectangles exclude frames without erasing pixels or
    # rescuing overlapping views. All registered originals must fit the native
    # canvas fully; cropping a foot/roof at the destination is never permitted.
    if (len(window) != 4 or not 0 <= window[0] < window[2] <= image.width
            or not 0 <= window[1] < window[3] <= image.height or len(box) != 4
            or not 0 <= box[0] < box[2] <= 64 or not 0 <= box[1] < box[3] <= 64):
        raise ValueError("station component crop or native placement leaves its canvas")
    crop = image.crop(window)
    bounds = crop.getchannel("A").point(lambda value: 255 if value >= 16 else 0).getbbox()
    if bounds is None:
        raise ValueError("empty station component original")
    if list(bounds) != transform["measured_alpha16_bbox"]:
        raise ValueError("station component measured source bounds changed")
    left, top, right, bottom = bounds
    # Transparent RGB may retain magenta from the source frame. Only measured
    # alpha determines visible clearance; opaque separator pixels inside a
    # selected rectangle still refuse before quantization or publication.
    if min(left, top, crop.width - right, crop.height - bottom) < 16:
        raise ValueError("station component has insufficient transparent source clearance")
    if any(alpha >= 16 and red > 220 and green < 60 and blue > 220
           for red, green, blue, alpha in crop.get_flattened_data()):
        raise ValueError("station component crop contains opaque separator magenta")
    native = Image.new("RGBA", (64, 64))
    native.alpha_composite(crop.crop(bounds).resize((box[2] - box[0], box[3] - box[1]), Image.Resampling.BOX), box[:2])
    return native


def native_component(record, selection, layout):
    """Quantize one checked original component without changing its native registration.

    @param record Immutable station source/provenance document.
    @param selection Explicit component and native parent selection.
    @param layout Actual compiled graphics/axis/role geometry.
    @return Indexed64x64 source with the established safe palette and common anchor.
    """
    return loader.base.quantize(native_component_rgba(record, selection, layout), {})


def validate_ready_pose(parts, layout):
    """Check actual platform ends, native rail centre and separate source-parent ownership.

    @param parts Indexed back/front original source components for one native cell.
    @param layout Its compiled native geometry, independent of source captions.
    @return Indexed whole preview retaining every selected component pixel.
    @throws ValueError On source overlap, lost contacts, wrong palette or rail obstruction.
    """
    # Only non-canopy roles are delivered at this checkpoint. Missing roof and
    # glass originals must never inherit completeness from these low parents.
    if layout["parts"][2] is not None or layout["roof_glass"] is not None:
        raise ValueError("base view cannot supply a missing canopy roof or glass child")
    back, front = parts["back"], parts["front"]
    pixels = []
    # These physical components are separated by the native rail gap. Duplicate
    # opaque coverage would count a whole scene twice and obscure real trains.
    for rear, near in zip(back.get_flattened_data(), front.get_flattened_data()):
        if rear != 0 and near != 0:
            raise ValueError("station base source parents overlap")
        pixels.append(rear or near)
    whole = back.copy()
    whole.putdata(pixels)
    if set(pixels) & (loader.base.FORBIDDEN - {0}):
        raise ValueError("station base view contains forbidden palette colors")
    # Native bare cells must remain low decks. A loader cannot substitute for
    # a missing bare view, and a flat platform cannot stand in for the actual
    # loading-building silhouette merely because both share ledge contacts.
    if layout["role"] == 0 and whole.getbbox()[1] < 30:
        raise ValueError("native bare platform source contains a building")
    if layout["role"] == 1 and back.getbbox()[1] >= 30:
        raise ValueError("native loading-building source is missing its housing")
    axis = layout["axis"]
    contacts = ((41, 38), (53, 44), (9, 54), (21, 60)) if axis == "X" else ((9, 44), (21, 38), (41, 60), (53, 54))
    # Every genuine axis must reach both actual ledges at both through mouths.
    # Two pixels include low deck top/side shading, not an absent platform end.
    for px, py in contacts:
        if not any(whole.getpixel((x, y)) != 0 for x in range(px - 2, px + 3) for y in range(py - 2, min(64, py + 3))):
            raise ValueError("station base view misses an actual native ledge contact")
    # Native X/Y projected depth is derived from the fixed2:1 tile camera.
    # Whole-source shadows, frames and crosspieces must leave the rail centre
    # empty. The front parent also leaves actual checked train heights clear.
    for index, value in enumerate(pixels):
        x, y = index % 64, index // 64
        depth = 2 * y + x if axis == "X" else 2 * y - x
        if value != 0 and (124 <= depth <= 132 if axis == "X" else 62 <= depth <= 70):
            raise ValueError("station base view paints across the native rail centre")
    for longitudinal in range(16):
        x, y = (47 - 2 * longitudinal, 41 + longitudinal) if axis == "X" else (15 + 2 * longitudinal, 41 + longitudinal)
        for height in range(9):
            if front.getpixel((x, y - height)) != 0:
                raise ValueError("station front parent covers the native train centre")
    return whole


def review_board(poses):
    """Compare the four delivered native base cells and their independently authored axes.

    @param poses Ordered native geometry, selected parents and whole previews.
    @return Source-only RGB contact page at native1x/4x, without runtime claims.
    """
    board = Image.new("RGB", (1280, 416), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((16, 12), "SOURCE ONLY: native graphics0..3 ready0, genuine X/Y bare and loader views; canopy/states/runtime pending", fill="black")
    # Parallel native views expose hardware/height/anchor drift directly; this
    # source page does not replace ordinary-game rendering or user art review.
    for index, (layout, parts, whole) in enumerate(poses):
        x = 16 + index * 320
        draw.text((x, 44), f"gfx{layout['graphics']} {layout['axis']} {'bare' if layout['role'] == 0 else 'loader'}", fill="black")
        rgba = whole.convert("RGBA")
        board.paste(rgba, (x, 64), rgba)
        large = rgba.resize((256, 256), Image.Resampling.NEAREST)
        board.paste(large, (x + 32, 144), large)
    return board


def build(record):
    """Build the four actual bare/loading-building base views in memory.

    @param record Station generation record with explicit ready0 parent selections.
    @return Partial source report and deterministic output-path-to-bytes mapping.
    """
    geometry = load_geometry()
    selections = record["families"]["facility.rail-station"]["ready_base_parts"]
    expected = {(graphics, part) for graphics in range(4) for part in ("back", "front")}
    keys = [(row["graphics"], row["part"]) for row in selections]
    # All low native cells are necessary for this partial view checkpoint;
    # duplicates or an extra canopy row cannot hide a missing genuine parent.
    if len(keys) != len(expected) or set(keys) != expected:
        raise ValueError("station base view parent matrix is missing or ambiguous")
    original = loader.original_candidate(record, "station-loader-x-base-v4")
    retained = loader.split_base(loader.native_base(original))
    atlas = retained["back"].copy().resize((256, 256), Image.Resampling.NEAREST)
    atlas.putdata([0] * (256 * 256))
    poses, rows, files = [], [], {}
    # Each compiled graphics ID selects a real axis/role and its actual parents;
    # reversed travel, missing views or another station family create no slots.
    for graphics in range(4):
        layout = geometry["layouts"][graphics]
        parts = {}
        for part in ("back", "front"):
            selected = next(row for row in selections if row["graphics"] == graphics and row["part"] == part)
            if "retained_base" in selected:
                # Reuse exact approved X pixels, rather than regenerating a
                # delivered component or relabeling it as the other axis.
                if layout["axis"] != "X" or selected["retained_base"] != original["id"]:
                    raise ValueError("retained X original cannot supply another native axis")
                parts[part] = retained[part].copy()
            else:
                parts[part] = native_component(record, selected, layout)
        whole = validate_ready_pose(parts, layout)
        poses.append((layout, parts, whole))
        for part_index, part in enumerate(PARTS):
            sprite = parts.get(part)
            # Bare/loading-building roofs and glass are genuinely absent.
            # Empty source slots encode that structural fact, never missing art.
            if sprite is not None:
                atlas.paste(sprite, (part_index * 64, graphics * 64))
            rows.append({"native_graphics": graphics, "axis": layout["axis"], "role": layout["role"],
                         "state": "ready", "frame": 0, "part": part, "structurally_absent": sprite is None,
                         "source_selection": next((row for row in selections if row["graphics"] == graphics and row["part"] == part), None),
                         "box": [part_index * 64, graphics * 64, (part_index + 1) * 64, (graphics + 1) * 64],
                         "anchor": [-31, -33], "native_packed_anchor": native_packed_anchor(layout, part)})
        files[OUTPUT / f"{PREFIX}-gfx{graphics}-whole.png"] = png_bytes(whole)
    review = io.BytesIO()
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md task24.2; archi-map-layers.md; partial source view review")
    review_board(poses).save(review, format="PNG", pnginfo=metadata, optimize=False, compress_level=9)
    files[OUTPUT / f"{PREFIX}-atlas.png"] = png_bytes(atlas)
    files[OUTPUT / f"{PREFIX}-review.png"] = review.getvalue()
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md task24.2; archi-map-layers.md; four native source base views only",
              "asset_id": "facility.rail-station", "native_graphics": list(range(4)), "state": "ready", "frame": 0,
              "source_rows": rows, "nonempty_source_parts": 8, "structurally_absent_parts": 8,
              "source_coverage_complete": False, "runtime_activated": False,
              "remaining": "Canopy graphics4..7 structural roof/glass/support originals, then all declared preview/loading phases and512-slot source closure"}
    report["artifacts"] = [{"path": path.relative_to(loader.base.ROOT).as_posix(), "sha256": hashlib.sha256(payload).hexdigest()} for path, payload in files.items()]
    files[OUTPUT / f"{PREFIX}-manifest.json"] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def main():
    """Review the complete declared partial source matrix after in-memory validation.

    @return None; all refusal gates run before any generated artifact is written.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--review", action="store_true", required=True)
    parser.parse_args()
    record = json.loads(loader.RECORD.read_text(encoding="utf-8"))
    public = json.loads(loader.PUBLIC.read_text(encoding="utf-8"))
    validate_station_contract(next(row for row in public["assets"] if row["asset_id"] == "facility.rail-station"))
    report, files = build(record)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    # Publication starts only after every declared source parent/pose passes;
    # failed geometry cannot overwrite the last accepted partial review.
    for path, payload in files.items():
        path.write_bytes(payload)
    print(json.dumps({"native_graphics": report["native_graphics"], "nonempty_source_parts": report["nonempty_source_parts"],
                      "review": f"{PREFIX}-review.png", "source_coverage_complete": False, "runtime_activated": False}))


if __name__ == "__main__":
    main()
