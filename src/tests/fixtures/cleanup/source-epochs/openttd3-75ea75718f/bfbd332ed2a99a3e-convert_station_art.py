"""Review original conventional station cells at their verified native placement.

@file convert_station_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group24.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This initial base checkpoint selects one X loading-building silhouette and its
two actual parents. It does not claim the other axis, modular roles, preview
states, family coverage, packing or ordinary-game rendering. Native sorting
boxes retain their actual owner; cropping/resampling never creates a station.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw, PngImagePlugin

import convert_m1_art as base
from convert_terrain_art import png_bytes
from tools.ground_station_contract import load_geometry, validate_station_contract


RECORD = base.ROOT / "media/multilayer/source/ground/station-generation-record.json"
PUBLIC = base.ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
OUTPUT = base.ROOT / "logs/proofs/mlm/native-art/ground"
PREFIX = "station-loader-x-base"


def original_candidate(record, identity):
    """Resolve one immutable eligible base candidate and its exact references.

    @param record Existing shared station source provenance, without mutation.
    @param identity Explicit source candidate ID; no latest-file discovery.
    @return One eligible original source record.
    @throws ValueError On rejected/ambiguous/changed originals or references.
    """
    found = [row for row in record["candidates"] if row["id"] == identity]
    if len(found) != 1:
        raise ValueError("station base candidate is missing or ambiguous")
    source = found[0]
    if source["selection"]["status"] not in ("pending_native_scale_review", "approved_base_silhouette"):
        raise ValueError("rejected station original cannot provide a native pose")
    if base.digest(base.repository_path(source["path"])) != source["sha256"]:
        raise ValueError("immutable station original changed")
    # Reference roles distinguish a geometry target from a rejected design.
    # Every retained input stays exact even after another candidate is selected.
    for reference in source["references"]:
        if base.digest(base.repository_path(reference["path"])) != reference["sha256"]:
            raise ValueError("immutable station reference changed")
    return source


def native_base(source):
    """Register the entire genuine X loader original without replacing source pixels.

    @param source Eligible immutable candidate and measured native transform.
    @return Indexed64x64 original base pose at native anchor(-31,-33).
    @throws ValueError On unsafe extraction, altered registration or lost geometry.
    """
    load_geometry()
    transform = source.get("native_transform", {})
    # This base checkpoint must not accidentally bless a canopy, a copied Y
    # view or a completed source matrix. Other genuine views have later owners.
    if (transform.get("axis") != "X" or transform.get("role") != 1 or transform.get("state") != "ready"
            or transform.get("frame") != 0 or transform.get("native_size") != [64, 64]
            or transform.get("anchor") != [-31, -33] or transform.get("resampling") != "BOX"
            or transform.get("alpha_crop_threshold") != 16):
        raise ValueError("unsupported station base native transform")
    with Image.open(base.repository_path(source["path"])) as original:
        image = original.convert("RGBA")
    if list(image.size) != source["image_size"]:
        raise ValueError("station original dimensions changed")
    bounds = image.getchannel("A").point(lambda value: 255 if value >= 16 else 0).getbbox()
    if bounds is None:
        raise ValueError("empty station source original")
    if list(bounds) != transform["measured_alpha16_bbox"]:
        raise ValueError("station original measured registration changed")
    left, top, right, bottom = bounds
    # An isolated cutout needs no separator rescue or guessed source masks.
    # All visible model pixels fit with generous clearance on all four sides.
    if min(left, top, image.width - right, image.height - bottom) < 16:
        raise ValueError("station original has insufficient transparent clearance")
    if any(alpha >= 16 and red > 220 and green < 60 and blue > 220
           for red, green, blue, alpha in image.get_flattened_data()):
        raise ValueError("station original contains opaque separator magenta")
    height = round((bottom - top) * 64 / (right - left))
    # Uniform registration must retain the whole original inside the native
    # canvas; an oversized silhouette cannot be repaired by cropping its roof.
    if height < 1 or height > 64:
        raise ValueError("station original cannot fit the native base canvas")
    if transform.get("native_box") != [0, 64 - height, 64, 64]:
        raise ValueError("station base must preserve uniform source scale and floor contact")
    native = Image.new("RGBA", (64, 64))
    native.alpha_composite(image.crop(bounds).resize((64, height), Image.Resampling.BOX), (0, 64 - height))
    indexed = base.quantize(native, {})
    validate_base(indexed)
    return indexed


def split_base(whole):
    """Assign the two separated native platform components to their real parents.

    @param whole Indexed source pose with an already validated empty rail corridor.
    @return Exact disjoint back/front originals whose union is the unchanged pose.
    """
    parts = {}
    # Native X projection gives2*screen_y+screen_x =97+4*world_y-2*z.
    # The front platform starts at world_y11, beyond the empty track corridor.
    # Threshold132 lies inside that physical gap: it assigns actual separated
    # components, creates no edge pixels and cannot turn a tall rear wall into
    # foreground over a train. Native bounding boxes still own actual sorting.
    for name in ("back", "front"):
        part = whole.copy()
        part.putdata([value if (2 * (offset // 64) + offset % 64 > 132) == (name == "front") else 0
                      for offset, value in enumerate(whole.get_flattened_data())])
        if part.getbbox() is None:
            raise ValueError("station original is missing an actual platform parent")
        parts[name] = part
    return parts


def validate_base(whole):
    """Check native contacts, palette and an actually empty track centre independently.

    @param whole Indexed native X loading-building pose to inspect.
    @return None after the base geometry checks pass.
    @throws ValueError On missing ends, forbidden colors or corridor obstruction.
    """
    # Only a native-sized indexed source can be paired with these actual
    # station parents; an RGB/enlarged review cannot count as playable geometry.
    if whole.size != (64, 64) or whole.mode != "P":
        raise ValueError("station base has the wrong native canvas or palette mode")
    if set(whole.get_flattened_data()) & (base.FORBIDDEN - {0}):
        raise ValueError("station base contains a forbidden native palette index")
    # Native platform inner-edge endpoints come from the actual5/6/5 strip
    # geometry. Two-pixel tolerance includes the low deck's top/side shading,
    # without accepting an absent ledge or a decorative multi-tile building.
    for px, py in ((41, 38), (53, 44), (9, 54), (21, 60)):
        if not any(whole.getpixel((x, y)) != 0 for x in range(px - 2, px + 3) for y in range(py - 2, min(64, py + 3))):
            raise ValueError("station base misses a native through-platform ledge contact")
    # This low native channel includes the actual rail centre and the sorting
    # split. Rear elevated art may sit behind a train, but these original
    # separated components must not contain a bridge, floor or source shadow.
    for offset, value in enumerate(whole.get_flattened_data()):
        if value != 0 and 124 <= 2 * (offset // 64) + offset % 64 <= 132:
            raise ValueError("station base paints across the actual rail corridor")


def review_board(whole, parts):
    """Show native parts and joined cells as a source placement review.

    @param whole Registered native original model.
    @param parts Its disjoint back/front parent images.
    @return RGB source review; this is not an ordinary-game screenshot.
    """
    board = Image.new("RGB", (1024, 736), (228, 233, 237))
    draw = ImageDraw.Draw(board)
    draw.text((16, 12), "SOURCE ONLY: X loading-building base, ready phase0; native1x/4x and three adjacent cells; runtime pending", fill="black")
    # Each actual parent is visible alone and together, so a low foreground
    # strip cannot hide an accidentally copied roof or missing rear loader.
    for index, (name, sprite) in enumerate((("whole", whole), *parts.items())):
        x = 24 + index * 336
        draw.text((x, 44), name, fill="black")
        rgba = sprite.convert("RGBA")
        board.paste(rgba, (x, 64), rgba)
        large = rgba.resize((256, 256), Image.Resampling.NEAREST)
        board.paste(large, (x + 64, 64), large)
    layout = Image.new("RGBA", (192, 96))
    # Three real X-neighbor positions expose the fixed native32/16 pitch.
    # A neutral code-native floor diamond is context only and is never packed
    # as artwork; actual game drawing remains the later integration checkpoint.
    floor = ImageDraw.Draw(layout)
    for index in range(3):
        x, y = 32 + index * 32, 32 - index * 16
        floor.polygon([(x + 31, y + 33), (x + 63, y + 48), (x + 31, y + 63), (x, y + 48)],
                      fill=(183, 192, 198, 255), outline=(95, 110, 120, 255))
    for index in range(3):
        layout.alpha_composite(whole.convert("RGBA"), (32 + index * 32, 32 - index * 16))
    large = layout.resize((768, 384), Image.Resampling.NEAREST)
    board.paste(large, (128, 352), large)
    return board


def build(record, identity):
    """Build the explicit base review in memory without publishing family coverage.

    @param record Immutable station source/provenance document to consume.
    @param identity Explicit eligible base candidate ID.
    @return Base report and deterministic output-path-to-bytes mapping.
    """
    source = original_candidate(record, identity)
    whole = native_base(source)
    parts = split_base(whole)
    review = io.BytesIO()
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text("Owner", "r2mlvs14-remaining-assets-and-runtime-integration.md task24.1; archi-map-layers.md; source preview only")
    review_board(whole, parts).save(review, format="PNG", pnginfo=metadata, optimize=False, compress_level=9)
    files = {OUTPUT / f"{PREFIX}-whole.png": png_bytes(whole),
             OUTPUT / f"{PREFIX}-review.png": review.getvalue()}
    rows = []
    # There are only two nonempty parents in this actual native role. A roof
    # stays structurally absent rather than being faked by opaque extra art.
    for name, image in parts.items():
        path = OUTPUT / f"{PREFIX}-{name}.png"
        payload = png_bytes(image)
        files[path] = payload
        rows.append({"part": name, "path": path.relative_to(base.ROOT).as_posix(), "sha256": hashlib.sha256(payload).hexdigest(),
                     "native_bbox": list(image.getbbox()), "opaque_pixels": sum(value != 0 for value in image.get_flattened_data())})
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md task24.1; archi-map-layers.md; source-only base review, not complete family/runtime delivery",
              "asset_id": "facility.rail-station", "candidate": identity, "source_sha256": source["sha256"],
              "axis": "X", "role": 1, "graphics": 2, "state": "ready", "frame": 0,
              "anchor": [-31, -33], "native_size": [64, 64], "parts": rows, "roof": None, "glass": None,
              "whole_path": (OUTPUT / f"{PREFIX}-whole.png").relative_to(base.ROOT).as_posix(),
              "whole_sha256": hashlib.sha256(files[OUTPUT / f"{PREFIX}-whole.png"]).hexdigest(),
              "review_path": (OUTPUT / f"{PREFIX}-review.png").relative_to(base.ROOT).as_posix(),
              "source_coverage_complete": False, "runtime_activated": False,
              "remaining": "Genuine Y view, bare/canopy roles, independent elevated canopy roof/glass sources, preview/loading phases and full512-slot source checks"}
    files[OUTPUT / f"{PREFIX}-manifest.json"] = (json.dumps(report, indent=2) + "\n").encode("utf-8")
    return report, files


def main():
    """Rebuild one explicit source base review, preserving all shipped owners.

    @return None; source refusal occurs before any review output write.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--review", required=True)
    args = parser.parse_args()
    record = json.loads(RECORD.read_text(encoding="utf-8"))
    public = json.loads(PUBLIC.read_text(encoding="utf-8"))
    validate_station_contract(next(row for row in public["assets"] if row["asset_id"] == "facility.rail-station"))
    report, files = build(record, args.review)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    # Validate the complete in-memory source first; failed candidates must not
    # replace an earlier accepted review with plausible partial output files.
    for path, payload in files.items():
        path.write_bytes(payload)
    print(json.dumps({"candidate": report["candidate"], "parts": report["parts"], "review": report["review_path"],
                      "source_coverage_complete": False, "runtime_activated": False}, indent=2))


if __name__ == "__main__":
    main()
