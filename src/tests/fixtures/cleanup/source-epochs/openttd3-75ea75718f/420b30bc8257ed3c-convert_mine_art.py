"""Register genuine mine machinery against actual fixed native cells and sorting.

@file convert_mine_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md tasks27.1/27.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Completed base geometry is separate from native construction/operating/depleted
source phases and later packing. Existing M1 material reuse preserves ancestry.
"""

import argparse
import hashlib
import io
import json
import statistics

from PIL import Image, ImageDraw, PngImagePlugin

import convert_facility_art as facilities
import convert_m1_art as base
from convert_portal_art import native_housing
from convert_terrain_art import png_bytes
from sprite_quantize import front_mask
from tools.ground_mine_contract import ROOT, mine_contract, validate_mine_contract


RECORD = ROOT / "media/multilayer/source/ground/mine-generation-record.json"
PUBLIC = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
OUTPUT = ROOT / "logs/proofs/mlm/native-art/ground"
PREFIX = "coal-mine-base"
PARTS = ("back", "front")


def signature(value):
    """Bind fixed native/original geometry independently of JSON dictionary order.

    @param value Original component, fitting or native registration mapping.
    @return Canonical sorted-JSON SHA256 identity.
    """
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def component_record(record, selection):
    """Resolve exactly one selected immutable original and native cell registration.

    @param record Mine original/provenance document.
    @param selection Explicit native variant/source/component identity.
    @return Candidate source and exact registered original component.
    """
    sources = [row for row in record["candidates"] if row["id"] == selection["source"]]
    if len(sources) != 1:
        raise ValueError("mine original is missing or ambiguous")
    components = [row for row in sources[0].get("components", []) if row["id"] == selection["component"]]
    if len(components) != 1:
        raise ValueError("mine original component is missing or ambiguous")
    return sources[0], components[0]


def original_cell(record, selection, contract):
    """BOX sample one whole retained cell without clipping or replacing original pixels.

    @param record Mine source/candidate/provenance document.
    @param selection Actual native row-major cell and original component selection.
    @param contract Compiled fixed mine geometry and source placement contract.
    @return Indexed64x64 original apparatus/floor at its canonical native anchor.
    @throws ValueError Before publication on rejected, changed or unsafe original geometry.
    """
    source, component = component_record(record, selection)
    # A partially usable sheet supplies only explicitly selected independent
    # cells. Its tall chute/filter cannot inherit approval from safe neighbors.
    if source["selection"]["status"] not in ("pending_native_component_review", "approved_native_components") or selection["variant"] in source.get("excluded_variants", []):
        raise ValueError("rejected or excluded mine original cannot supply pixels")
    facilities.verify_original(source)
    variant = selection["variant"]
    # Native world coordinates, fixed view and common anchor own the source;
    # changing captions or reusing a whole different cell cannot fill a gap.
    if (component["variant"] != variant or component["world_cell"] != contract["native_cells"][variant]
            or component["direction"] != "none" or component["native_size"] != [64, 64]
            or component["anchor"] != [-31, -33] or component["resampling"] != "BOX"
            or component["alpha_crop_threshold"] != 16 or component["whole_original_uniform_scale"] is not True):
        raise ValueError("mine original disagrees with its actual native cell/view")
    with Image.open(base.repository_path(source["path"])) as original:
        image = original.convert("RGBA")
    if list(image.size) != source["image_size"]:
        raise ValueError("mine original dimensions changed")
    window = component["source_window"]
    # Ordinary complete rectangles exclude frames. Overlapping source masks,
    # separator erasure and destination clipping cannot rescue a missing cell.
    if (len(window) != 4 or not 0 <= window[0] < window[2] <= image.width
            or not 0 <= window[1] < window[3] <= image.height):
        raise ValueError("mine original crop leaves its canvas")
    crop = image.crop(window)
    bounds = crop.getchannel("A").point(lambda alpha: 255 if alpha >= 16 else 0).getbbox()
    if bounds is None or list(bounds) != component["measured_alpha16_bbox"]:
        raise ValueError("mine measured original bounds changed or became empty")
    left, top, right, bottom = bounds
    if min(left, top, crop.width - right, crop.height - bottom) < 16:
        raise ValueError("mine original has insufficient transparent clearance")
    if any(alpha >= 16 and red > 220 and green < 60 and blue > 220 for red, green, blue, alpha in crop.get_flattened_data()):
        raise ValueError("mine original contains opaque separator magenta")
    height = round((bottom - top) * 62 / (right - left))
    box = component["native_box"]
    # A centered62px whole original matches the actual0..15 contact diamond.
    # Integer height rounding is the only aspect change; an anisotropic squeeze
    # or shifted/clipped floor must not masquerade as native registration.
    if height > 64 or box != [1, 64 - height, 63, 64]:
        raise ValueError("mine original loses whole uniform native registration")
    canvas = Image.new("RGBA", (64, 64))
    canvas.alpha_composite(crop.crop(bounds).resize((62, height), Image.Resampling.BOX), box[:2])
    return base.quantize(canvas, {})


