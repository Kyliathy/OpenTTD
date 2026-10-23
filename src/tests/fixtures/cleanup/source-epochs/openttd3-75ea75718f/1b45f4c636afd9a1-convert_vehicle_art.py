"""Stage selected M1 rolling-stock pixels with explicit partial source coverage.

@file convert_vehicle_art.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups 14-18
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group 19
@see media/multilayer/source/m1/vehicle-generation-record.json
@see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 5 (envelopes, D6)
Originals/candidates stay immutable. Conversion performs native resampling,
declared authored-component frame assembly and DOS palette packing. Missing
states/animation are never filled by copying an unanimated frame.
"""

import argparse
import hashlib
import json
import math

from PIL import Image, ImageDraw

import convert_m1_art as base
from convert_terrain_art import png_bytes


RECORD = base.ASSETS / "vehicle-generation-record.json"
REPORT = base.OUTPUT / "vehicle-details-manifest.json"
REVIEW = base.REVIEW / "vehicle-details-review.png"
DIRECTIONS = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"]
# Per-view safe envelopes retain the projected rail-unit proportions; the full
# native canvas and public anchor stay identical. Runtime coupler review remains
# required before these staged sources can count as integrated train graphics.
ENVELOPES = {"N": (16, 24), "NE": (24, 24), "E": (32, 24), "SE": (24, 24),
             "S": (16, 24), "SW": (24, 24), "W": (32, 24), "NW": (24, 24)}


def source_cells(source):
    """Cut reviewed irregular sheets without trusting generated grid alignment.

    @param source Immutable source with a grid and optional explicit cell bounds.
    @return Rows of original RGBA cell pixels and their source rectangles.
    @throws ValueError On missing, overlapping or out-of-image registered cells.
    """
    if "source_cell_boxes" not in source:
        return base.source_cells(source)
    image = Image.open(base.repository_path(source["path"])).convert("RGBA")
    columns, rows = source["grid"]
    boxes = source["source_cell_boxes"]
    if len(boxes) != columns * rows:
        raise ValueError("explicit vehicle cell count disagrees with grid")
    cells = []
    # Measured crop rectangles compensate for uneven generated gutters. Crops
    # preserve original pixels, and actual selected equipment is registered later
    # onto the retained native chassis; they never synthesize a missing heading.
    for index, (left, top, right, bottom) in enumerate(boxes):
        if not (0 <= left < right <= image.width and 0 <= top < bottom <= image.height):
            raise ValueError("explicit vehicle cell leaves original image")
        if any(left < other[2] and right > other[0] and top < other[3] and bottom > other[1] for other in boxes[:index]):
            # Overlapping crops could count the same generated pixels as two
            # directions/frames, concealing a genuinely missing source pose.
            raise ValueError("explicit vehicle source cells overlap")
        if index % columns == 0:
            cells.append([])
        cell = image.crop((left, top, right, bottom))
        if cell.getchannel("A").point(lambda value: 255 if value >= 128 else 0).getbbox() is None:
            raise ValueError("empty explicitly registered vehicle cell")
        cells[-1].append(cell)
    return cells, boxes


def native_pose(subject, source, direction, state="idle"):
    """Reuse a selected native atlas cell or resample a retained source crop.

    @param subject Original selected RGBA crop, with alpha preserved.
    @param source Provenance indicating whether this is an existing native pose.
    @param direction Genuine engine view owning its approved safe envelope.
    @param state Source load/work state owning any ground-contact calibration.
    @return RGBA native canvas with the same fixed world anchor and contacts.
    @throws ValueError If a declared native pose has another canvas size.
    """
    if source.get("pose_sampling") == "native":
        if subject.size != (64, 48):
            raise ValueError("selected existing native vehicle pose has wrong canvas")
        if "native_translation" in source:
            translated = Image.new("RGBA", (64, 48))
            offsets = source["native_translation"]
            translated.paste(subject, tuple(offsets.get(state + "/" + direction, offsets.get(direction, [0, 0]))))
            return translated
        return subject.copy()
    return base.fit(subject, (64, 48), ENVELOPES[direction])


def opaque_box(image):
    """Measure the drawn part of a pose the way the packer keeps it.

    @param image RGBA native pose.
    @return Bounding box (left, top, right, bottom) of pixels with alpha >= 128, or None when empty.
    """
    return image.getchannel("A").point(lambda value: 255 if value >= 128 else 0).getbbox()


