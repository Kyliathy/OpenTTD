"""Convert measured original Space rectangles to fixed native canvases and review contacts.

@file convert_space_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Groups14BB-14BFC
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups63-64/67-69
Only retained original crops, uniform resampling, fixed registration and the existing DOS quantizer are used; creative edits remain imagegen.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_m1_art as base

ROOT = Path(__file__).resolve().parents[2]
RECORD = ROOT / "media/multilayer/source/space/space-generation-record.json"
ASTEROID_ASSETS = ("asteroid.nickel", "asteroid.ice", "asteroid.rare-earth")


def identity(path):
    """Read a concrete repository source or derived artifact identity.

    @param path Existing exact file; no recency selection.
    @return Repository-relative path and actual SHA256.
    """
    path = path.resolve()
    return dict(path=path.relative_to(ROOT).as_posix(), sha256=hashlib.sha256(path.read_bytes()).hexdigest())


def read_record():
    """Read the original Space provenance owner.

    @return Explicit family/candidate definitions; no gameplay state is copied or installed.
    """
    return json.loads(RECORD.read_text(encoding="utf-8"))


def registration_signature(candidate, direction):
    """Bind one approved original and its exact native transform.

    @param candidate Retained original with measured per-view registration.
    @param direction Genuine engine-viewport heading in that original.
    @return Stable SHA256 over original bytes, heading and native registration.
    """
    value = dict(source=candidate["sha256"], direction=direction, transform=candidate["native_transforms"][direction])
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode("utf-8")).hexdigest()


def source_separator(pixel, policy=None):
    """Observe measured separator colors without altering any original pixel.

    @param pixel Original red/green/blue/alpha tuple.
    @param policy Optional per-output measured thresholds for antialiased/darkened magenta frames; defaults retain the original bright-frame test.
    @return Whether this visible pixel belongs to the declared magenta separator color; never a permission to erase it from a crop.
    """
    red, green, blue, alpha = pixel
    policy = policy or {"red_min": 220, "green_max": 60, "blue_min": 220, "chroma_min": 0}
    # Transparent hidden RGB is not source artwork. Visible magenta/purple separators must remain outside selected rectangles even when the generator darkens their antialiased edges.
    return (alpha >= 16 and red >= policy["red_min"] and green <= policy["green_max"] and blue >= policy["blue_min"]
            and min(red, blue) - green >= policy["chroma_min"])


def source_candidate(key, document):
    """Resolve one exact original source and verify its immutable ancestry.

    @param key Explicit candidate key.
    @param document Original Space generation record.
    @return Original candidate with verified source/receipt/reference identities.
    @throws ValueError On absent, rejected, duplicate or changed ancestry.
    """
    matches = [row for row in document["candidates"] if row["key"] == key]
    if len(matches) != 1:
        raise ValueError("missing or duplicate Space candidate")
    candidate = matches[0]
    if candidate["status"].startswith("rejected"):
        raise ValueError("rejected Space source cannot supply a native view")
    # An original prompt receipt and every actual image input remain unchanged; a plausible replacement cannot silently supply a selected model.
    for record in [{k: candidate[k] for k in ("path", "sha256")}, candidate["generation_receipt"], *candidate["references"]]:
        if identity(ROOT / record["path"]) != record:
            raise ValueError("immutable Space source ancestry changed")
    return candidate


def native_view(key, direction, document=None):
    """Convert one genuine measured view without synthesizing another direction or state.

    @param key Exact retained original.
    @param direction Genuine original engine-viewport heading.
    @param document Optional detached provenance record used by focused refusal checks.
    @return Indexed declared native pixels and original measured registration evidence; craft remain64x48 and natural asteroid bodies use96x80.
    @throws ValueError On missing direction, clipping, separator intrusion or changed registration.
    """
    document = read_record() if document is None else document
    candidate = source_candidate(key, document)
    family = document["families"][candidate["asset_id"]]
    if direction not in candidate["native_transforms"]:
        raise ValueError("missing genuine Space heading")
    approved = family.get("approved_base_registration", {}).get(direction)
    # Selected body registration is immutable across later tool/work/load variants; source approval cannot drift with a modified conversion transform.
    if approved is not None and registration_signature(candidate, direction) != approved:
        raise ValueError("approved Space registration changed")
    dimensions, anchor = ([96, 80], [-48, -48]) if candidate["asset_id"] in ASTEROID_ASSETS else ([64, 48], [-32, -32])
    # Public canvas and world reference are one contract. Natural2tile bodies require wider padding under actual2:1 projection; widening a craft or shifting either family's anchor cannot conceal a registration error.
    if family["public_native_size"] != dimensions or family["public_anchor"] != anchor:
        raise ValueError("Space native canvas or anchor changed")
    native, evidence = source_pixels(candidate, direction, family["native_art_envelope"], dimensions)
    return base.quantize(native, {}), evidence


def source_pixels(candidate, direction, envelope, native_size=(64, 48)):
    """Register original pixels for a validated base or separately approved authored component.

    @param candidate Original already checked by source_candidate for exact ancestry; this helper grants no source approval.
    @param direction Genuine original engine-viewport heading.
    @param envelope Declared native visible bounds; component callers additionally constrain replacement to their approved physical part.
    @param native_size Explicit supported craft64x48 or natural-body96x80 canvas; default preserves all accepted craft APIs/native pixels.
    @return Registered RGBA native canvas and measured original evidence before canonical palette conversion.
    @throws ValueError On changed measured crop, separation, scale or native clipping.
    """
    if direction not in candidate["native_transforms"]:
        raise ValueError("missing genuine Space heading")
    # A multi-view original may retain useful genuine views while one camera angle fails model continuity. That explicitly excluded view cannot re-enter a base or component through its otherwise valid source rectangle.
    if direction in candidate.get("unaccepted_directions", {}):
        raise ValueError("unaccepted original Space view")
    if tuple(native_size) not in ((64, 48), (96, 80)):
        raise ValueError("unsupported Space native canvas")
    width, height = native_size
    transform = candidate["native_transforms"][direction]
    original = Image.open(ROOT / candidate["path"]).convert("RGBA")
    left, top, right, bottom = transform["crop_box"]
    # Ordinary source rectangles must remain inside their own original; out-of-bounds padding could hide a clipped hook or solar paddle.
    if not (0 <= left < right <= original.width and 0 <= top < bottom <= original.height):
        raise ValueError("Space source crop leaves its original")
    crop = original.crop((left, top, right, bottom))
    bounds = crop.getchannel("A").point([0] * 16 + [255] * 240).getbbox()
    # Measured immutable subject bounds protect all original hooks/paddles, including faint visible edges, from later registration drift.
    if bounds is None or list(bounds) != transform["measured_subject_bbox_in_crop"]:
        raise ValueError("Space subject differs from measured original")
    if min(bounds[0], bounds[1], crop.width - bounds[2], crop.height - bounds[3]) < 16:
        raise ValueError("Space source clips required transparent clearance")
    if any(source_separator(pixel, candidate.get("separator_color_policy")) for pixel in crop.get_flattened_data()):
        # Separator colors cannot become hull, cargo or status pixels; refuse instead of painting away source evidence.
        raise ValueError("opaque magenta separator enters Space crop")
    if "source_part_box" in transform:
        part_left, part_top, part_right, part_bottom = transform["source_part_box"]
        # A real authored effect may meet its original mounting plane. Validate the complete separated source view first, then take only its declared ordinary subpart rectangle; this never repairs source overlap or keys away a defective background.
        if not (left <= part_left < part_right <= right and top <= part_top < part_bottom <= bottom):
            raise ValueError("authored Space part leaves its validated source view")
        crop = original.crop((part_left, part_top, part_right, part_bottom))
        left, top, right, bottom = part_left, part_top, part_right, part_bottom
    numerator, denominator = transform["uniform_scale"]
    # A single declared physical scale across source views preserves their model proportions; malformed/nonpositive scales cannot justify clipping.
    if numerator <= 0 or denominator <= 0 or numerator >= denominator:
        raise ValueError("unsupported Space source scale")
    scale = numerator / denominator
    small = crop.resize((round(crop.width * scale), round(crop.height * scale)), Image.Resampling.BOX)
    ref_x, ref_y = transform["source_reference"]
    native_x, native_y = transform.get("native_reference", (32, 32))
    # Whole bodies keep their fixed world reference; isolated authored parts declare the actual original native mounting contact instead of moving the protected hull.
    if not (0 <= native_x < width and 0 <= native_y < height):
        raise ValueError("Space component reference leaves native canvas")
    offset = (native_x - round((ref_x - left) * scale), native_y - round((ref_y - top) * scale))
    visible = small.getchannel("A").point([0] * 16 + [255] * 240).getbbox()
    if visible is None:
        raise ValueError("Space resampling loses every model pixel")
    native_bounds = [visible[0] + offset[0], visible[1] + offset[1], visible[2] + offset[0], visible[3] + offset[1]]
    x0, y0, x1, y1 = envelope
    # Even a caller's overly broad envelope cannot authorize silent canvas clipping of an original body or physical component.
    if not (0 <= x0 < x1 <= width and 0 <= y0 < y1 <= height):
        raise ValueError("Space visible envelope leaves native canvas")
    # Every original visible part must fit the approved native envelope before canvas pasting; silent canvas clipping cannot pass source review.
    if native_bounds[0] < x0 or native_bounds[1] < y0 or native_bounds[2] > x1 or native_bounds[3] > y1:
        raise ValueError(f"Space model leaves its approved native envelope: {direction} {native_bounds}")
    native = Image.new("RGBA", (width, height))
    native.paste(small, offset)
    return native, dict(direction=direction, original=identity(ROOT / candidate["path"]), transform=transform, native_subject_bounds=native_bounds)


def review(key, output, family=False):
    """Write one original's exact native review without claiming full source or runtime coverage.

    @param key Explicit candidate, or a family whose eight free headings were selected.
    @param output Fresh repository-local proof directory.
    @param family Whether to compare all eight selected family headings across their original sources.
    @return Hash-bound partial native source review report.
    """
    document = read_record()
    if family:
        definitions = document["families"][key]
        directions = definitions["directions"]
        # A combined base contact requires every actual selected heading; a duplicate/absent view cannot be filled from a neighboring direction.
        if len(directions) != 8 or any("free/" + direction not in definitions["selections"] for direction in directions):
            raise ValueError("Space base review requires eight selected genuine headings")
        selected = [(definitions["selections"]["free/" + direction], direction) for direction in directions]
    else:
        candidate = source_candidate(key, document)
        selected = [(key, direction) for direction in candidate["native_transforms"]]
    output = output.resolve()
    # Source conversion proof belongs to the existing native-art proof home; it cannot overwrite shipped payloads or a prior approved review.
    if not output.is_relative_to(ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("Space review requires a fresh native-art proof directory")
    output.mkdir(parents=True)
    definitions = document["families"][source_candidate(selected[0][0], document)["asset_id"]]
    width, height = definitions["public_native_size"]
    column_width = width * 4
    large_top = height + 28
    atlas = Image.new("P", (width * len(selected), height), 0)
    atlas.putpalette(base.PALETTE_BYTES)
    contact = Image.new("RGB", (column_width * len(selected), large_top + height * 4 + 4), (38, 43, 49))
    drawing = ImageDraw.Draw(contact)
    evidence = []
    # Each displayed heading comes from its own original source window; native and nearest-neighbor enlarged views show the same converted pixels.
    for index, (original_key, direction) in enumerate(selected):
        pixels, registration = native_view(original_key, direction, document)
        atlas.paste(pixels, (width * index, 0))
        rgba = pixels.convert("RGBA")
        contact.paste(rgba, (column_width * index + (column_width - width) // 2, 20), rgba)
        large = rgba.resize((column_width, height * 4), Image.Resampling.NEAREST)
        contact.paste(large, (column_width * index, large_top), large)
        drawing.text((column_width * index + 8, 4), direction, fill=(235, 235, 235))
        evidence.append(registration)
    atlas.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    report = output / "source-review.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, candidate=key,
                                     views=evidence, complete_source_coverage=False, packed=False, runtime=False,
                                     artifacts=[identity(output / name) for name in ("native.png", "contact.png")]), indent=2) + "\n", encoding="utf-8")
    return identity(report)


def main():
    """Convert one explicit candidate and report its partial source review identity."""
    parser = argparse.ArgumentParser(description=__doc__)
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument("--candidate")
    selection.add_argument("--family")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(review(args.candidate or args.family, args.output, bool(args.family))))


if __name__ == "__main__":
    main()
