"""Uniform source registration for the actual native launch-center world cells.

@file launch_facility_source.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group61C.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group2B/task16.2.
Only original-pixel reduction and the established site registration/quantizer
are used. These helpers create no artwork, native station or runtime sprites.
"""

from fractions import Fraction
import hashlib
import json

from PIL import Image, ImageDraw, PngImagePlugin

from tools.launch_facility_art_contract import ASSET, launch_facility_contract
from tools import orbital_site_art as site

COMPONENT_WIDTHS = {"workshop": 108, "pad": 80, "recovery": 80, "storage": 144}


def geometry(direction):
    """Describe real row-major cells through the verified viewport projection.

    @param direction Existing NE/SE/SW/NW source label bound to a native rotation.
    @return Fixed whole/cell canvases, floor vertices and actual sorting contacts.
    """
    layouts = launch_facility_contract()["native_rotations"]
    if direction not in layouts:
        raise ValueError("unknown launch-center native source view")
    layout = layouts[direction]
    width, height = layout["footprint"]
    origin = [160 + 16 * (width - height), 64]
    cells = [[x, y] for y in range(height) for x in range(width)]
    # Actual world X projects left and Y right. Sorting canvases must follow
    # that same basis, otherwise valid hardware would appear in another zone.
    origins = [[origin[0] + 32 * (y - x) - 31, origin[1] + 16 * (x + y) - 33] for x, y in cells]
    floor = [origin, [origin[0] + 32 * height, origin[1] + 16 * height],
             [origin[0] + 32 * (height - width), origin[1] + 16 * (width + height)],
             [origin[0] - 32 * width, origin[1] + 16 * width]]
    return dict(asset_id=ASSET, direction=direction, native_rotation=layout["rotation"],
                native_canvas=[320, 224], native_cell=[64, 64], world_cells=cells,
                native_cell_origins=origins, floor_origin=origin, floor_corners=floor,
                anchor=[-31, -33], native_sort_contact=[31, 48])


def visible_floor_extrema(image):
    """Propose measurable vertices when all four floor tips are visibly exposed.

    This reports geometry only. A model review must confirm that each extreme
    belongs to the floor rather than hardware before registration is approved.
    @param image Complete unmodified original RGBA image.
    @return North/east/south/west source vertices at the declared alpha128 edge.
    """
    alpha = image.getchannel("A").point([0] * 128 + [255] * 128)
    bounds = alpha.getbbox()
    if bounds is None:
        raise ValueError("launch-center original has no visible floor")
    left, top, right, bottom = bounds
    north = [x for x in range(left, right) if alpha.getpixel((x, top))]
    east = [y for y in range(top, bottom) if alpha.getpixel((right - 1, y))]
    south = [x for x in range(left, right) if alpha.getpixel((x, bottom - 1))]
    west = [y for y in range(top, bottom) if alpha.getpixel((left, y))]
    return [[sum(north) / len(north), top], [right - 1, sum(east) / len(east)],
            [sum(south) / len(south), bottom - 1], [left, sum(west) / len(west)]]


def visible_component_feet(image):
    """Propose three visible box-foot contacts for an apron-free component.

    A model reviewer must confirm that these belong to the physical base;
    roof overhangs or flared feet require another explicit measured contact.
    @param image Entire unmodified original RGBA component.
    @return North/east/south/west contacts, with the hidden north inferred from the rectangle.
    """
    alpha = image.getchannel("A").point([0] * 128 + [255] * 128)
    bounds = alpha.getbbox()
    if bounds is None:
        raise ValueError("launch-center component has no visible hardware")
    left, top, right, bottom = bounds
    east = [right - 1, max(y for y in range(top, bottom) if alpha.getpixel((right - 1, y)))]
    west = [left, max(y for y in range(top, bottom) if alpha.getpixel((left, y)))]
    south_x = [x for x in range(left, right) if alpha.getpixel((x, bottom - 1))]
    south = [sum(south_x) / len(south_x), bottom - 1]
    north = [east[axis] + west[axis] - south[axis] for axis in (0, 1)]
    return [north, east, south, west]


def component_geometry(zone, direction):
    """Project one actual exclusive native work area without inventing floor cells.

    @param zone Existing pad/workshop/recovery/storage zone name.
    @param direction Existing source view bound to a native quarter turn.
    @return Whole-canvas geometry restricted to that zone's actual world cells.
    """
    native = geometry(direction)
    zones = launch_facility_contract()["native_rotations"][direction]["zones"]
    if zone not in zones:
        raise ValueError("unknown launch-center physical source zone")
    x0, y0, width, height = zones[zone]
    ox, oy = native["floor_origin"]
    origin = [ox + 32 * (y0 - x0), oy + 16 * (x0 + y0)]
    cells = [[x, y] for y in range(y0, y0 + height) for x in range(x0, x0 + width)]
    # Workshop/plate/store pixels must fit their original native zone sorting
    # canvases. Other zones cannot lend fictitious cell coverage to a component.
    origins = [[ox + 32 * (y - x) - 31, oy + 16 * (x + y) - 33] for x, y in cells]
    native.update(component=zone, world_cells=cells, native_cell_origins=origins, floor_origin=origin,
                  floor_corners=[origin, [origin[0] + 32 * height, origin[1] + 16 * height],
                                 [origin[0] + 32 * (height - width), origin[1] + 16 * (width + height)],
                                 [origin[0] - 32 * width, origin[1] + 16 * width]])
    return native