def envelope_excess(image, direction):
    """Compare a pose with its view's safe envelope (r2df decision D6).

    @param image RGBA native pose.
    @param direction Engine view owning the envelope.
    @return Opaque (width, height) when either exceeds the envelope, otherwise None.
    """
    box = opaque_box(image)
    if box is None:
        return None
    width, height = box[2] - box[0], box[3] - box[1]
    limit = ENVELOPES[direction]
    return None if width <= limit[0] and height <= limit[1] else (width, height)


def rederives_envelope(source, sources):
    """Whether a source's poses are re-derived inside the envelopes.

    The declaration sits on a native chassis source; component sources painted onto that chassis inherit it, so
    every load/work state of the vehicle shrinks by the same transform and keeps its cues registered.

    @param source Source record of the pose.
    @param sources Source-key lookup.
    @return True when the source or its component reference declares envelope_fit.
    """
    if source.get("envelope_fit") == "rederive":
        return True
    reference = sources.get(source.get("component_reference"))
    return reference is not None and reference.get("envelope_fit") == "rederive"


def envelope_transform(chassis, direction):
    """Work out how an oversize chassis is re-derived inside its envelope (r2df Group 5, D6).

    The vehicle keeps its gameplay length; only its art shrinks. The opaque subject is scaled uniformly, and its
    ground contact (bottom row) and horizontal centre stay where they were, so the packer's anchor, the rails and
    the couplers line up as before. Every frame painted onto the same chassis uses this one transform.

    @param chassis RGBA native pose whose opaque box decides the transform.
    @param direction Engine view owning the envelope.
    @return (box, size, origin): the opaque box, its scaled size and where it is pasted; None when it already fits.
    """
    box = opaque_box(chassis)
    if box is None:
        return None
    width, height = box[2] - box[0], box[3] - box[1]
    limit = ENVELOPES[direction]
    scale = min(limit[0] / width, limit[1] / height)
    if scale >= 1:
        return None
    size = (max(1, round(width * scale)), max(1, round(height * scale)))
    return box, size, (round((box[0] + box[2] - size[0]) / 2), box[3] - size[1])


def apply_envelope_transform(image, transform):
    """Scale a pose with a chassis transform, using the established BOX resampling.

    @param image RGBA native pose sharing the chassis's alpha.
    @param transform Result of envelope_transform, or None to keep the pose.
    @return RGBA native canvas of the same size.
    """
    if transform is None:
        return image
    box, size, origin = transform
    canvas = Image.new("RGBA", image.size)
    canvas.paste(image.crop(box).resize(size, Image.Resampling.BOX), origin)
    return canvas


def transform_region(region, transform):
    """Map a registered equipment region through a chassis transform, rounded outward and kept on the canvas.

    @param region Native (x, y, width, height) region.
    @param transform Result of envelope_transform, or None.
    @return The region covering the same equipment after scaling.
    """
    if transform is None:
        return list(region)
    box, size, origin = transform
    x, y, width, height = region
    scale_x, scale_y = size[0] / (box[2] - box[0]), size[1] / (box[3] - box[1])
    left = max(0, origin[0] + math.floor((x - box[0]) * scale_x))
    top = max(0, origin[1] + math.floor((y - box[1]) * scale_y))
    right = min(64, max(left + 1, origin[0] + math.ceil((x + width - box[0]) * scale_x)))
    bottom = min(48, max(top + 1, origin[1] + math.ceil((y + height - box[1]) * scale_y)))
    return [left, top, right - left, bottom - top]


def component_regions(source, index):
    """Resolve selected authored equipment regions without expanding their ownership.

    @param source Source record with registered source/target equipment regions.
    @param index Pose index, retaining the source's real state/frame/view mapping.
    @return Ordered pairs of source and target rectangles in native coordinates.
    @throws ValueError When source/target registrations have different lengths.
    """
    if "component_source_regions" in source:
        before = source["component_source_regions"][index]
        after = source["component_target_regions"][index]
    else:
        before = [source["component_source_boxes"][index]]
        after = [source["component_target_boxes"][index]]
    if len(before) != len(after):
        raise ValueError("vehicle component source/target region counts disagree")
    return list(zip(before, after))


