"""Register original mine-terminal parents against actual native truck-stop geometry.

@file convert_terminal_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md task25.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The first stage selects only the genuine SE base. Original pixels, independent
native parents and real road/ground owners stay separate from packing/runtime.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw, PngImagePlugin

import convert_m1_art as base
from convert_terrain_art import png_bytes
from tools.ground_terminal_contract import ROOT, PARTS, load_geometry, packed_anchor, validate_terminal_contract


RECORD = ROOT / "media/multilayer/source/ground/terminal-generation-record.json"
PUBLIC = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
OUTPUT = ROOT / "logs/proofs/mlm/native-art/ground"
PREFIX = "terminal-se-base"


def signature(value):
    """Identify fixed original-component registration independently of JSON key order.

    @param value Approved crop/native geometry mapping.
    @return Canonical sorted-JSON SHA256 identity.
    """
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def native_component(record, selection, layout):
    """Copy and BOX-sample one complete genuine original into its native parent canvas.

    @param record Immutable terminal source/candidate/provenance document.
    @param selection Explicit source/component/native-parent selection.
    @param layout Actual compiled native graphics and entrance/axis geometry.
    @return Indexed64x64 original parent source with common anchor(-31,-33).
    @throws ValueError Before publication on changed/rejected sources or unsafe registration.
    """
    candidates = [row for row in record["candidates"] if row["id"] == selection["source"]]
    if len(candidates) != 1:
        raise ValueError("terminal original is missing or ambiguous")
    source = candidates[0]
    # A failed whole sheet may supply only explicitly reviewed independent
    # components; its rejected loader never inherits approval from safe rails.
    if source["selection"]["status"] not in ("pending_native_component_review", "approved_native_components") or selection["component"] in source.get("excluded_components", []):
        raise ValueError("rejected or excluded terminal original cannot supply pixels")
    if base.digest(base.repository_path(source["path"])) != source["sha256"]:
        raise ValueError("immutable terminal original changed")
    for reference in source["references"]:
        if base.digest(base.repository_path(reference["path"])) != reference["sha256"]:
            raise ValueError("immutable terminal reference changed")
    components = [row for row in source.get("components", []) if row["id"] == selection["component"]]
    if len(components) != 1:
        raise ValueError("terminal component registration is missing or ambiguous")
    transform = components[0]
    # Native view and sequence ownership control the source, not a source
    # caption. Relabelling a rail or another entrance cannot fill a missing view.
    if (layout["graphics"] not in transform["native_graphics"] or transform["direction"] != layout["direction"]
            or transform["part"] != selection["part"] or transform["native_size"] != [64, 64]
            or transform["anchor"] != [-31, -33] or transform["resampling"] != "BOX"
            or transform["alpha_crop_threshold"] != 16):
        raise ValueError("terminal original disagrees with its native parent/view")
    with Image.open(base.repository_path(source["path"])) as original:
        image = original.convert("RGBA")
    if list(image.size) != source["image_size"]:
        raise ValueError("terminal original dimensions changed")
    window, box = transform["source_window"], transform["native_box"]
    # Ordinary complete rectangles exclude source frames. Destination clipping,
    # painted corrections or guessed overlapping-view masks cannot rescue art.
    if (len(window) != 4 or not 0 <= window[0] < window[2] <= image.width
            or not 0 <= window[1] < window[3] <= image.height or len(box) != 4
            or not 0 <= box[0] < box[2] <= 64 or not 0 <= box[1] < box[3] <= 64):
        raise ValueError("terminal original crop/placement leaves its canvas")
    crop = image.crop(window)
    bounds = crop.getchannel("A").point(lambda alpha: 255 if alpha >= 16 else 0).getbbox()
    if bounds is None or list(bounds) != transform["measured_alpha16_bbox"]:
        raise ValueError("terminal measured source bounds changed or became empty")
    left, top, right, bottom = bounds
    if min(left, top, crop.width - right, crop.height - bottom) < 16:
        raise ValueError("terminal original has insufficient transparent clearance")
    # Transparent RGB haze is not model paint. Measured visible separator
    # pixels inside an ordinary source rectangle still refuse before sampling.
    if any(alpha >= 16 and red > 220 and green < 60 and blue > 220
           for red, green, blue, alpha in crop.get_flattened_data()):
        raise ValueError("terminal original contains opaque separator magenta")
    native = Image.new("RGBA", (64, 64))
    native.alpha_composite(crop.crop(bounds).resize((box[2] - box[0], box[3] - box[1]), Image.Resampling.BOX), box[:2])
    return base.quantize(native, {})


def validate_base(parts, layout):
    """Verify the three genuine SE parents, native strip contacts and open truck entrance.

    @param parts Indexed original parents in exact compiled native sequence order.
    @param layout Actual SE bay geometry, not a through-layout or four-tile draft.
    @return Whole indexed source-only model comparison.
    @throws ValueError On missing physical contacts, lost equipment or blocked entrance.
    """
    if layout["graphics"] != 1 or layout["drive_through"] or len(parts) != 3:
        raise ValueError("terminal SE base cannot supply another native layout")
    whole = parts[0].copy()
    whole.putdata([0] * (64 * 64))
    # Each parent owns its original mounting strip, including the two different
    # safety-rail lengths. Native sequence order is kept for source comparison;
    # actual vehicle sorting remains the later decoded/ordinary-game proof.
    for index, sprite in enumerate(parts):
        if sprite.getbbox() is None or sprite.size != (64, 64) or sprite.mode != "P":
            raise ValueError("terminal base is missing a nonempty native parent")
        if set(sprite.get_flattened_data()) & (base.FORBIDDEN - {0}):
            raise ValueError("terminal source contains forbidden native palette indices")
        parent = layout["parents"][index]
        x, y, z = parent["origin"]
        width, depth, height = parent["extent"]
        contacts = [(x, y), (x, y + depth - 1)] if index != 1 else [(x, y + depth - 1), (x + width - 1, y + depth - 1)]
        for wx, wy in contacts:
            px, py = 31 + 2 * (wy - wx), 33 + wx + wy - z
            if not any(sprite.getpixel((sx, sy)) for sx in range(max(0, px - 2), min(64, px + 3))
                       for sy in range(max(0, py - 2), min(64, py + 3))):
                raise ValueError(f"terminal parent{index} misses a real native mounting contact ({px},{py})")
        # All source pixels retain their actual whole original; no per-pixel
        # foreground erasure is performed to invent space for a truck.
        whole.putdata([new if new else old for old, new in zip(whole.get_flattened_data(), sprite.get_flattened_data())])
    if parts[1].getbbox()[1] >= 30:
        raise ValueError("terminal base is missing its actual loading housing")
    # Both native SE bay traffic sides enter at x5/x9 and y12..15. Their mouth
    # points must stay fully empty in the source silhouette; the roadway and
    # true U-turn motion remain the actual road controller's responsibility.
    for wx in (5, 9):
        for wy in range(12, 16):
            px, py = 31 + 2 * (wy - wx), 33 + wx + wy
            if whole.getpixel((px, py)):
                raise ValueError("terminal source obstructs a real native truck entrance")
    return whole


def review_board(parts, whole):
    """Show the complete one-tile base and each independent native source parent.

    @param parts Three indexed genuine SE parent originals.
    @param whole Whole source model at fixed native registration.
    @return RGB1x/4x whole and parent comparison without runtime claims.
    """
    board = Image.new("RGB", (1056, 512), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((16, 12), "SOURCE ONLY: SE bay gfx1, three original native parents; other views/states and runtime pending", fill="black")
    # One column per exact native owner exposes source contact/height drift,
    # rather than hiding misregistered parts inside a single large preview.
    for index, (name, image) in enumerate(zip(("whole",) + PARTS, [whole] + parts)):
        x = 16 + index * 256
        draw.text((x, 44), name, fill="black")
        rgba = image.convert("RGBA")
        board.paste(rgba, (x, 72), rgba)
        large = rgba.resize((256, 256), Image.Resampling.NEAREST)
        board.paste(large, (x, 160), large)
    return board


def build(record):
    """Build one selected native SE base after complete original/contact validation.

    @param record Terminal source record with three explicit independently registered parents.
    @return Partial source report and deterministic output-path-to-bytes mapping.
    """
    family = record["families"]["facility.road-terminal"]
    selections = family["base_parts"]
    if len(selections) != 3 or {row["part"] for row in selections} != set(PARTS):
        raise ValueError("terminal base parent matrix is missing or ambiguous")
    layout = load_geometry()["layouts"][1]
    parts, registrations = [], {}
    # Preserve each complete original parent. Approved native registrations
    # are immutable even when a one-pixel drift might still touch a tolerance.
    for part in PARTS:
        selected = next(row for row in selections if row["part"] == part)
        source = next(row for row in record["candidates"] if row["id"] == selected["source"])
        component = next(row for row in source["components"] if row["id"] == selected["component"])
        registrations[part] = component
        parts.append(native_component(record, selected, layout))
    if signature(registrations) != family["approved_base_component_geometry_sha256"]:
        raise ValueError("terminal native source registration changed after selection")
    whole = validate_base(parts, layout)
    atlas = whole.copy().resize((192, 64), Image.Resampling.NEAREST)
    atlas.putdata([0] * (192 * 64))
    rows = []
    for index, (part, image) in enumerate(zip(PARTS, parts)):
        atlas.paste(image, (index * 64, 0))
        rows.append({"native_graphics": 1, "direction": "SE", "state": "ready", "frame": 0, "part": part,
                     "box": [index * 64, 0, 64, 64], "anchor": [-31, -33],
                     "native_packed_anchor": packed_anchor(layout, index), "selection": next(row for row in selections if row["part"] == part),
                     "nonempty_pixels": sum(value != 0 for value in image.get_flattened_data())})
    files = {OUTPUT / f"{PREFIX}-atlas.png": png_bytes(atlas), OUTPUT / f"{PREFIX}-whole.png": png_bytes(whole)}
    board = io.BytesIO()
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md task25.1; archi-map-layers.md; one native SE base only")
    review_board(parts, whole).save(board, format="PNG", pnginfo=metadata, optimize=False, compress_level=9)
    files[OUTPUT / f"{PREFIX}-review.png"] = board.getvalue()
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md task25.1; archi-map-layers.md",
              "asset_id": "facility.road-terminal", "native_graphics": 1, "direction": "SE", "state": "ready", "frame": 0,
              "source_parts": rows, "source_coverage_complete": False, "runtime_activated": False,
              "remaining": "Other5 genuine native layouts, all declared preview/loading phases and full288-slot source closure; packing/game proof10.4D"}
    report["artifacts"] = [{"path": path.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(payload).hexdigest()} for path, payload in files.items()]
    files[OUTPUT / f"{PREFIX}-manifest.json"] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def main():
    """Publish one fully validated base review without falsely completing the family.

    @return None; every source/contact refusal occurs before any artifact write.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--review", action="store_true", required=True)
    parser.parse_args()
    record = json.loads(RECORD.read_text(encoding="utf-8"))
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    validate_terminal_contract(next(row for row in public["assets"] if row["asset_id"] == "facility.road-terminal"))
    report, files = build(record)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    # A rejected view cannot overwrite the current accepted partial source;
    # the complete selected base must validate before publication starts.
    for path, payload in files.items():
        path.write_bytes(payload)
    print(json.dumps({"native_graphics": 1, "nonempty_source_parts": 3,
                      "source_coverage_complete": report["source_coverage_complete"], "runtime_activated": False}))


if __name__ == "__main__":
    main()