def fit_transform(source_corners, direction, zone=None):
    """Fit one uniform reduction and translation without shearing source pixels.

    @param source_corners Four independently reviewed north/east/south/west tips.
    @param direction Existing native source rotation label.
    @param zone Optional actual native component; absence fits a whole-facility original.
    @return Proposed transform and observed native-corner errors; never approval.
    """
    native = geometry(direction) if zone is None else component_geometry(zone, direction)
    targets = native["floor_corners"]
    # Exactly four distinct floor tips are needed to detect unequal world-axis
    # scales. Fitting only the overall width could hide a non-native footprint.
    if len(source_corners) != 4 or len({tuple(point) for point in source_corners}) != 4:
        raise ValueError("launch-center floor needs four distinct measured tips")
    source_mean = [sum(point[axis] for point in source_corners) / 4 for axis in (0, 1)]
    target_mean = [sum(point[axis] for point in targets) / 4 for axis in (0, 1)]
    numerator = sum((source[axis] - source_mean[axis]) * (target[axis] - target_mean[axis])
                    for source, target in zip(source_corners, targets) for axis in (0, 1))
    denominator = sum((source[axis] - source_mean[axis]) ** 2 for source in source_corners for axis in (0, 1))
    scale = numerator / denominator
    if not 0 < scale < 1:
        raise ValueError("launch-center original needs a positive uniform reduction")
    fraction = Fraction(scale).limit_denominator(100000)
    scale = float(fraction)
    offset = [round(target_mean[axis] - scale * source_mean[axis]) for axis in (0, 1)]
    reference = [(native["floor_origin"][axis] - offset[axis]) / scale for axis in (0, 1)]
    actual = [[native["floor_origin"][axis] + round((point[axis] - reference[axis]) * scale)
               for axis in (0, 1)] for point in source_corners]
    error = max(abs(observed[axis] - expected[axis]) for observed, expected in zip(actual, targets) for axis in (0, 1))
    transform = dict(canvas=native["native_canvas"], native_reference=native["floor_origin"],
                     native_floor_corners=targets, uniform_scale=[fraction.numerator, fraction.denominator],
                     source_reference=reference, source_floor_corners=source_corners)
    return transform, dict(actual_native_corners=actual, expected_native_corners=targets,
                           maximum_corner_error=error, permitted_corner_error=2)


def registered_base(candidate):
    """Check one whole original against native cells using the existing site pipeline.

    @param candidate Explicit original identity, reviewed corners and uniform transform.
    @return Indexed whole original and original-pixel/native-coverage evidence.
    @throws ValueError For changed pixels, dimensions, clearance, floor or cell drift.
    """
    path = site.art.ROOT / candidate["path"]
    if hashlib.sha256(path.read_bytes()).hexdigest() != candidate["sha256"]:
        raise ValueError("launch-center original bytes changed")
    # A base is one isolated original of the existing facility. A multi-view
    # sheet or another family cannot be silently treated as one native pose.
    if candidate.get("asset_id") != ASSET or len(candidate.get("directions", [])) != 1:
        raise ValueError("launch-center base requires one complete isolated view")
    direction = candidate["directions"][0]
    native = geometry(direction)
    return site.floor_original(candidate, native)


