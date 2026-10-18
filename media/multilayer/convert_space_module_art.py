"""Register original fixed orbital module artwork against native world geometry.

@file convert_space_module_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13E
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group70
This source-only Core adapter preserves the native3x3 footprint and one original
approach. Artwork supplies no inventory, work, power, service or runtime sprite.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw
import convert_space_art as art
from tools.orbital_art_contract import validate_orbital_contract

ASSET = "module.nucleus"
PUBLIC = art.ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
CANVAS = (256, 192)
FLOOR_ORIGIN = (128, 64)
CELLS = [(x, y) for y in range(3) for x in range(3)]
ORIGINS = [(97 + 32 * (y - x), 31 + 16 * (x + y)) for x, y in CELLS]
FLOOR = [(128, 64), (224, 112), (128, 160), (32, 112)]
APPROACH = (224, 128)
COUPLING = (208, 120)
COUPLING_BOUNDS = (200, 112, 218, 127)


def registration_signature(candidate):
    """Bind the original whole model to its measured fixed native registration.

    @param candidate Exact retained source candidate, including its whole-model transform.
    @return Canonical SHA256 of immutable pixels, role and native projection.
    """
    fields = ("key", "asset_id", "path", "sha256", "directions", "native_transform")
    return hashlib.sha256(json.dumps({key: candidate[key] for key in fields}, sort_keys=True).encode("utf-8")).hexdigest()


def public_contract(document=None):
    """Require the actual fixed Core geometry before inspecting source artwork.

    @param document Optional public manifest for focused detached refusal checks.
    @return Actual module.nucleus row with unchanged cell anchors and source obligations.
    @throws ValueError On a foreign, rotated, shrunken or otherwise altered native family.
    """
    document = json.loads(PUBLIC.read_text(encoding="utf-8")) if document is None else document
    matches = [row for row in document["assets"] if row["asset_id"] == ASSET]
    if len(matches) != 1:
        raise ValueError("missing or duplicate Core public family")
    row = matches[0]
    validate_orbital_contract(row)
    # All nine world cells belong to one native Core. Source publication cannot
    # shrink its paid footprint, introduce rotation or move its eventual anchor.
    if (row["consumer"] != 11 or row["kind"] != "building" or row["native_size"] != [64, 64]
            or row["anchor"] != [-31, -33] or row["parts"] != ["back", "front"]
            or row["geometry"] != "per_tile" or row["bounds"] != [0, 0, 64, 64]
            or row["states"] != ["blueprint", "partial", "complete", "blocked"] or row["frames"] != 2):
        raise ValueError("Core public cell/state contract changed")
    return row


def native_body(key, document=None, public=None):
    """Convert one original Core body with a uniform scale and fixed floor reference.

    @param key Exact immutable original candidate selected for the Core base.
    @param document Optional detached Space provenance catalogue for refusal checks.
    @param public Optional public manifest; native geometry remains authoritative.
    @return Indexed native body and registration/coverage evidence; the coupling remains separate.
    @throws ValueError On invalid ancestry, geometry, crop, separator or native clipping.
    """
    public_contract(public)
    document = art.read_record() if document is None else document
    candidate = art.source_candidate(key, document)
    # A module is one fixed native site, not a vehicle heading or a rotation of
    # another building. A valid foreign receipt does not grant Core ownership.
    if candidate["asset_id"] != ASSET or candidate["directions"] != ["none"]:
        raise ValueError("Core original belongs to another family or orientation")
    definition = document["families"][ASSET]
    # Source metadata cannot create an additional operational berth or replace
    # the native tile/anchor contract while retaining an approved raw image.
    if (definition["native_approach_slots"] != 1 or definition["footprint"] != [3, 3]
            or definition["public_native_size"] != [64, 64] or definition["public_anchor"] != [-31, -33]):
        raise ValueError("Core source family geometry or approach capacity changed")
    signature = registration_signature(candidate)
    if definition.get("approved_base_registration") != signature:
        raise ValueError("approved Core base registration changed")
    transform = candidate["native_transform"]
    # One floor reference binds all nine future native cuts. The approach is
    # the existing Q16 cell centre at(x,y+3), outside the occupied footprint.
    if (transform["canvas"] != list(CANVAS) or transform["native_reference"] != list(FLOOR_ORIGIN)
            or transform["native_floor_corners"] != [list(point) for point in FLOOR]
            or transform["native_approach"] != list(APPROACH)):
        raise ValueError("Core whole-model floor/approach contract changed")
    with Image.open(art.ROOT / candidate["path"]) as original:
        original = original.convert("RGBA")
    if list(original.size) != candidate["image_size"]:
        raise ValueError("Core original dimensions changed")
    left, top, right, bottom = transform["crop_box"]
    # Ordinary rectangles select the complete original cutout. Padding beyond
    # raw dimensions cannot conceal clipped platform corners or machinery.
    if not (0 <= left < right <= original.width and 0 <= top < bottom <= original.height):
        raise ValueError("Core crop leaves original")
    crop = original.crop((left, top, right, bottom))
    bounds = crop.getchannel("A").point([0] * 16 + [255] * 240).getbbox()
    if bounds is None:
        raise ValueError("Core original is empty")
    if list(bounds) != transform["measured_subject_bbox_in_crop"]:
        raise ValueError("Core measured subject changed")
    if min(bounds[0], bounds[1], crop.width - bounds[2], crop.height - bounds[3]) < 16:
        raise ValueError("Core original lacks transparent clearance")
    if any(art.source_separator(pixel) for pixel in crop.get_flattened_data()):
        raise ValueError("Core source separator enters crop")
    numerator, denominator = transform["uniform_scale"]
    if not (0 < numerator < denominator):
        raise ValueError("Core source needs a positive uniform reduction")
    scale = numerator / denominator
    rx, ry = transform["source_reference"]
    # Measured original floor vertices must land on the same native3x3 diamond
    # under that one scale. A stretched or shifted platform cannot claim the
    # correct footprint merely by copying expected native coordinates.
    for source_point, native_point in zip(transform["source_floor_corners"], FLOOR, strict=True):
        mapped = (FLOOR_ORIGIN[0] + round((source_point[0] - rx) * scale),
                  FLOOR_ORIGIN[1] + round((source_point[1] - ry) * scale))
        if max(abs(mapped[i] - native_point[i]) for i in range(2)) > 2:
            raise ValueError("Core original floor does not match native3x3 projection")
    small = crop.resize((round(crop.width * scale), round(crop.height * scale)), Image.Resampling.BOX)
    offset = (FLOOR_ORIGIN[0] - round((rx - left) * scale), FLOOR_ORIGIN[1] - round((ry - top) * scale))
    native = Image.new("RGBA", CANVAS)
    # Verify all visible original pixels fit before assembly. Alpha compositing
    # onto the empty native canvas may not silently clip an oversized model.
    small_bounds = small.getchannel("A").point([0] * 16 + [255] * 240).getbbox()
    if small_bounds is None:
        raise ValueError("Core native reduction loses the model")
    placed = [small_bounds[0] + offset[0], small_bounds[1] + offset[1], small_bounds[2] + offset[0], small_bounds[3] + offset[1]]
    if not (0 <= placed[0] < placed[2] <= CANVAS[0] and 0 <= placed[1] < placed[3] <= CANVAS[1]):
        raise ValueError("Core original leaves native canvas")
    native.alpha_composite(small, offset)
    indexed = art.base.quantize(native, {})
    coverage = [0] * len(CELLS)
    # Whole-model approval must fit the actual64x64 cell canvases already. This
    # is coverage only; source partitioning and front/back ownership follow70.2.
    for py in range(CANVAS[1]):
        for px in range(CANVAS[0]):
            if indexed.getpixel((px, py)) == 0:
                continue
            eligible = [i for i, (ox, oy) in enumerate(ORIGINS) if 0 <= px - ox < 64 and 0 <= py - oy < 64]
            if not eligible:
                raise ValueError(f"Core original pixel leaves native cells: {px},{py}")
            for i in eligible:
                coverage[i] += 1
    if not all(coverage):
        raise ValueError("Core base omits a native world cell")
    return indexed, dict(original={name: candidate[name] for name in ("path", "sha256")},
                         signature=signature, transform=transform, native_subject_bounds=placed,
                         native_cell_origins=[list(point) for point in ORIGINS],
                         covered_pixels_per_cell=coverage, native_approach=list(APPROACH),
                         native_sha256=hashlib.sha256(indexed.tobytes()).hexdigest())


def coupling_source(document):
    """Register one genuine isolated coupling at the Core's original approach edge.

    @param document Exact Space source catalogue with approved body and coupling inputs.
    @return Indexed component-only canvas and immutable source/contact evidence.
    @throws ValueError On changed ownership, ancestry, registration, clearance or contact.
    """
    definition = document["families"][ASSET]
    candidate = art.source_candidate(definition["coupling_source"], document)
    # Separate original parts retain one physical station identity. A vehicle
    # part or another module cannot add a service berth through this source API.
    if (candidate["asset_id"] != ASSET or candidate["directions"] != ["none"]
            or candidate.get("component") != "service_coupling"):
        raise ValueError("foreign Core coupling source")
    signature = registration_signature(candidate)
    if signature != definition.get("approved_coupling_registration"):
        raise ValueError("approved Core coupling registration changed")
    transform = candidate["native_transform"]
    # The original single approach fixes the receiving point and small source
    # envelope; a fresh source signature cannot move or enlarge the berth.
    if transform["native_reference"] != list(COUPLING) or transform["native_bounds"] != list(COUPLING_BOUNDS):
        raise ValueError("Core original coupling misses the native approach edge")
    with Image.open(art.ROOT / candidate["path"]) as original:
        original = original.convert("RGBA")
    if list(original.size) != candidate["image_size"]:
        raise ValueError("Core coupling original dimensions changed")
    bounds = original.getchannel("A").point([0] * 16 + [255] * 240).getbbox()
    # The complete isolated original must retain its measured extent; a missing
    # cheek cannot be hidden by reusing an earlier component registration.
    if bounds is None or list(bounds) != transform["measured_subject_bbox"]:
        raise ValueError("Core coupling measured source changed")
    if min(bounds[0], bounds[1], original.width - bounds[2], original.height - bounds[3]) < 16:
        raise ValueError("Core coupling lacks original transparent clearance")
    if any(art.source_separator(pixel) for pixel in original.get_flattened_data()):
        raise ValueError("Core coupling contains a source separator")
    numerator, denominator = transform["uniform_scale"]
    if not (0 < numerator < denominator):
        raise ValueError("Core coupling needs a positive uniform reduction")
    scale = numerator / denominator
    crop = original.crop(bounds)
    small = crop.resize((round(crop.width * scale), round(crop.height * scale)), Image.Resampling.BOX)
    rx, ry = transform["source_reference"]
    offset = (COUPLING[0] - round((rx - bounds[0]) * scale), COUPLING[1] - round((ry - bounds[1]) * scale))
    visible = small.getchannel("A").point([0] * 16 + [255] * 240).getbbox()
    if visible is None:
        raise ValueError("Core reduction loses the original coupling")
    x0, y0, x1, y1 = COUPLING_BOUNDS
    # Refuse overflow before compositing, including disconnected source pixels;
    # canvas clipping must never silently repair an oversized original part.
    if not (x0 <= visible[0] + offset[0] < visible[2] + offset[0] <= x1
            and y0 <= visible[1] + offset[1] < visible[3] + offset[1] <= y1):
        raise ValueError("Core coupling leaves original receiving envelope")
    component = Image.new("RGBA", CANVAS)
    component.alpha_composite(small, offset)
    indexed = art.base.quantize(component, {})
    actual = indexed.getbbox()
    x0, y0, x1, y1 = COUPLING_BOUNDS
    # Tiny original hardware must survive reduction entirely inside its actual
    # receiving area. A larger crop cannot silently become a second dock floor.
    if actual is None or not (x0 <= actual[0] < actual[2] <= x1 and y0 <= actual[1] < actual[3] <= y1):
        raise ValueError("Core coupling leaves original receiving envelope")
    if indexed.getpixel(COUPLING) == 0:
        raise ValueError("Core coupling misses its actual mounting reference")
    return indexed, dict(original={name: candidate[name] for name in ("path", "sha256")},
                         signature=signature, transform=transform, native_bounds=list(actual),
                         native_contact=list(COUPLING))


def native_base(key, document=None, public=None):
    """Assemble the original body and one coupling without changing native occupancy.

    @param key Exact original Core body candidate; no latest-file discovery.
    @param document Optional detached source catalogue for focused refusal checks.
    @param public Optional public manifest whose fixed native geometry remains authoritative.
    @return Indexed complete base and original body/coupling/protection evidence.
    @throws ValueError On an invalid component, floating mount or native cell overflow.
    """
    document = art.read_record() if document is None else document
    body, evidence = native_body(key, document, public)
    component, source = coupling_source(document)
    pose = body.copy()
    changed, overlap = [], 0
    # Original component pixels are the sole additions. Every other body pixel
    # stays exact; no algorithm paints a mouth, repairs alpha or clears a floor.
    for y in range(CANVAS[1]):
        for x in range(CANVAS[0]):
            value = component.getpixel((x, y))
            if value == 0:
                continue
            if not any(0 <= x - ox < 64 and 0 <= y - oy < 64 for ox, oy in ORIGINS):
                raise ValueError("Core coupling leaves actual native cell canvases")
            overlap += body.getpixel((x, y)) != 0
            pose.putpixel((x, y), value)
            changed.append([x, y])
    if overlap < 3:
        raise ValueError("Core coupling floats outside its original platform edge")
    evidence.update(body_native_sha256=evidence["native_sha256"], coupling=source,
                    coupling_pixels=changed, original_mount_overlap_pixels=overlap,
                    protected_body_pixels=CANVAS[0] * CANVAS[1] - len(changed),
                    native_sha256=hashlib.sha256(pose.tobytes()).hexdigest())
    return pose, evidence


def review_base(key, output):
    """Publish native and enlarged Core source contacts without claiming cut/state coverage.

    @param key Exact selected immutable original base candidate.
    @param output Fresh directory under the repository's native-art proof owner.
    @return Source-review identity; native packing and runtime remain untouched.
    @throws ValueError On unsafe output ownership or an invalid original.
    """
    output = output.resolve()
    # A fresh proof keeps previous accepted pixels and immutable originals
    # intact; this source review cannot replace any shipped payload.
    if not output.is_relative_to(art.ROOT / "logs/proofs/mlm/native-art") or output.exists():
        raise ValueError("Core review requires a fresh native-art proof directory")
    native, evidence = native_base(key)
    rgba = native.convert("RGBA")
    contact = Image.new("RGB", (1024, 1088), (38, 43, 49))
    contact.paste(rgba, (384, 32), rgba)
    enlarged = rgba.resize((1024, 768), Image.Resampling.NEAREST)
    contact.paste(enlarged, (0, 256), enlarged)
    drawing = ImageDraw.Draw(contact)
    drawing.text((16, 8), "Core complete base: fixed3x3 native source; cuts/states/packing/runtime pending", fill="white")
    output.mkdir(parents=True)
    native.save(output / "native.png", transparency=0)
    contact.save(output / "contact.png")
    report = output / "base-source-review.json"
    report.write_text(json.dumps(dict(_documentation=__doc__, asset_id=ASSET, base=evidence,
                                     native_cell=[64, 64], anchor=[-31, -33], footprint=[3, 3],
                                     complete_source_coverage=False, packed=False, runtime_activated=False,
                                     artifacts=[art.identity(output / name) for name in ("native.png", "contact.png")]),
                                 indent=2) + "\n", encoding="utf-8")
    return art.identity(report)


def main():
    """Review one explicit original Core base without building or activating native sprites."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(review_base(args.candidate, args.output)))


if __name__ == "__main__":
    main()
