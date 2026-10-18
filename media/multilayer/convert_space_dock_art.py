"""Register original fixed Dock bodies and two clamps against native site geometry.

@file convert_space_dock_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13F
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group71
The activated native Dock owns two half-tile-separated berths in one approach
cell. Source assembly grants no slot, service, inventory, work or runtime sprite.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
from convert_space_module_art import registration_signature
from tools.orbital_art_contract import validate_orbital_contract

ASSET = "module.dock"
PUBLIC = art.ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
CANVAS = (192, 160)
FLOOR_ORIGIN = (96, 48)
FLOOR = ((96, 48), (160, 80), (96, 112), (32, 80))
CELLS = tuple((x, y) for y in range(2) for x in range(2))
ORIGINS = tuple((65 + 32 * (y - x), 15 + 16 * (x + y)) for x, y in CELLS)
APPROACHES = ((160, 96), (144, 104))
CONTACTS = ((144, 88), (128, 96))
CLAMP_BOUNDS = (-8, -6, 0, 0)


def indexed_canvas(size=CANVAS):
    """Allocate canonical transparent native storage without generating artwork.

    @param size Exact whole-model or per-tile source canvas dimensions.
    @return Transparent DOS-indexed canvas with the established palette.
    """
    result = Image.new("P", size, 0)
    result.putpalette(art.base.PALETTE_BYTES)
    result.info["transparency"] = 0
    return result


def public_contract(document=None):
    """Require the delivered fixed Dock contract before reading source pixels.

    @param document Optional detached public manifest for focused refusal checks.
    @return The unique actual2x2 Dock row with unchanged native anchors.
    @throws ValueError On missing, foreign or changed public geometry/state ownership.
    """
    document = json.loads(PUBLIC.read_text(encoding="utf-8")) if document is None else document
    matches = [row for row in document["assets"] if row["asset_id"] == ASSET]
    if len(matches) != 1:
        raise ValueError("missing or duplicate Dock public family")
    row = matches[0]
    validate_orbital_contract(row)
    # All four world cells belong to one fixed native Dock. Source approval
    # cannot turn it into another module or shift its future per-cell anchor.
    if (row["consumer"] != 11 or row["kind"] != "building" or row["native_size"] != [64, 64]
            or row["anchor"] != [-31, -33] or row["parts"] != ["back", "front"]
            or row["geometry"] != "per_tile" or row["bounds"] != [0, 0, 64, 64]
            or row["states"] != ["blueprint", "partial", "complete", "blocked"] or row["frames"] != 2):
        raise ValueError("Dock public cell/state contract changed")
    return row


def family_contract(document, public=None):
    """Bind source-local geometry to the original native Dock and its actual berths.

    @param document Exact or detached Space source catalogue.
    @param public Optional detached public manifest.
    @return Dock family with actual world cells, contacts and activation admission.
    @throws ValueError On changed footprint, orientation, approach or slot geometry.
    """
    public_contract(public)
    family = document["families"][ASSET]
    expected = dict(footprint=[2, 2], public_native_size=[64, 64], public_anchor=[-31, -33],
                    native_canvas=list(CANVAS), directions=["none"], native_approach_slots=2,
                    native_inert_approach_slots=1, native_world_cells=[list(point) for point in CELLS],
                    native_approaches=[list(point) for point in APPROACHES],
                    native_clamp_contacts=[list(point) for point in CONTACTS])
    # The second activated slot shares the original south cell. An authoring
    # dictionary cannot add another cell, rotation, approach or free facility.
    if any(family.get(key) != value for key, value in expected.items()):
        raise ValueError("Dock source differs from actual fixed native geometry or berth ownership")
    return family


def eligible_tiles(x, y):
    """Observe which actual native tile canvases can hold an original source pixel.

    @param x Whole-model native source column.
    @param y Whole-model native source row.
    @return Real cell indexes only; callers reject pixels with no native owner.
    """
    return [i for i, (ox, oy) in enumerate(ORIGINS) if 0 <= x - ox < 64 and 0 <= y - oy < 64]


def original_image(candidate):
    """Read one complete measured original using only a normal rectangular crop.

    @param candidate Ancestry-verified original and its approved transform.
    @return Whole original RGBA crop, preserving its generated transparency.
    @throws ValueError On dimension changes, clipping, measurement or separator intrusion.
    """
    with Image.open(art.ROOT / candidate["path"]) as source:
        source = source.convert("RGBA")
    transform = candidate["native_transform"]
    # Separate originals need no source masks or padding beyond their image.
    # Keeping the complete rectangle protects every visible corner and fitting.
    if list(source.size) != candidate["image_size"] or transform["crop_box"] != [0, 0, *source.size]:
        raise ValueError("Dock original dimensions or whole-source crop changed")
    bounds = source.getchannel("A").point([0] * 16 + [255] * 240).getbbox()
    if bounds is None or list(bounds) != transform["measured_subject_bbox_in_crop"]:
        raise ValueError("Dock original subject measurement changed")
    if min(bounds[0], bounds[1], source.width - bounds[2], source.height - bounds[3]) < 16:
        raise ValueError("Dock original lacks transparent clearance")
    if any(art.source_separator(pixel) for pixel in source.get_flattened_data()):
        raise ValueError("Dock source separator enters whole original")
    return source


def reduced_original(candidate, native_reference):
    """Uniformly reduce and place an approved original without hiding overflow.

    @param candidate Verified original and measured uniform registration.
    @param native_reference Actual native floor or mounting reference point.
    @return Indexed native canvas and original placement evidence.
    @throws ValueError On invalid reduction, canvas overflow or pixels outside real tile canvases.
    """
    original = original_image(candidate)
    transform = candidate["native_transform"]
    numerator, denominator = transform["uniform_scale"]
    if not (0 < numerator < denominator):
        raise ValueError("Dock source needs a positive uniform reduction")
    scale = numerator / denominator
    small = original.resize((round(original.width * scale), round(original.height * scale)), Image.Resampling.BOX)
    rx, ry = transform["source_reference"]
    offset = (native_reference[0] - round(rx * scale), native_reference[1] - round(ry * scale))
    bounds = small.getchannel("A").point([0] * 16 + [255] * 240).getbbox()
    if bounds is None:
        raise ValueError("Dock source vanishes under native reduction")
    placed = [bounds[i] + offset[i % 2] for i in range(4)]
    # Check the original before compositing; a small destination cannot conceal
    # an oversized deck, cabinet or clamp by silently cropping its pixels.
    if not (0 <= placed[0] < placed[2] <= CANVAS[0] and 0 <= placed[1] < placed[3] <= CANVAS[1]):
        raise ValueError("Dock original leaves native canvas before assembly")
    native = Image.new("RGBA", CANVAS)
    native.alpha_composite(small, offset)
    image = art.base.quantize(native, {})
    coverage = [0] * len(CELLS)
    # Native sorting cells are a real spatial bound, not four copies of the
    # whole machine. Every opaque original pixel must fit at least one cell.
    for y in range(CANVAS[1]):
        for x in range(CANVAS[0]):
            if image.getpixel((x, y)) == 0:
                continue
            cells = eligible_tiles(x, y)
            if not cells:
                raise ValueError(f"Dock original pixel leaves actual tile canvases: {x},{y}")
            for cell in cells:
                coverage[cell] += 1
    return image, dict(candidate=candidate["key"], original=art.identity(art.ROOT / candidate["path"]),
                       signature=registration_signature(candidate), transform=transform,
                       native_subject_bounds=placed, covered_pixels_per_cell=coverage,
                       native_sha256=hashlib.sha256(image.tobytes()).hexdigest())


def native_body(state="complete", document=None, public=None):
    """Resolve a genuine body for one declared Dock construction stage.

    @param state Exact physical stage; only explicitly selected original states are allowed.
    @param document Optional detached catalogue for focused refusal checks.
    @param public Optional actual public geometry contract.
    @return Indexed original body with measured floor/source evidence.
    @throws ValueError On foreign originals, changed approvals or invalid native floor geometry.
    """
    document = art.read_record() if document is None else document
    family = family_contract(document, public)
    if state not in family["body_sources"]:
        raise ValueError("missing genuine Dock body state")
    candidate = art.source_candidate(family["body_sources"][state], document)
    # A valid original receipt alone does not authorize a Core, rotated Dock or
    # different physical stage to replace this body's separately approved form.
    if (candidate["asset_id"] != ASSET or candidate["directions"] != ["none"]
            or candidate.get("state") != state or candidate.get("component") != f"fixed_dock_{state}_body"
            or registration_signature(candidate) != family["approved_body_registration"][state]):
        raise ValueError("Dock original body or stage registration changed")
    transform = candidate["native_transform"]
    # A genuine body must use the existing floor origin and projection. Its
    # image dimensions or a new approval cannot move native world geometry.
    if (transform["canvas"] != list(CANVAS) or transform["native_reference"] != list(FLOOR_ORIGIN)
            or transform["native_floor_corners"] != [list(point) for point in FLOOR]):
        raise ValueError("Dock whole-model native floor contract changed")
    numerator, denominator = transform["uniform_scale"]
    if not (0 < numerator < denominator):
        raise ValueError("Dock source needs a positive uniform reduction")
    scale = numerator / denominator
    rx, ry = transform["source_reference"]
    # Measured floor vertices, not copied expected labels, must land on the
    # actual2x2 diamond under one uniform reduction and a fixed translation.
    for original, expected in zip(transform["source_floor_corners"], FLOOR, strict=True):
        actual = (FLOOR_ORIGIN[0] + round((original[0] - rx) * scale),
                  FLOOR_ORIGIN[1] + round((original[1] - ry) * scale))
        if max(abs(actual[i] - expected[i]) for i in range(2)) > 2:
            raise ValueError("Dock original floor does not match native2x2 projection")
    image, evidence = reduced_original(candidate, FLOOR_ORIGIN)
    if not all(evidence["covered_pixels_per_cell"]):
        raise ValueError("Dock body omits an actual world cell")
    return image, evidence


def clamp_source(slot, document):
    """Place one genuine clamp instance at its existing native floor-edge reference.

    @param slot Original activated berth slot0or1, never a new approach cell.
    @param document Catalogue retaining the one original clamp and exact instance geometry.
    @return Indexed clamp-only canvas and physical source/slot evidence.
    @throws ValueError On extra slots, changed original registration or mounting overflow.
    """
    family = family_contract(document)
    if slot not in (0, 1):
        raise ValueError("Dock art cannot add a native berth")
    candidate = art.source_candidate(family["clamp_source"], document)
    # Two physical instances share one genuine original. Reusing source pixels
    # does not activate either native berth or admit a foreign hardware family.
    if (candidate["asset_id"] != ASSET or candidate["directions"] != ["none"]
            or candidate.get("component") != "fixed_dock_clamp"
            or registration_signature(candidate) != family["approved_clamp_registration"]):
        raise ValueError("Dock clamp original or registration changed")
    transform = candidate["native_transform"]
    if transform["native_relative_bounds"] != list(CLAMP_BOUNDS):
        raise ValueError("Dock clamp mounting envelope changed")
    image, evidence = reduced_original(candidate, CONTACTS[slot])
    x0, y0, x1, y1 = evidence["native_subject_bounds"]
    cx, cy = CONTACTS[slot]
    # The clamp sits inboard on the deck edge. Its whole source must fit before
    # assembly, including the foreground cheek; a tile crop cannot repair it.
    if not (cx - 8 <= x0 < x1 <= cx and cy - 6 <= y0 < y1 <= cy):
        raise ValueError("Dock clamp original leaves approved inboard mounting envelope")
    return image, dict(evidence, slot=slot, floor_contact=list(CONTACTS[slot]),
                       native_approach=list(APPROACHES[slot]))


def native_base(document=None, public=None):
    """Assemble the complete Dock from its original body and two accountable clamp instances.

    @param document Optional detached source catalogue for refusal checks.
    @param public Optional detached public geometry contract.
    @return Whole indexed Dock and original pixel/attachment preservation evidence.
    @throws ValueError On unanchored clamps, duplicated hardware or obstructed approaches.
    """
    document = art.read_record() if document is None else document
    body, body_evidence = native_body("complete", document, public)
    result = body.copy()
    clamps, occupied = [], set()
    # Both fixed instances attach to the same paid Dock. Their exact native
    # displacement comes from the original half-tile slot offset, not a mirror,
    # another service cell or two independently invented animation clocks.
    for slot in range(2):
        component, evidence = clamp_source(slot, document)
        pixels, overlap = [], 0
        for y in range(CANVAS[1]):
            for x in range(CANVAS[0]):
                value = component.getpixel((x, y))
                if value == 0:
                    continue
                if (x, y) in occupied:
                    raise ValueError("Dock clamp instances duplicate physical pixels")
                occupied.add((x, y))
                pixels.append([x, y])
                overlap += body.getpixel((x, y)) != 0
                result.putpixel((x, y), value)
        if overlap < 3:
            raise ValueError("Dock clamp lacks a genuine deck attachment")
        clamps.append(dict(evidence, original_pixels=pixels, body_overlap_pixels=overlap))
    # Native service points lie outside the occupied deck. Source furniture
    # cannot fill those points or stand in for an actual contained craft.
    for point in APPROACHES:
        if result.getpixel(point) != 0:
            raise ValueError("Dock artwork obstructs an actual reserved approach")
    return result, dict(body=body_evidence, clamps=clamps,
                        protected_body_canvas_pixels=CANVAS[0] * CANVAS[1] - len(occupied),
                        native_sha256=hashlib.sha256(result.tobytes()).hexdigest())


def review_base(output):
    """Publish bounded whole-Dock and individual-clamp source contacts.

    @param output Fresh repository native-art proof directory.
    @return Exact base-review identity; tile/state/packing/runtime gates remain pending.
    @throws ValueError On unsafe output ownership or invalid original assembly.
    """
    output = output.resolve()
    # Separate immutable proof directories protect accepted originals and
    # shipped graphics while a later source task adds its own evidence.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("Dock base review requires a fresh native-art proof directory")
    native, evidence = native_base()
    contact = Image.new("RGB", (768, 640), (38, 43, 49))
    rgba = native.convert("RGBA").resize(contact.size, Image.Resampling.NEAREST)
    contact.paste(rgba, mask=rgba.getchannel("A"))
    clamp_contact = Image.new("RGB", (512, 224), (38, 43, 49))
    pen = ImageDraw.Draw(clamp_contact)
    components = []
    # Each diagnostic shows the genuine instance pixels already assembled,
    # not another cached conversion or a procedurally drawn replacement jaw.
    for slot, instance in enumerate(evidence["clamps"]):
        image = indexed_canvas()
        for point in instance["original_pixels"]:
            image.putpixel(tuple(point), native.getpixel(tuple(point)))
        components.append(image)
        cx, cy = CONTACTS[slot]
        rgba = image.crop((cx - 8, cy - 6, cx, cy)).convert("RGBA").resize((256, 192), Image.Resampling.NEAREST)
        clamp_contact.paste(rgba, (slot * 256, 24), rgba)
        pen.text((slot * 256 + 6, 6), f"slot{slot} edge{CONTACTS[slot]}", fill="white")
    output.mkdir(parents=True)
    native.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    clamp_contact.save(output / "clamp-contact.png")
    for slot, image in enumerate(components):
        image.save(output / f"clamp{slot}.png", transparency=0)
    report = output / "base-source-review.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, asset_id=ASSET, base=evidence,
                                     complete_source_coverage=False, packed=False, runtime_activated=False,
                                     artifacts=[art.identity(output / name) for name in
                                                ("native.png", "contact.png", "clamp-contact.png", "clamp0.png", "clamp1.png")]),
                                 indent=2) + "\n", encoding="utf-8")
    return art.identity(report)


def main():
    """Emit only an explicitly requested Dock body or assembled source image."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--body-state", choices=("complete", "blueprint", "partial"))
    parser.add_argument("--review-base", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.review_base:
        print(json.dumps(review_base(args.output)))
        return
    # A direct diagnostic write must stay in the native-art proof home and
    # preserve every existing proof and shipped payload.
    output = args.output.resolve()
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("Dock diagnostic requires a new native-art proof file")
    image, evidence = native_body(args.body_state) if args.body_state else native_base()
    output.parent.mkdir(parents=True, exist_ok=True)
    image.save(output, transparency=0)
    print(json.dumps(evidence))


if __name__ == "__main__":
    main()