def assemble_component(pixels, source, index, direction, sources, cache):
    """Pack an authored cue onto its declared immutable native chassis.

    @param pixels Selected tool-authored pose resampled to the native canvas.
    @param source Provenance and explicit component/reference registration.
    @param index Selected pose index in the source's declared mapping.
    @param direction Actual engine view for the fixed reference chassis.
    @param sources Immutable source-key lookup, separate from gameplay identities.
    @param cache Native reference images cached by source key and engine direction.
    @return Native RGBA frame with original chassis alpha and only authored cue pixels changed.
    @throws ValueError On foreign geometry, unsupported components or unregistered regions.
    """
    reference = sources[source["component_reference"]]
    if reference["asset_id"] != source["asset_id"] or reference["part"] != source["part"] or reference["frame"] != 0:
        # The component can change only this model/part's selected chassis; a
        # foreign vehicle or animated reference would conceal geometry changes.
        raise ValueError("vehicle component has no fixed own-part reference")
    if source["component_name"] not in {"cab_beacon", "equipment_indicator", "running_gear", "cutter_and_work_status", "service_equipment"}:
        raise ValueError("unsupported selected vehicle component")
    reference_state = source.get("component_reference_state", "idle")
    if reference_state == "pose_state":
        reference_state = source["poses"][index]["state"]
    key = (reference["key"], direction, reference_state)
    if key not in cache:
        cells, _boxes = source_cells(reference)
        if "poses" in reference:
            pose, mapping = next((index, row) for index, row in enumerate(reference["poses"])
                                 if row["direction"] == direction and row["state"] == reference_state and row["frame"] == 0)
            column, row = mapping["source_cell"]
        else:
            pose = reference["directions"].index(direction)
            column, row = pose % 4, pose // 4
        subject = cells[row][column].crop(reference["native_crop_boxes"][pose])
        cache[key] = native_pose(subject, reference, direction, reference_state)
    image = cache[key].copy()
    # A physical part may have several visible wheels or a disk plus its status
    # lamp. Select their authored pixels independently, retaining one fixed body
    # and its contacts rather than accepting generated chassis drift as motion.
    for source_box, target_box in component_regions(source, index):
        sx, sy, width, height = source_box
        tx, ty, target_width, target_height = target_box
        if (width, height) != (target_width, target_height) or not (0 < width <= 64 and 0 < height <= 48 and 0 <= sx <= 64 - width and 0 <= sy <= 48 - height and 0 <= tx <= 64 - width and 0 <= ty <= 48 - height):
            # Component registration cannot scale or leave the native canvas;
            # otherwise another source region could shift contacts or erase art.
            raise ValueError("vehicle component is not registered inside its canvas")
        # Real source and chassis pixels must both be opaque. Transparent
        # padding cannot erase the body or introduce a detached moving highlight.
        for row in range(height):
            for column in range(width):
                authored = pixels.getpixel((sx + column, sy + row))
                original = image.getpixel((tx + column, ty + row))
                if authored[3] >= 128 and original[3] >= 128:
                    # Keep the selected chassis alpha so wheel/disk phases never
                    # change the train's silhouette, ground contact or couplers.
                    image.putpixel((tx + column, ty + row), (*authored[:3], original[3]))
    return image