def registered_component(candidate):
    """Register original hardware through its actual native-zone contact geometry.

    @param candidate Original pixels, physical ground reference and native-zone transform.
    @return Indexed whole-canvas component and exact original/cell coverage evidence.
    @throws ValueError For changed source bytes, footprint, clearance or zone ownership.
    """
    path = site.art.ROOT / candidate["path"]
    if hashlib.sha256(path.read_bytes()).hexdigest() != candidate["sha256"]:
        raise ValueError("launch-center component original bytes changed")
    # Each original supplies exactly one physical component in one true view;
    # another family or a sheet cannot authorize native zone coverage.
    if candidate.get("asset_id") != ASSET or len(candidate.get("directions", [])) != 1:
        raise ValueError("launch-center component requires one isolated native view")
    native = component_geometry(candidate["component"], candidate["directions"][0])
    reference = [(native["floor_corners"][0][axis] + native["floor_corners"][2][axis]) // 2 for axis in (0, 1)]
    transform = candidate["native_transform"]
    measured = transform["measured_subject_bbox_in_crop"]
    scale = Fraction(COMPONENT_WIDTHS[native["component"]], measured[2] - measured[0])
    # An apron-free machine has a ground reference, not authored whole-floor
    # corners. The native ground supplies the zone floor; every hardware pixel
    # must fit that exact zone's sorting canvases at its frozen physical size.
    if (transform["canvas"] != native["native_canvas"] or transform["native_reference"] != reference
            or transform["native_zone_corners"] != native["floor_corners"]
            or transform["component"] != native["component"]
            or candidate["native_width_target"] != COMPONENT_WIDTHS[native["component"]]
            or transform["uniform_scale"] != [scale.numerator, scale.denominator]
            or transform["source_reference"] != candidate["contact_review"]["source_reference"]):
        raise ValueError("launch-center component changes its native zone/contact")
    indexed, evidence = site.registered_original(candidate, reference, native)
    if not all(evidence["covered_pixels_per_cell"]):
        raise ValueError("launch-center component omits an actual zone cell")
    return indexed, evidence


def propose_component_transform(candidate, source_reference):
    """Reduce original hardware uniformly to the model's declared native width.

    This is a registration proposal for review, not selection. Native ground
    remains a separate existing renderer input; no floor or hardware is painted.
    @param candidate Explicit original identity, actual zone and single native view.
    @param source_reference Measured physical ground reference in source pixels.
    @return Detached transform and actual original-image measurements.
    """
    native = component_geometry(candidate["component"], candidate["directions"][0])
    with Image.open(site.art.ROOT / candidate["path"]) as source:
        image = source.convert("RGBA")
    bounds = image.getchannel("A").point([0] * 16 + [255] * 240).getbbox()
    if bounds is None:
        raise ValueError("launch-center component has no original pixels")
    width = COMPONENT_WIDTHS[native["component"]]
    fraction = Fraction(width, bounds[2] - bounds[0])
    reference = [(native["floor_corners"][0][axis] + native["floor_corners"][2][axis]) // 2 for axis in (0, 1)]
    transform = dict(canvas=native["native_canvas"], native_reference=reference,
                     native_zone_corners=native["floor_corners"], component=native["component"],
                     source_reference=source_reference, uniform_scale=[fraction.numerator, fraction.denominator],
                     crop_box=[0, 0, *image.size], measured_subject_bbox_in_crop=list(bounds))
    return transform, dict(image_size=list(image.size), native_width_target=width)


def write_component_template(zone, direction):
    """Rebuild a labelled geometry-only low hardware box for original authoring.

    @param zone Actual native component whose contacts constrain the original.
    @param direction Existing native quarter-turn view label.
    @return PNG/JSON reference paths; these template pixels are never source art.
    """
    native = component_geometry(zone, direction)
    layout = launch_facility_contract()["native_rotations"][direction]["zones"][zone]
    width, height = layout[2:]
    source_origin = [512 + 64 * (width - height), 350]
    contacts = [source_origin,
                [source_origin[0] + 128 * height, source_origin[1] + 64 * height],
                [source_origin[0] + 128 * (height - width), source_origin[1] + 64 * (width + height)],
                [source_origin[0] - 128 * width, source_origin[1] + 64 * width]]
    height_pixels = dict(workshop=96, pad=16, recovery=16, storage=40)[zone]
    roof = [[x, y - height_pixels] for x, y in contacts]
    canvas = Image.new("RGBA", (1024, 1024))
    draw = ImageDraw.Draw(canvas)
    # The diagram fixes a physical box and its base contacts. Face colours,
    # labels and hidden edges are authoring guidance, never machinery pixels.
    draw.polygon([contacts[1], contacts[2], roof[2], roof[1]], fill="#8796a5", outline="#253449", width=3)
    draw.polygon([contacts[2], contacts[3], roof[3], roof[2]], fill="#a5b5c3", outline="#253449", width=3)
    draw.polygon(roof, fill="#d3dfe7", outline="#253449", width=3)
    for label, point in zip(("N hidden foot", "E foot", "S foot", "W foot"), contacts):
        x, y = point
        draw.ellipse((x - 5, y - 5, x + 5, y + 5), fill="#f0b44c")
        draw.text((x - 36, y + 9), label, fill="white")
    draw.text((80, 80), f"GEOMETRY ONLY: {zone} {direction}, {width}x{height} native cells", fill="white")
    draw.text((80, 104), "World X down-left; Y down-right; no surrounding apron", fill="white")
    directory = site.art.ROOT / "logs/proofs/mlm/native-art/space/launch-center-components-v1"
    directory.mkdir(parents=True, exist_ok=True)
    image_path = directory / f"{zone}-{direction}-template.png"
    info = PngImagePlugin.PngInfo()
    info.add_text("Description", "r2mlvs14-remaining-assets-and-runtime-integration.md Group61CB: geometry reference only, not source art")
    canvas.save(image_path, pnginfo=info)
    metadata = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61CB: reproducible native-zone geometry template, not original art.",
                    geometry=native, source_canvas=[1024, 1024], source_contacts=contacts,
                    scale_from_native=4, box_height=height_pixels, image=site.art.identity(image_path))
    data_path = image_path.with_suffix(".json")
    data_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    return image_path, data_path