def control_cell(record):
    """Reproduce the compatible original M1 hut and require its exact shipped native pixels.

    @param record Mine base record with explicit approved one-cell control-hut ancestry.
    @return Indexed native M1 ready0 hut, unchanged before later mine status authoring.
    @throws ValueError On foreign geometry, changed originals or false native reuse evidence.
    """
    family = record["families"]["mine.COAL"]
    reuse = family["control_reuse"]
    # This is one genuine compatible physical component, never an entire
    # foreign industry or a convenient rotated image of missing mine equipment.
    if (signature(reuse) != family["approved_control_reuse_sha256"]
            or reuse["source_record"] != facilities.RECORD.relative_to(ROOT).as_posix()
            or reuse["source_family"] != "facility.earth-receiver" or reuse["source_variant"] != 5
            or reuse["source_state"] != "ready" or reuse["source_frame"] != 0
            or reuse["target_variant"] != 7 or reuse["target_world_cell"] != [1, 2]
            or reuse["source_native_canvas"] != [64, 64] or reuse["source_anchor"] != [-31, -33]
            or reuse["phase_component"] != "facility-status"):
        raise ValueError("mine control reuse changes genuine component geometry or ancestry")
    donor = json.loads(facilities.RECORD.read_text(encoding="utf-8"))
    source = next(row for row in donor["families"] if row["asset_id"] == reuse["source_family"])
    if source["path"] != reuse["source_path"] or source["sha256"] != reuse["source_sha256"]:
        raise ValueError("mine control original ancestry changed")
    facilities.verify_original(source)
    cells, _boxes = facilities.select_cells(source)
    component = next(row for row in donor["components"] if row["key"] == reuse["phase_component"])
    fixed = native_housing(cells[1][5])
    fixed = facilities.insert_indicator(fixed, facilities.indicator(component)[1][0], source["cue_targets"][5])
    native = base.quantize(fixed, {})
    atlas_path = base.repository_path(reuse["source_native_atlas"])
    if base.digest(atlas_path) != reuse["source_native_sha256"]:
        raise ValueError("mine control native reuse atlas changed")
    with Image.open(atlas_path) as atlas:
        expected = atlas.crop((640, 256, 704, 320))
    if native.tobytes() != expected.tobytes():
        raise ValueError("mine control original does not reproduce exact native pixels")
    return native


def validate_cell(sprite, variant):
    """Require actual floor contacts, legal native material and the unchanged sorting envelope.

    @param sprite Indexed64x64 whole original cell, including its intact one-tile floor.
    @param variant Actual native variant used for actionable refusal details.
    @return None after original physical placement checks.
    """
    # A missing component, palette trick or resized blank cannot count as
    # one of the actual native nine machinery cells in the completed base.
    if sprite.mode != "P" or sprite.size != (64, 64) or sprite.getbbox() is None:
        raise ValueError("mine source loses a nonempty actual native cell")
    if set(sprite.get_flattened_data()) & (base.FORBIDDEN - {0}):
        raise ValueError("mine source contains forbidden native palette indices")
    # All four paid-floor corners must meet native0..15 projection. Separately
    # check the true south tip: a skewed floor can intersect a corner tolerance
    # with its sloping edge while its actual contact is several pixels away.
    for px, py in ((31, 33), (1, 48), (31, 63), (61, 48)):
        if not any(sprite.getpixel((x, y)) for x in range(max(0, px - 2), min(64, px + 3))
                   for y in range(max(0, py - 2), min(64, py + 3))):
            raise ValueError(f"mine cell{variant} misses actual floor contact({px},{py})")
    left, top, right, bottom = sprite.getbbox()
    south = statistics.mean(x for x in range(left, right) if sprite.getpixel((x, bottom - 1)))
    if abs(south - 31) > 2:
        raise ValueError(f"mine cell{variant} has a skewed actual south floor contact")
    # The full projected16x16x20 native volume bounds source height/footprint.
    # Two pixels cover raster edge rounding, never cropping or changing sorting
    # to accommodate a tall source tower or machinery outside its paid cell.
    for y in range(64):
        for x in range(64):
            if sprite.getpixel((x, y)) and (y < 13 + abs(x - 31) / 2 - 2 or y > 64 - abs(x - 31) / 2 + 2):
                raise ValueError(f"mine cell{variant} leaves its actual20-high sorting envelope")


