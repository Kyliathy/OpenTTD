"""Register whole original site components without altering native world geometry.

@file orbital_site_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13G
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group72
This pure source helper grants no stock, capacity, service, work or utility.
Earlier accepted family converters remain unchanged.
"""
import hashlib

from PIL import Image

import convert_space_art as art
from tools.orbital_tile_art import signature


def registration_signature(candidate):
    """Bind original pixels, physical role and one uniform native registration.

    @param candidate Explicit selected original or documented material reuse.
    @return Canonical SHA256 protecting identity, ancestry role and geometry.
    """
    fields = ("key", "asset_id", "path", "sha256", "directions", "state",
              "component", "reused_original_key", "native_transform")
    return signature({key: candidate.get(key) for key in fields})


def original_image(candidate):
    """Read the entire original rectangle with its genuine generated alpha.

    @param candidate Ancestry-verified original and measured whole-image transform.
    @return Original RGBA image without painting, padding, alpha repair or warping.
    @throws ValueError On changed dimensions, clipping, measurement or separators.
    """
    with Image.open(art.ROOT / candidate["path"]) as source:
        image = source.convert("RGBA")
    transform = candidate["native_transform"]
    # Complete isolated originals require no mask or synthetic border. Checking
    # both dimensions and rectangle prevents a crop from concealing lost hardware.
    if list(image.size) != candidate["image_size"] or transform["crop_box"] != [0, 0, *image.size]:
        raise ValueError("site original dimensions or whole-source crop changed")
    bounds = image.getchannel("A").point([0] * 16 + [255] * 240).getbbox()
    if bounds is None or list(bounds) != transform["measured_subject_bbox_in_crop"]:
        raise ValueError("site original subject measurement changed")
    if min(bounds[0], bounds[1], image.width - bounds[2], image.height - bounds[3]) < 16:
        raise ValueError("site original lacks transparent clearance")
    if any(art.source_separator(pixel) for pixel in image.get_flattened_data()):
        raise ValueError("site separator enters original subject")
    return image


def registered_original(candidate, native_reference, geometry):
    """Uniformly reduce one whole original into actual native sorting canvases.

    @param candidate Verified original with an independently approved transform.
    @param native_reference Existing floor or physical coupling mounting reference.
    @param geometry Family-validated canvas, cell size and actual world-cell origins.
    @return Indexed whole canvas and measured original placement/coverage evidence.
    @throws ValueError On invalid reduction or original pixels without a real cell owner.
    """
    image = original_image(candidate)
    transform = candidate["native_transform"]
    numerator, denominator = transform["uniform_scale"]
    if not (0 < numerator < denominator):
        raise ValueError("site original needs a positive uniform reduction")
    scale = numerator / denominator
    small = image.resize((round(image.width * scale), round(image.height * scale)), Image.Resampling.BOX)
    rx, ry = transform["source_reference"]
    offset = (native_reference[0] - round(rx * scale), native_reference[1] - round(ry * scale))
    bounds = small.getchannel("A").point([0] * 16 + [255] * 240).getbbox()
    if bounds is None:
        raise ValueError("site original vanishes under reduction")
    placed = [bounds[i] + offset[i % 2] for i in range(4)]
    width, height = geometry["native_canvas"]
    # Observe the whole original before assembly. A destination image must
    # never hide oversized buildings or a clipped coupling through compositing.
    if not (0 <= placed[0] < placed[2] <= width and 0 <= placed[1] < placed[3] <= height):
        raise ValueError("site original leaves native canvas before assembly")
    native = Image.new("RGBA", (width, height))
    native.alpha_composite(small, offset)
    indexed = art.base.quantize(native, {})
    origins = geometry["native_cell_origins"]
    cell_width, cell_height = geometry["native_cell"]
    coverage = [0] * len(origins)
    # A whole source is not repeated on every tile. Each actual opaque pixel
    # must fit a native sorting canvas before later lossless ownership cuts.
    for y in range(height):
        for x in range(width):
            if not indexed.getpixel((x, y)):
                continue
            owners = [i for i, (ox, oy) in enumerate(origins)
                      if 0 <= x - ox < cell_width and 0 <= y - oy < cell_height]
            if not owners:
                raise ValueError(f"site original pixel leaves actual tile canvases: {x},{y}")
            for owner in owners:
                coverage[owner] += 1
    return indexed, dict(candidate=candidate["key"], original=art.identity(art.ROOT / candidate["path"]),
                         signature=registration_signature(candidate), transform=transform,
                         native_subject_bounds=placed, covered_pixels_per_cell=coverage,
                         native_sha256=hashlib.sha256(indexed.tobytes()).hexdigest())


def floor_original(candidate, geometry):
    """Check measured original vertices against the family's fixed native footprint.

    @param candidate Approved physical stage body, never an inferred source variant.
    @param geometry Actual native floor origin, four corners and sorting canvases.
    @return Indexed genuine stage and measured native geometry/source evidence.
    @throws ValueError On floor drift, unsupported projection or a missing actual world cell.
    """
    transform = candidate["native_transform"]
    # An authoring signature cannot move native world tiles or reinterpret
    # the original layout as a newly saved building orientation.
    if (transform["canvas"] != geometry["native_canvas"]
            or transform["native_reference"] != geometry["floor_origin"]
            or transform["native_floor_corners"] != geometry["floor_corners"]):
        raise ValueError("site whole-model native floor contract changed")
    numerator, denominator = transform["uniform_scale"]
    if not (0 < numerator < denominator):
        raise ValueError("site original needs a positive uniform reduction")
    scale = numerator / denominator
    rx, ry = transform["source_reference"]
    origin = geometry["floor_origin"]
    # Measured corners use the same uniform scale as every original pixel.
    # Two native pixels cover source-edge measurement, not a different camera.
    for source, expected in zip(transform["source_floor_corners"], geometry["floor_corners"], strict=True):
        actual = (origin[0] + round((source[0] - rx) * scale),
                  origin[1] + round((source[1] - ry) * scale))
        if max(abs(actual[i] - expected[i]) for i in range(2)) > 2:
            raise ValueError("site original floor does not match native projection")
    image, evidence = registered_original(candidate, origin, geometry)
    if not all(evidence["covered_pixels_per_cell"]):
        raise ValueError("site body omits an actual world cell")
    return image, evidence