def nearest_component_pose(subject, direction):
    """Retain source-pixel cue colors at the same registered native pose scale.

    @param subject Authored source part inside its explicit crop calibration.
    @param direction Actual engine view determining the approved safe envelope.
    @return RGBA native canvas with nearest-neighbor sampling for tiny cue selection.
    """
    maximum = ENVELOPES[direction]
    scale = min(maximum[0] / subject.width, maximum[1] / subject.height)
    width, height = max(1, round(subject.width * scale)), max(1, round(subject.height * scale))
    image = Image.new("RGBA", (64, 48))
    # Equipment bulbs occupy only a few native pixels. Their authored colors
    # must survive reduction without blending into the neighboring blue roof.
    image.paste(subject.resize((width, height), Image.Resampling.NEAREST), ((64 - width) // 2, 40 - height))
    return image


def derive(asset_ids=None, audit=None):
    """Convert declared authored poses without fabricating missing combinations.

    @param asset_ids Optional set of existing family IDs for a focused audit;
        None retains the complete M1 conversion and its published report shape.
    @param audit Optional list receiving every pose whose finished pixels exceed their envelope before any
        declared re-derivation (r2df task 5.1); when given, oversize poses are listed instead of refused.
    @return Native partial coverage manifest and exact output PNG bytes.
    @throws ValueError On altered original/reference bytes, unsupported poses or an oversize pose (task 5.2).
    """
    document = json.loads(RECORD.read_text(encoding="utf-8"))
    public = json.loads((base.PUBLIC).read_text(encoding="utf-8"))
    families = {row["asset_id"]: row for row in public["assets"]}
    grouped = {}
    cache = {}
    fixed_images = {}
    sources = {source["key"]: source for source in document["sources"]}
    selected_sources = document["sources"]
    selected_candidates = document["candidates"]
    if asset_ids is not None:
        # A family audit must refuse a typo or an empty selection rather than
        # report success without checking the requested locomotive's originals.
        requested = set(asset_ids)
        available = {source["asset_id"] for source in selected_sources}
        if not requested or not requested <= available:
            raise ValueError("focused vehicle conversion requires existing source families")
        selected_sources = [source for source in selected_sources if source["asset_id"] in requested]
        selected_candidates = [source for source in selected_candidates if source["asset_id"] in requested]
    coverage = set()
    # Rejects are provenance, never accepted input cells. Hash verification still
    # protects their retained bytes so later revision history stays auditable.
    for source in selected_sources + selected_candidates:
        if base.digest(base.repository_path(source["path"])) != source["sha256"]:
            raise ValueError("immutable vehicle source/candidate changed")
        for reference in source["references"]:
            if base.digest(base.repository_path(reference["path"])) != reference["sha256"]:
                raise ValueError("immutable vehicle reference changed")
    # Every selected cell carries its real part, heading, state and authored frame.
    # Coverage remains sparse until each required combination genuinely exists.
    for source in selected_sources:
        family = families[source["asset_id"]]
        if source["part"] not in family["parts"]:
            raise ValueError("vehicle source invents another physical part")
        if "poses" in source:
            mapping = source["poses"]
        else:
            if source["grid"] != [4, 2] or source["directions"] != DIRECTIONS:
                # A simple base-view sheet declares exactly eight engine views;
                # multiframe/partial sheets require explicit per-cell identities.
                raise ValueError("vehicle source lacks declared view mapping")
            mapping = [{"direction": direction, "state": source["state"], "frame": source["frame"],
                        "source_cell": [index % 4, index // 4]} for index, direction in enumerate(DIRECTIONS)]
        mapped = [tuple(pose["source_cell"]) for pose in mapping]
        excluded = [tuple(cell) for cell in source.get("excluded_cells", [])]
        columns, rows = source["grid"]
        all_cells = {(column, row) for row in range(rows) for column in range(columns)}
        if len(set(mapped + excluded)) != len(mapped + excluded) or set(mapped + excluded) != all_cells:
            # Every generated cell is selected once or explicitly excluded. A
            # missing last row must never masquerade as a full animation sheet.
            raise ValueError("ambiguous vehicle source cell coverage")
        cells, boxes = source_cells(source)
        poses = grouped.setdefault(source["asset_id"], [])
        for index, pose in enumerate(mapping):
            direction, state, frame = pose["direction"], pose["state"], pose["frame"]
            if direction not in DIRECTIONS or state not in family["states"] or frame not in range(family["frames"]):
                # The existing public model owns every heading/state/frame;
                # selected cells cannot add a new work phase or local identity.
                raise ValueError("vehicle pose disagrees with public model contract")
            identity = (source["asset_id"], source["part"], state, frame, direction)
            if identity in coverage:
                raise ValueError("duplicate vehicle pose coverage")
            coverage.add(identity)
            # Explicit selected per-view calibration removes other headings'
            # empty cell padding without changing original pixels. Later frames
            # must retain this registration; moving details cannot shift a chassis.
            crop = source["native_crop_boxes"][index]
            column, row = pose["source_cell"]
            subject = cells[row][column].crop(crop)
            pixels = native_pose(subject, source, direction, state)
            if "component_reference" in source:
                # Tiny authored equipment lights retain nearest source colors;
                # wheel/reel regions use the declared native sampling policy.
                authored_cue = nearest_component_pose(subject, direction) if source.get("component_sampling") == "nearest" or source["component_name"] in {"cab_beacon", "equipment_indicator"} else pixels
                pixels = assemble_component(authored_cue, source, index, direction, sources, fixed_images)
            # r2df D6: every finished pose fits its view's envelope so 8-unit neighbours do not overlap. Declared
            # chassis are re-derived after cue assembly; anything else oversize is refused, never clipped.
            excess = envelope_excess(pixels, direction)
            label = f"{source['asset_id']} {source['part']} {state} frame {frame} {direction}"
            regions = [list(target) for _origin, target in component_regions(source, index)] if "component_reference" in source else []
            target_box = source.get("component_target_boxes", [None] * len(mapping))[index]
            if excess is not None and audit is not None:
                audit.append({"pose": label, "source": source["key"], "opaque": list(excess),
                              "envelope": list(ENVELOPES[direction]), "rederived": rederives_envelope(source, sources)})
            if excess is not None and rederives_envelope(source, sources):
                if "component_reference" in source:
                    # The chassis decides the transform. Outside the scaled equipment regions the frame is exactly the
                    # scaled chassis, so a wheel, lamp or load cue still changes only its own registered pixels.
                    reference_state = source.get("component_reference_state", "idle")
                    if reference_state == "pose_state":
                        reference_state = state
                    chassis = fixed_images[(sources[source["component_reference"]]["key"], direction, reference_state)]
                    transform = envelope_transform(chassis, direction)
                    scaled = apply_envelope_transform(pixels, transform)
                    pixels = apply_envelope_transform(chassis, transform)
                    regions = [transform_region(region, transform) for region in regions]
                    # Each scaled equipment region takes the scaled frame's pixels; the rest stays chassis.
                    for x, y, width, height in regions:
                        pixels.paste(scaled.crop((x, y, x + width, y + height)), (x, y))
                    if target_box is not None:
                        target_box = transform_region(target_box, transform)
                else:
                    pixels = apply_envelope_transform(pixels, envelope_transform(pixels, direction))
                excess = envelope_excess(pixels, direction)
            if excess is not None and audit is None:
                limit = ENVELOPES[direction]
                raise ValueError(f"vehicle pose {label}: opaque {excess[0]}x{excess[1]} exceeds the {limit[0]}x{limit[1]} {direction} envelope; "
                                 "declare envelope_fit on its chassis source to re-derive it")
            poses.append({"part": source["part"], "state": state, "frame": frame,
                          "direction": direction, "source": source["key"], "source_path": source["path"],
                          "source_box": boxes[row * columns + column], "anchor": family["anchor"],
                          "component": source.get("component_name"),
                          "component_target_box": target_box,
                          "component_regions": regions,
                          "reference_state": state if source.get("component_reference_state") == "pose_state" else source.get("component_reference_state", "idle"),
                          "pixels": base.quantize(pixels, cache)})
    outputs, files = [], {}
    # Stable sorting produces an exact sparse atlas; there is no padded pose
    # pretending that idle frame zero satisfies working/service animation.
    for identity, poses in sorted(grouped.items()):
        family = families[identity]
        poses.sort(key=lambda row: (family["parts"].index(row["part"]), family["states"].index(row["state"]), row["frame"], DIRECTIONS.index(row["direction"])))
        sheet = Image.new("P", (8 * 64, len(poses) // 8 * 48), 0)
        sheet.putpalette(base.PALETTE_BYTES)
        sprites = []
        for index, pose in enumerate(poses):
            x, y = index % 8 * 64, index // 8 * 48
            sheet.paste(pose.pop("pixels"), (x, y))
            sprites.append({**pose, "box": [x, y, 64, 48]})
        path = base.OUTPUT / (identity.replace(".", "-") + "-authored.png")
        encoded = png_bytes(sheet)
        files[path] = encoded
        required = {(part, state, frame, direction) for part in family["parts"]
                    for state in family["states"] for frame in range(family["frames"]) for direction in DIRECTIONS}
        present = {(row["part"], row["state"], row["frame"], row["direction"]) for row in sprites}
        missing = sorted(required - present)
        outputs.append({"asset_id": identity, "path": path.relative_to(base.ROOT).as_posix(),
                        "sha256": hashlib.sha256(encoded).hexdigest(), "size": list(sheet.size),
                        "native_cell": [64, 48], "frames_per_state": family["frames"], "coverage_complete": not missing,
                        "required_pose_count": len(required), "missing_pose_count": len(missing),
                        "missing": [list(row) for row in missing], "sprites": sprites})
    report = {"_documentation": "Owner: r2mlvs13-graphics-and-asset-pipeline.md Groups 14-18; sparse source coverage only",
              "schema_version": 1, "source_record": RECORD.relative_to(base.ROOT).as_posix(),
              "runtime_activated": False, "atlases": outputs,
              "pose_count": sum(len(row["sprites"]) for row in outputs),
              "occupied_envelopes": {name: list(size) for name, size in ENVELOPES.items()}}
    return report, files


def publish(report, files):
    """Write partial atlases and a labeled native/enlarged source review.

    @param report Exact selected physical part/state/frame coverage.
    @param files Derived repository atlas paths and their encoded bytes.
    @return None; publish or verify the declared repository-owned source artifacts.
    """
    base.REVIEW.mkdir(parents=True, exist_ok=True)
    base.OUTPUT.mkdir(parents=True, exist_ok=True)
    for path, encoded in files.items():
        path.write_bytes(encoded)
    REPORT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    bind_manifest(report)
    height = sum(row["size"][1] * 3 + 64 for row in report["atlases"]) + 12
    review = Image.new("RGB", (1048, height), (44, 47, 51))
    draw = ImageDraw.Draw(review)
    y = 12
    # Actual native size exposes lost shape/detail; nearest-neighbor enlargement
    # keeps every source pixel visible without implying actual viewport loading.
    for row in report["atlases"]:
        image = Image.open(base.ROOT / row["path"]).convert("RGBA")
        draw.text((12, y), row["asset_id"] + f": {len(row['sprites'])}/{row['required_pose_count']} selected poses; directions N NE E SE S SW W NW", fill="white")
        review.paste(image, (12, y + 24), image)
        large = image.resize((image.width * 2, image.height * 2), Image.Resampling.NEAREST)
        review.paste(large, (12, y + image.height + 36), large)
        y += row["size"][1] * 3 + 64
    review.save(REVIEW)
    publish_animation_reviews(report)


def publish_animation_reviews(report):
    """Expose complete authored direction/frame sets as native and enlarged GIFs.

    @param report Selected sparse model coverage; incomplete states are excluded.
    @return None; write source-only reviews, never runtime screenshots or sprites.
    """
    # Review only actually complete part/state sets. A missing support phase
    # cannot be hidden by animating the cutter or borrowing another part's pixels.
    for model in report["atlases"]:
        frame_count = model["frames_per_state"]
        atlas = Image.open(base.ROOT / model["path"]).convert("RGBA")
        poses = {(row["part"], row["state"], row["frame"], row["direction"]): row for row in model["sprites"]}
        sets = sorted({(row["part"], row["state"]) for row in model["sprites"]})
        for part, state in sets:
            if not all((part, state, frame, direction) in poses for frame in range(frame_count) for direction in DIRECTIONS):
                continue
            frames = []
            # Native poses reveal lost animation pixels; nearest enlargement
            # exposes registration jitter and heading continuity without implying
            # that the current game already loads these feature sprites.
            for frame in range(frame_count):
                canvas = Image.new("RGB", (1048, 560), (44, 47, 51))
                draw = ImageDraw.Draw(canvas)
                draw.text((12, 12), f"SOURCE ONLY: {model['asset_id']} {part} {state} frame {frame}; native 1x / enlarged 4x; runtime pending", fill="white")
                for index, direction in enumerate(DIRECTIONS):
                    row = poses[(part, state, frame, direction)]
                    x, y, width, height = row["box"]
                    sprite = atlas.crop((x, y, x + width, y + height))
                    left, top = 12 + index % 4 * 256, 40 + index // 4 * 250
                    draw.text((left, top), direction, fill="white")
                    canvas.paste(sprite, (left + 80, top + 16), sprite)
                    large = sprite.resize((256, 192), Image.Resampling.NEAREST)
                    canvas.paste(large, (left, top + 58), large)
                frames.append(canvas)
            name = model["asset_id"].replace(".", "-") + f"-{part}-{state}-animation-review.gif"
            frames[0].save(base.REVIEW / name, save_all=True, append_images=frames[1:], duration=240, loop=0, disposal=2)


def bind_manifest(report):
    """Bind exact partial source coverage while retaining original provenance.

    @param report Declared model poses and explicit missing combinations from derive.
    @return None; publish or verify the declared repository-owned source artifacts.
    """
    path = base.PUBLIC
    manifest = json.loads(path.read_text(encoding="utf-8"))
    sources = json.loads(RECORD.read_text(encoding="utf-8"))["sources"]
    # A model's selected chassis, separate support and authored components share
    # one public art family. Preserve previous originals and packed ownership;
    # complete idle cues cannot claim the still-missing moving/work/service art.
    for output in report["atlases"]:
        row = next(row for row in manifest["assets"] if row["asset_id"] == output["asset_id"])
        selected = [source for source in sources if source["asset_id"] == output["asset_id"]]
        original = next(source for source in selected if source["part"] == "body" and source.get("frame") == 0)
        original_paths = list(dict.fromkeys(row.get("authoring", {}).get("originals", []) + [source["path"] for source in selected]))
        original_identity = original.get("original_source", original)
        row["source"] = {"status": "authored" if output["coverage_complete"] else "authored_partial",
                         "path": original_identity["path"], "sha256": original_identity["sha256"],
                         "provenance": "first_party_original_builtin_image_gen", "rights": "first_party_generated_original"}
        row["authoring"] = {"date": "2026-10-07", "status": "complete_registered_source_coverage_runtime_pending" if output["coverage_complete"] else "partial_registered_source_coverage_runtime_pending",
                            "source_record": report["source_record"], "originals": original_paths,
                            "atlas_manifest": REPORT.relative_to(base.ROOT).as_posix(),
                            "native_atlas": output["path"], "native_sha256": output["sha256"],
                            "staged_pose_count": len(output["sprites"]), "required_pose_count": output["required_pose_count"],
                            "missing_pose_count": output["missing_pose_count"], "runtime_activated": False,
                            "coverage_complete": output["coverage_complete"]}
    path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def check(expected, files, asset_ids=None):
    """Verify reproducibility, reserved colors, source sparsity and transparency.

    @param expected Coverage derived freshly from immutable selected originals.
    @param files Expected encoded atlas bytes regenerated without writes.
    @param asset_ids Optional family selection used by derive; compare only that
        slice of the existing report without publishing a partial replacement.
    @throws ValueError If partial coverage or native pixels disagree with records.
    @return None; publish or verify the declared repository-owned source artifacts.
    """
    published = json.loads(REPORT.read_text(encoding="utf-8"))
    if asset_ids is not None:
        # A focused check consumes the same shared report, preserving unrelated
        # families and preventing a scoped run from replacing the public atlas.
        requested = set(asset_ids)
        actual = {row["asset_id"] for row in expected["atlases"]}
        if not requested or actual != requested:
            raise ValueError("focused vehicle check lost its requested families")
        published["atlases"] = [row for row in published["atlases"] if row["asset_id"] in requested]
        published["pose_count"] = sum(len(row["sprites"]) for row in published["atlases"])
    if published != expected:
        raise ValueError("vehicle coverage manifest disagrees with originals")
    # Each model must regenerate exactly from its selected cells while retaining
    # honest missing-pose counts; rejection history cannot silently fill a gap.
    for row in expected["atlases"]:
        path = base.repository_path(row["path"])
        if path.read_bytes() != files[path]:
            raise ValueError("vehicle conversion is not reproducible")
        image = Image.open(path)
        if image.mode != "P" or image.info.get("transparency") != 0 or image.getpalette() != base.PALETTE_BYTES:
            # The train uses the same DOS alpha/palette semantics as the supplied
            # profile; reserved indices cannot become animation or livery effects.
            raise ValueError("vehicle palette/transparency mismatch")
        if set(image.get_flattened_data()) & (base.FORBIDDEN - {0}):
            raise ValueError("vehicle uses reserved palette indices")
        if len(row["sprites"]) + row["missing_pose_count"] != row["required_pose_count"]:
            raise ValueError("vehicle sparse coverage is falsely complete")
        native = {}
        # Pose keys keep actual part/state/frame/view ownership explicit; a
        # selected bulb frame cannot stand in for another physical train part.
        for pose in row["sprites"]:
            x, y, width, height = pose["box"]
            sprite = image.crop((x, y, x + width, y + height))
            if not any(sprite.get_flattened_data()):
                raise ValueError("empty selected vehicle pose")
            if any(sprite.getpixel(point) for point in [(0, 0), (63, 0), (0, 47), (63, 47)]):
                raise ValueError("opaque vehicle canvas corner")
            native[(pose["part"], pose["state"], pose["frame"], pose["direction"])] = sprite
        # Each declared moving/status component changes only its own equipment.
        # Compare against the selected idle chassis independently of source crop
        # calibration, keeping heading and coupler geometry stable through motion.
        for pose in row["sprites"]:
            if pose["component"] is None:
                continue
            current = native[(pose["part"], pose["state"], pose["frame"], pose["direction"])]
            original = native[(pose["part"], pose["reference_state"], 0, pose["direction"])]
            regions = pose["component_regions"]
            if pose["component"] in {"cab_beacon", "equipment_indicator"} and current.tobytes() == original.tobytes():
                # A lamp is visible in every heading; an unchanged authored lamp
                # means native conversion lost the declared status animation.
                raise ValueError("authored animation cue disappeared at native palette scale")
            # End-on wheels and away-facing cutter teeth may be hidden. Real
            # authored frames can therefore match; source provenance still must
            # include every frame, rather than padding missing poses with idle.
            for yy in range(48):
                for xx in range(64):
                    before, after = original.getpixel((xx, yy)), current.getpixel((xx, yy))
                    if (before == 0) != (after == 0):
                        raise ValueError("status animation changes chassis alpha/contacts")
                    if not any(x <= xx < x + width and y <= yy < y + height for x, y, width, height in regions) and before != after:
                        # Selected wheel/disk/lamp pixels never authorize a new
                        # cab, shell, coupler or unregistered machinery change.
                        raise ValueError("vehicle frame changes pixels outside authored equipment")
        for direction in DIRECTIONS:
            idle = [native[("body", "idle", frame, direction)] for frame in range(4)
                    if ("body", "idle", frame, direction) in native]
            if len(idle) == 4 and len({image.tobytes() for image in idle}) != 4:
                # Four supplied idle cue phases must survive the native palette;
                # duplicated frame-zero pixels cannot conceal an unfilled frame.
                raise ValueError("idle authored cue phases collapse into duplicate pixels")
    public = json.loads((base.PUBLIC).read_text(encoding="utf-8"))
    families = {row["asset_id"]: row for row in public["assets"]}
    # The public source handoff must describe exactly these baked poses. A stale
    # source count/hash can mislead packing into accepting a partial model or a
    # different atlas even when local source conversion is reproducible.
    for atlas in expected["atlases"]:
        family = families[atlas["asset_id"]]
        authoring = family["authoring"]
        if authoring["native_sha256"] != atlas["sha256"] or authoring["staged_pose_count"] != len(atlas["sprites"]) or authoring["missing_pose_count"] != atlas["missing_pose_count"] or authoring["coverage_complete"] != atlas["coverage_complete"]:
            raise ValueError("public vehicle source handoff is stale: " + atlas["asset_id"])
    print(f"Vehicle source conversion: {len(expected['atlases'])} models / {expected['pose_count']} authored poses; exact regeneration, palette, alpha and sparse coverage pass; source-only audit")


def main():
    """Publish or verify the bounded selected rolling-stock source handoff.

    @return None; fail on invalid retained sources or derived partial coverage.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="verify without writing")
    parser.add_argument("--envelopes", action="store_true", help="list poses exceeding their view envelope before re-derivation; writes nothing (r2df task 5.1)")
    args = parser.parse_args()
    if args.envelopes:
        audit = []
        derive(audit=audit)
        summary = {}
        # One row per family/part/view/state: the widest and tallest opaque pose among its frames.
        for row in audit:
            asset, part, state, _frame, _number, direction = row["pose"].split(" ")
            key = (asset, part, direction, state, row["rederived"])
            width, height = summary.get(key, (0, 0))
            summary[key] = (max(width, row["opaque"][0]), max(height, row["opaque"][1]))
        for (asset, part, direction, state, rederived), (width, height) in sorted(summary.items()):
            limit = ENVELOPES[direction]
            print(f"{asset} {part} {direction} {state}: {width}x{height} > {limit[0]}x{limit[1]}{' (re-derived)' if rederived else ''}")
        print(f"{len(audit)} oversize poses in {len(summary)} family/part/view/state groups")
        return
    report, files = derive()
    if not args.check:
        publish(report, files)
    check(report, files)


if __name__ == "__main__":
    main()