def source_parts(whole):
    """Retain the existing native industry underlay and independently sorted original upper part.

    @param whole Indexed original one-cell apparatus/floor with proven native placement.
    @return Full original back and exact original upper-front subset, matching M1 industry convention.
    """
    keep = front_mask(1, 1, 64, 64, (31, 33), full_span=True)
    front = whole.copy()
    front.putdata([value if (index % 64, index // 64) in keep else 0 for index, value in enumerate(whole.get_flattened_data())])
    return whole.copy(), front


def campus(parts):
    """Compose only the nine genuine machinery cells for a source-scale comparison.

    @param parts Native back/front pair by actual row-major variant0..8.
    @return Indexed256x160 source campus; separate loading floor remains unpainted.
    """
    image = Image.new("P", (256, 160), 0)
    image.putpalette(base.PALETTE_BYTES)
    image.info["transparency"] = 0
    # Ground underlays precede independent sorted upper parents, matching the
    # existing selected-industry convention. This comparison is not a native
    # world sorter or proof of ordinary-game vehicle/building occlusion.
    for part in range(2):
        for variant in sorted(parts, key=lambda value: (value % 3 + value // 3, value)):
            x, y = variant % 3, variant // 3
            sprite = parts[variant][part]
            # Transparency follows index0, not grayscale brightness. Dark coal
            # and metal remain opaque even when their palette luminance is low.
            mask = Image.new("L", (64, 64))
            mask.putdata([255 if value else 0 for value in sprite.get_flattened_data()])
            image.paste(sprite, (96 + 32 * (y - x), 8 + 16 * (x + y)), mask)
    return image


def review_board(parts, whole):
    """Show the true assembled footprint and every independently owned native cell/part.

    @param parts Nine actual native original back/front source pairs.
    @param whole Source-only campus comparison at fixed world registration.
    @return RGB native/3x campus and native/2x per-cell/part review board.
    """
    board = Image.new("RGB", (1584, 768), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((12, 8), "SOURCE BASE ONLY: native fixed3x3 Coal machinery; four separate loading cells untouched; construction/working/paused/depleted phases and runtime pending", fill="black")
    rgba = whole.convert("RGBA")
    board.paste(rgba, (16, 52), rgba)
    large = rgba.resize((768, 480), Image.Resampling.NEAREST)
    board.paste(large, (16, 236), large)
    # Individual cells expose missing roles, unsafe floor registration and
    # source-part ownership that a plausible flattened campus could conceal.
    for variant, pair in sorted(parts.items()):
        x, y = 800 + (variant % 3) * 256, 52 + (variant // 3) * 232
        draw.text((x, y), f"cell{variant} world({variant % 3},{variant // 3})" + (" M1 hut" if variant == 7 else ""), fill="black")
        rgba = pair[0].convert("RGBA")
        board.paste(rgba, (x, y + 20), rgba)
        large = rgba.resize((128, 128), Image.Resampling.NEAREST)
        board.paste(large, (x + 72, y + 20), large)
        for index, sprite in enumerate(pair):
            draw.text((x + index * 96, y + 156), PARTS[index], fill="black")
            rgba = sprite.convert("RGBA")
            board.paste(rgba, (x + index * 96, y + 172), rgba)
    return board


def build(record, public):
    """Build the complete eighteen-part fixed native Coal base from genuine original cells.

    @param record Immutable selected mine originals and approved control-hut reuse ancestry.
    @param public Shared native mine.COAL source contract.
    @return Partial-family base report and deterministic review artifact mapping.
    """
    validate_mine_contract(public)
    contract = mine_contract("COAL")
    family = record["families"]["mine.COAL"]
    selections = family["base_cells"]
    variants = [row["variant"] for row in selections]
    if len(variants) != 8 or set(variants) != set(range(9)) - {7}:
        raise ValueError("mine base native cell matrix is missing or ambiguous")
    registrations = {str(row["variant"]): component_record(record, row)[1] for row in selections}
    if signature(registrations) != family["approved_base_component_geometry_sha256"]:
        raise ValueError("mine native original registration changed after selection")
    parts, rows = {}, []
    atlas = Image.new("P", (1152, 64), 0)
    atlas.putpalette(base.PALETTE_BYTES)
    # Every real cell retains a distinct genuine original role. The compatible
    # control hut alone reuses one approved M1 component; no copied whole pose
    # or invented rotation fills an absent native machinery slot.
    for variant in range(9):
        selected = next((row for row in selections if row["variant"] == variant), None)
        whole = control_cell(record) if variant == 7 else original_cell(record, selected, contract)
        validate_cell(whole, variant)
        parts[variant] = source_parts(whole)
        for index, (part, sprite) in enumerate(zip(PARTS, parts[variant])):
            if sprite.getbbox() is None:
                raise ValueError("mine base lacks a real native source part")
            x = (variant * 2 + index) * 64
            atlas.paste(sprite, (x, 0))
            rows.append({"asset_id": "mine.COAL", "direction": "none", "native_layout": 0, "tile_variant": variant,
                         "world_cell": contract["native_cells"][variant], "part": part, "box": [x, 0, 64, 64],
                         "anchor": [-31, -33], "base_model_reference_only": True, "selection": selected,
                         "nonempty_pixels": sum(value != 0 for value in sprite.get_flattened_data()),
                         "source_pixels_sha256": hashlib.sha256(sprite.tobytes()).hexdigest()})
    whole = campus(parts)
    files = {OUTPUT / f"{PREFIX}-atlas.png": png_bytes(atlas), OUTPUT / f"{PREFIX}-campus.png": png_bytes(whole)}
    board = io.BytesIO()
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md tasks27.1/27.2; archi-map-layers.md; completed base model only")
    review_board(parts, whole).save(board, format="PNG", pnginfo=metadata, optimize=False, compress_level=9)
    files[OUTPUT / f"{PREFIX}-review.png"] = board.getvalue()
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md tasks27.1/27.2; archi-map-layers.md",
              "asset_id": "mine.COAL", "native_layout": 0, "directions": ["none"], "native_size": [64, 64], "anchor": [-31, -33],
              "footprint": [3, 3], "tile_variants": 9, "source_parts": rows, "base_nonempty_parts": 18,
              "source_coverage_complete": False, "runtime_activated": False, "control_reuse": family["control_reuse"],
              "part_policy": "Exact existing M1 industry convention: full original underlay plus independently sorted upper original subset; source comparison reconstructs original pixels, no repaint",
              "remaining": "Actual construction stages/working/paused/depleted source closure27.3 and native packing/decoded/game proof10.4D"}
    report["artifacts"] = [{"path": path.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(payload).hexdigest()} for path, payload in files.items()]
    files[OUTPUT / f"{PREFIX}-manifest.json"] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def main():
    """Publish only a fully validated native base comparison without claiming full state coverage.

    @return None; every source/native placement refusal precedes artifact writes.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--review", action="store_true", required=True)
    parser.parse_args()
    record = json.loads(RECORD.read_text(encoding="utf-8"))
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    report, files = build(record, next(row for row in public["assets"] if row["asset_id"] == "mine.COAL"))
    OUTPUT.mkdir(parents=True, exist_ok=True)
    # A missing or unsafe native cell cannot overwrite the accepted complete
    # base set; all originals, contacts and reused pixels validate first.
    for path, payload in files.items():
        path.write_bytes(payload)
    print(json.dumps({"native_cells": 9, "base_nonempty_parts": report["base_nonempty_parts"],
                      "source_coverage_complete": False, "runtime_activated": False}))


if __name__ == "__main__":
    main()
