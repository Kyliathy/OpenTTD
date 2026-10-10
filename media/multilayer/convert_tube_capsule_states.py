"""Assemble capsule source from original hulls, cargo, lamps and availability art.

@file convert_tube_capsule_states.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups49-53.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native cargo/service/power owners remain authoritative; source creates no simulation state.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_control_art as controls
import convert_facility_art as facilities
import convert_tube_capsules as models


tube = models.tube
DIRECTIONS = models.vehicles.DIRECTIONS
STATES = ["empty", "partial", "full", "service-empty", "service-partial", "service-full",
          "blocked-empty", "blocked-partial", "blocked-full"]


def component_signature(components):
    """Identify the publicly approved physical component registrations.

    @param components Original cargo, glass, company-mask and UI source contract.
    @return SHA256 of stable JSON, preventing unreviewed body-paint registrations.
    """
    return hashlib.sha256(json.dumps(components, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def source_names(spec):
    """Resolve safe model-owned proof names from the retained source family.

    @param spec Canonical selected capsule family, including its display name.
    @return Display model name and plain lowercase artifact stem.
    """
    name = spec["name"].split()[0]
    stem = name.split("-")[0].lower()
    # A model display name may name proof files only inside the fixed review
    # directory; punctuation/path segments cannot redirect generated output.
    if not stem.isascii() or not stem.isalpha():
        raise ValueError("invalid native capsule source name")
    return name, stem


def family_spec(document, asset_id="vehicle.model-09"):
    """Require complete independent cargo/mode states and approved original components.

    @param document Shared retained source record, optionally isolated for refusal checks.
    @param asset_id Explicit public capsule body family; defaults to retained Mole proof.
    @return Guarded model specification and its public source family.
    """
    selected = [row for row in document["families"] if row["asset_id"] == asset_id]
    if len(selected) != 1:
        raise ValueError("missing or duplicate original capsule source")
    spec = selected[0]
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == spec["asset_id"])
    # Only actual public tube designs may use this native state assembler.
    # A copied canvas or plausible display name cannot turn ordinary rolling
    # stock into a capsule or bind another model's purchase presentation.
    if (family["consumer"] != 17 or family["kind"] != "vehicle"
            or asset_id != f"vehicle.model-{spec['model_id']:02}"):
        raise ValueError("foreign native capsule source family")
    state_map = {state: {"load": ["empty", "partial", "full"][index % 3],
                         "mode": ["operating", "service", "blocked"][index // 3]} for index, state in enumerate(STATES)}
    # Cargo and service/power condition coexist in actual native vehicles.
    # Missing combinations cannot erase the load or inherit an empty pose.
    if (spec["states"] != STATES or spec["state_map"] != state_map or spec["directions"] != DIRECTIONS
            or spec["frames"] != 2 or spec["parts"] != ["body"] or spec["native_size"] != [64, 48]
            or spec["anchor"] != [-32, -32] or spec["runtime_activated"] is not False):
        raise ValueError("native capsule cargo/mode source contract mismatch")
    if any(family[key] != spec[key] for key in ("states", "directions", "frames", "parts", "native_size", "anchor")):
        raise ValueError("native capsule public geometry mismatch")
    components = spec["components"]
    if component_signature(components) != family["geometry_notes"]["approved_components_sha256"]:
        raise ValueError("unapproved native capsule component registration")
    for key in ("hold_masks", "cue_targets"):
        if set(components[key]) != set(DIRECTIONS):
            raise ValueError("missing original capsule component heading")
    heavy = components.get("cargo_presentation") == "heavy_modules"
    # Only the actual open heavy frame may gain load alpha. Closed hoppers,
    # package caddies and sealed tanks/casks retain their contained-load rule;
    # a copied flag cannot authorize holes or new cargo outside their hull.
    if heavy or components.get("allow_cargo_alpha_extension") or components.get("skip_transparent_cargo"):
        if (not heavy or asset_id != "vehicle.model-13"
                or components.get("allow_cargo_alpha_extension") is not True
                or components.get("skip_transparent_cargo") is not True
                or spec["heavy_load_presentation"]["requires_authorized_load_alpha_extension"] is not True
                or set(components["cargo_target_boxes"]) != set(DIRECTIONS)
                or set(components["cargo_placement_order"]) != set(DIRECTIONS)):
            raise ValueError("foreign or incomplete heavy load alpha contract")
        # Each fixed module projection keeps its own full box even where a
        # foreground brace occludes it. Mask clipping must never rescale the
        # module, move its ground contact or reverse actual near/far ordering.
        for direction in DIRECTIONS:
            boxes, masks = components["cargo_target_boxes"][direction], components["hold_masks"][direction]
            order = components["cargo_placement_order"][direction]
            if len(boxes) != 2 or len(masks) != 2 or len(order) != 2 or set(order) != {0, 1}:
                raise ValueError("incomplete heavy module placement")
            for box, mask in zip(boxes, masks):
                left, top, width, height = box
                if (width <= 0 or height <= 0 or left < 0 or top < 0 or left + width > 64 or top + height > 48
                        or not mask or any(not (left <= x < left + width and top <= y < top + height) for x, y in mask)):
                    raise ValueError("heavy module leaves its approved native volume")
    if components.get("cargo_presentation") == "sealed_roof_meter":
        # The sealed mineral cask exposes two genuine roof-glass fittings,
        # not loose cargo. Each approved meter cell remains independent so
        # partial/full cannot collapse or reveal contents through the armour.
        if (asset_id != "vehicle.model-12"
                or any(len(masks) != 2 or any(len(mask) != 1 for mask in masks)
                       or masks[0] == masks[1] for masks in components["hold_masks"].values())):
            raise ValueError("native sealed mineral roof-meter mismatch")
    if components["cargo_key"] == "cistern-level-glass-v1":
        visible = components["cargo_visible_directions"]
        hidden = components["cargo_occluded_directions"]
        presentation = spec["fluid_level_presentation"]
        # Opaque pressure tanks hide their contents. A valid source must name
        # exactly which original views expose the actual sight glass; hidden
        # views cannot acquire painted windows to manufacture load animation.
        if (visible != presentation["visible_original_gauge_directions"]
                or hidden != presentation["occluded_original_gauge_directions"]
                or set(visible) & set(hidden) or set(visible) | set(hidden) != set(DIRECTIONS)
                or any(not mask for d in visible for mask in components["hold_masks"][d])
                or any(components["hold_masks"][d] != [[], []] for d in hidden)):
            raise ValueError("native sealed-fluid visibility mismatch")
    return spec, family


def package_donors(components, document=None):
    """Decode actual sealed package artwork without importing loose bulk material.

    @param components Approved same-view original lid/strap crop registrations.
    @param document Optional isolated source record for malformed-input checks.
    @return Immutable package original and indexed partial/full donors by heading.
    """
    # Packaged goods have their own original component owner. A Rock wagon,
    # changed heading or transparent border cannot masquerade as a sealed lid.
    if (components["cargo_record"] != tube.RECORD.relative_to(tube.ROOT).as_posix()
            or components["cargo_key"] != "crate-packages-v1"
            or set(components["cargo_source_crops"]) != set(DIRECTIONS)):
        raise ValueError("foreign sealed capsule package donor")
    source = tube.retained_candidate(components["cargo_key"], document)
    if (source["asset_id"] != "vehicle.model-10" or source["component"] != "sealed_package_load"
            or source["status"] != "selected" or source["component_views"] != components["cargo_source_crops"]):
        raise ValueError("unapproved original capsule package views")
    original = Image.open(tube.base.repository_path(source["path"])).convert("RGBA")
    result = {}
    # Each actual view supplies its opaque sealed lid and safety strap only.
    # Native pocket masks preserve the original green posts/rails and shadow;
    # partial and full differ by one versus two physically occupied pockets.
    for direction, box in components["cargo_source_crops"].items():
        left, top, right, bottom = box
        if not (0 <= left < right <= original.width and 0 <= top < bottom <= original.height):
            raise ValueError("sealed package crop leaves original")
        material = original.crop(box)
        if material.getchannel("A").getextrema()[0] < 128:
            raise ValueError("sealed package material includes transparent source")
        pixels = tube.base.quantize(material.resize((64, 48), Image.Resampling.BOX), {})
        for load in ("partial", "full"):
            result[load, direction] = pixels
    return source, result


def liquid_donors(components, document=None):
    """Decode original fluid material for the actual sealed tank sight glass.

    @param components Approved empty/half/full original glass/material registrations.
    @param document Optional isolated provenance for source/refusal checks.
    @return Immutable gauge original and indexed partial/full material donors.
    """
    # A small glass component contributes liquid material, never a vehicle
    # body, loose cargo or fluid stock. Crops identify the genuinely filled
    # part of its authored half/full presentation and preserve original colour.
    if (components["cargo_record"] != tube.RECORD.relative_to(tube.ROOT).as_posix()
            or components["cargo_key"] != "cistern-level-glass-v1"
            or set(components["cargo_source_crops"]) != {"empty", "partial", "full"}):
        raise ValueError("foreign sealed capsule fluid donor")
    source = tube.retained_candidate(components["cargo_key"], document)
    if (source["asset_id"] != "vehicle.model-11" or source["component"] != "sealed_fluid_level_glass"
            or source["status"] != "selected" or source["component_states"] != components["cargo_source_crops"]):
        raise ValueError("unapproved original capsule fluid levels")
    original = Image.open(tube.base.repository_path(source["path"])).convert("RGBA")
    result = {}
    # The same front-on glass material is projected only inside an approved
    # existing fitting. The target masks express half/full visible height;
    # directions that hide this fitting retain the sealed original tank.
    for load in ("partial", "full"):
        box = components["cargo_source_crops"][load]
        left, top, right, bottom = box
        if not (0 <= left < right <= original.width and 0 <= top < bottom <= original.height):
            raise ValueError("fluid glass crop leaves original")
        material = original.crop(box)
        if material.getchannel("A").getextrema()[0] < 128:
            raise ValueError("fluid material includes transparent source")
        pixels = tube.base.quantize(material.resize((64, 48), Image.Resampling.BOX), {})
        for direction in DIRECTIONS:
            result[load, direction] = pixels
    return source, result


def sealed_meter_donors(components):
    """Reuse genuine M1 amber glass in the actual sealed-mineral roof meter.

    @param components Approved original glass identity, fixed phase and roof targets.
    @return Original component provenance and indexed material donors by load/view.
    """
    # A visual cargo bucket lights original fittings while the opaque mineral
    # cask remains sealed. The existing glass owner supplies actual colour;
    # no recolouring, loose Rock, invented cargo packet or extra stock is used.
    if (components["cargo_record"] != facilities.RECORD.relative_to(tube.ROOT).as_posix()
            or components["cargo_key"] != "facility-status"
            or components["cargo_meter_state"] != "construction"
            or components["cargo_meter_frame"] != 0):
        raise ValueError("foreign sealed mineral meter donor")
    source = next(row for row in json.loads(facilities.RECORD.read_text(encoding="utf-8"))["components"] if row["key"] == "facility-status")
    glass = facilities.indicator(source)[facilities.STATES.index("construction")][0]
    # Reduce the genuinely authored glass to its tiny native fitting, then
    # repeat that original sample for the existing generic crop/mask assembler.
    # One/two independent original roof targets express partial/full cargo.
    native = glass.resize((1, 1), Image.Resampling.BOX).resize((64, 48), Image.Resampling.NEAREST)
    donor = tube.base.quantize(native, {})
    return dict(source, component="sealed_roof_load_meter"), {(load, direction): donor for load in ("partial", "full") for direction in DIRECTIONS}


def heavy_donors(components, document=None):
    """Decode isolated original heavy modules with their genuine three-dimensional alpha.

    @param components Approved same-view whole-module crops and fixed target projections.
    @param document Optional isolated provenance for original/source refusal checks.
    @return Immutable original and indexed partial/full module donors by actual view.
    """
    # A solid original payload may fill an open frame. Loose Rock, a borrowed
    # wagon body or a cropped neighbouring source cell cannot supply it.
    if (components["cargo_record"] != tube.RECORD.relative_to(tube.ROOT).as_posix()
            or components["cargo_key"] != "forge-heavy-modules-v1"
            or set(components["cargo_source_crops"]) != set(DIRECTIONS)):
        raise ValueError("foreign original heavy module donor")
    source = tube.retained_candidate(components["cargo_key"], document)
    if (source["asset_id"] != "vehicle.model-13" or source["component"] != "heavy_pallet_load"
            or source["status"] != "selected" or source["component_views"] != components["cargo_source_crops"]
            or len(source["actual_component_order"]) != 8 or set(source["actual_component_order"]) != set(DIRECTIONS)):
        raise ValueError("unapproved original heavy module views")
    cells, boxes = tube.base.source_cells(source)
    original = Image.open(tube.base.repository_path(source["path"])).convert("RGBA")
    result = {}
    # Generated subjects can cross nominal equal-grid boundaries. Isolate the
    # actual opacity bands and require the complete original subject crop;
    # a neighbour's pixels or a missing lifting lug cannot inherit approval.
    for index, direction in enumerate(source["actual_component_order"]):
        cell = cells[index // 4][index % 4]
        bounds = cell.getchannel("A").point(lambda value: 255 if value >= 128 else 0).getbbox()
        left, top, unused_right, unused_bottom = boxes[index]
        expected = [left + bounds[0], top + bounds[1], left + bounds[2], top + bounds[3]]
        crop = components["cargo_source_crops"][direction]
        if crop != expected:
            raise ValueError("heavy module crop crosses or truncates its actual source cell")
        material = original.crop(crop).resize((64, 48), Image.Resampling.BOX)
        donor = tube.base.quantize(material, {})
        for load in ("partial", "full"):
            result[load, direction] = donor
    return source, result


def cargo_donors(components, document=None):
    """Decode the approved physical cargo form from its actual original owner.

    @param components Approved cargo material, level glass or sealed load-meter registrations.
    @param document Optional shared source record for model-owned packaged components.
    @return Original provenance and indexed partial/full donors by actual direction.
    """
    if components.get("cargo_presentation") == "heavy_modules":
        return heavy_donors(components, document)
    if components.get("cargo_presentation") == "sealed_roof_meter":
        return sealed_meter_donors(components)
    if components["cargo_key"] == "crate-packages-v1":
        return package_donors(components, document)
    if components["cargo_key"] == "cistern-level-glass-v1":
        return liquid_donors(components, document)
    record = tube.base.ASSETS / "generation-record.json"
    # Cargo-only reuse comes from the actual selected Rock original. A different
    # wagon, private cache or painted texture cannot silently supply missing art.
    if components["cargo_record"] != record.relative_to(tube.ROOT).as_posix() or components["cargo_key"] != "rock":
        raise ValueError("foreign original capsule cargo donor")
    source = next(row for row in json.loads(record.read_text(encoding="utf-8"))["sources"] if row["key"] == "rock")
    for item in [source] + source["references"]:
        if tube.base.digest(tube.base.repository_path(item["path"])) != item["sha256"]:
            raise ValueError("immutable capsule cargo source/reference changed")
    cells, boxes = tube.base.source_cells(source)
    result = {}
    # The original partial/full rows and true headings supply cargo material
    # only. No donor wagon chassis, wheels or private cargo stock is copied.
    for row, state in ((1, "partial"), (2, "full")):
        for column, direction in enumerate(DIRECTIONS):
            pixels = tube.base.quantize(tube.base.fit(cells[row][column], (64, 48), (48, 32)), {})
            result[state, direction] = pixels
    return source, result


def status_lamps(components):
    """Reduce genuine original lamp phases without procedural recolouring.

    @param components Canonical M1 glass identity and actual two-phase/state selection.
    @return Original provenance and native glass palette indices by mode/frame.
    """
    # Ready/service/blocked use their original green/amber/red artwork. Exact
    # phase selection prevents one static panel being counted as two animations.
    if (components["indicator_record"] != facilities.RECORD.relative_to(tube.ROOT).as_posix()
            or components["indicator_key"] != "facility-status" or components["indicator_frames"] != [0, 2]
            or components["indicator_state_map"] != {"operating": "ready", "service": "construction", "blocked": "blocked"}):
        raise ValueError("native capsule original lamp contract mismatch")
    source = next(row for row in json.loads(facilities.RECORD.read_text(encoding="utf-8"))["components"] if row["key"] == "facility-status")
    phases = facilities.indicator(source)
    result = {}
    for mode, state in components["indicator_state_map"].items():
        indices = []
        for phase in components["indicator_frames"]:
            glass = phases[facilities.STATES.index(state)][phase].resize((1, 1), Image.Resampling.BOX)
            indices.append(tube.base.quantize(glass, {}).getpixel((0, 0)))
        if len(set(indices)) != 2:
            raise ValueError("native capsule lamp phases collapse at native size")
        result[mode] = indices
    return source, result


def company_plaque(candidate, direction, transform, pixels, source_box=None):
    """Preserve the original blue company fitting through explicit native palette remap.

    @param candidate Immutable own-model source containing the real side plaque.
    @param direction Actual view; end views naturally hide the side fitting.
    @param transform Exact source crop/resampling/registration used for this hull.
    @param pixels Indexed own-model hull used to refuse transparency insertion.
    @param source_box Optional approved original plaque box, excluding similarly coloured hull shadows.
    @return Original plaque bounds/RGB, native target and genuine company shade, or None.
    """
    if direction in ("N", "S"):
        return None
    left, top, right, bottom = transform["native_crop"]
    original = Image.open(tube.base.repository_path(candidate["path"])).convert("RGBA").crop((left, top, right, bottom))
    plaque_box = source_box or [left, top, right, bottom]
    # Cyan pressure-tank shadows can also be strongly blue. An explicit
    # original fitting crop keeps company remap off those fixed materials;
    # it must stay inside this exact registered model view.
    if not (left <= plaque_box[0] < plaque_box[2] <= right and top <= plaque_box[1] < plaque_box[3] <= bottom):
        raise ValueError("original capsule company crop leaves its view")
    points = []
    # Strong original blue selects the small authored plaque, excluding amber
    # hull, neutral ore pockets and pale headlights. Palette remap is confined
    # to this fitting; it cannot tint cargo or manufacture a status light.
    for index, colour in enumerate(original.get_flattened_data()):
        red, green, blue, alpha = colour
        px, py = index % original.width + left, index // original.width + top
        if not (plaque_box[0] <= px < plaque_box[2] and plaque_box[1] <= py < plaque_box[3]):
            continue
        if alpha >= 128 and blue >= 70 and blue > 2 * red and blue > 1.3 * green:
            points.append((index % original.width, index // original.width, colour))
    if not points:
        raise ValueError("missing original capsule company plaque")
    bounds = [min(p[0] for p in points), min(p[1] for p in points), max(p[0] for p in points) + 1, max(p[1] for p in points) + 1]
    if bounds[2] - bounds[0] > original.width / 4 or bounds[3] - bounds[1] > original.height / 3:
        # Multiple unrelated blue regions cannot impersonate one small fitting.
        raise ValueError("original capsule company mask expands beyond its plaque")
    subject = transform["native_subject_size"]
    if subject is None:
        scale = min(transform["native_maximum"][0] / original.width, transform["native_maximum"][1] / original.height)
        subject = [max(1, round(original.width * scale)), max(1, round(original.height * scale))]
    dx, dy = transform["native_translation"]
    x = (64 - subject[0]) // 2 + dx + int(sum(p[0] for p in points) / len(points) * subject[0] / original.width)
    y = 40 - subject[1] + dy + int(sum(p[1] for p in points) / len(points) * subject[1] / original.height)
    if pixels.getpixel((x, y)) == 0:
        raise ValueError("original capsule company plaque leaves hull alpha")
    rgb = [round(sum(p[2][channel] for p in points) / len(points)) for channel in range(3)]
    shade = min(range(198, 206), key=lambda value: sum((tube.base.PALETTE[value][channel] - rgb[channel]) ** 2 for channel in range(3)))
    return {"source_box": [bounds[0] + left, bounds[1] + top, bounds[2] + left, bounds[3] + top],
            "original_mean_rgb": rgb, "target": [x, y], "palette_index": shade}


def purchase_source(spec, native_models):
    """Reuse the own-model empty preview and genuine calendar-lock artwork.

    @param spec Guarded actual capsule source specification.
    @param native_models Actual selected eight-direction body models and provenance.
    @return Two-pose purchase source atlas report and indexed bytes.
    """
    components = spec["components"]
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    name, stem = source_names(spec)
    purchase_id = f"purchase.model-{spec['model_id']:02}"
    family = next(row for row in public["assets"] if row["asset_id"] == purchase_id)
    # Purchase controls keep their native UI geometry and calendar presentation;
    # neither a world-body canvas nor another state can substitute for this icon.
    if (family["native_size"] != [20, 20] or family["anchor"] != [0, 0] or family["states"] != ["available", "locked"]
            or family["frames"] != 1 or family["directions"] != ["none"]):
        raise ValueError("native capsule purchase public geometry mismatch")
    if (components["purchase_lock_record"] != controls.RECORD.relative_to(tube.ROOT).as_posix()
            or components["purchase_lock_family"] != "control.future-lock"):
        # Availability is an existing UI owner; world vehicle pixels never
        # acquire a painted padlock or a second introduction-date rule.
        raise ValueError("foreign original capsule purchase lock")
    report, files = controls.derive()
    lock = next(row for row in report["atlases"] if row["asset_id"] == "control.future-lock")
    with Image.open(io.BytesIO(files[tube.base.repository_path(lock["path"])])) as atlas:
        glyph = atlas.crop((0, 0, 20, 20)).convert("RGBA")
        badge = glyph.crop(glyph.getchannel("A").getbbox()).resize((7, 7), Image.Resampling.BOX)
    model, original, transform = native_models["E"]
    lock_original = json.loads(controls.RECORD.read_text(encoding="utf-8"))["source"]
    lock_provenance = {"path": lock_original["path"], "sha256": lock_original["sha256"],
                       "record": controls.RECORD.relative_to(tube.ROOT).as_posix(),
                       "family": "control.future-lock", "native_selection": lock["sprites"][0], "target": [12, 12, 7, 7]}
    image = model.convert("RGBA")
    # The model-owned preview must fit above its genuine calendar badge.
    # A taller tank can choose a smaller uniform fit without clipping or
    # changing the native world chassis; existing models keep their exact fit.
    maximum = components.get("purchase_maximum", [18, 14])
    if len(maximum) != 2 or any(value <= 0 or value > bound for value, bound in zip(maximum, (18, 14))):
        raise ValueError("native capsule purchase preview leaves UI canvas")
    available = tube.base.fit(image.crop(image.getchannel("A").getbbox()), (20, 20), maximum)
    locked = available.copy()
    # Taller own-model previews need the badge one row lower so the calendar
    # presentation preserves every opaque body pixel in the purchase button.
    # The approved component registration owns placement; Mole keeps its
    # original default and continues to regenerate byte for byte.
    lock_target = components.get("purchase_lock_target", [12, 12])
    if len(lock_target) != 2 or any(value < 0 or value + 7 > 20 for value in lock_target):
        raise ValueError("native capsule purchase badge leaves UI canvas")
    lock_provenance["target"] = list(lock_target) + [7, 7]
    locked.alpha_composite(badge, tuple(lock_target))
    atlas = Image.new("P", (40, 20), 0)
    atlas.putpalette(tube.base.PALETTE_BYTES)
    sprites = []
    # Both purchase states retain the exact own-model silhouette. Only the
    # separate genuine calendar-lock badge is added to the UI canvas.
    for column, (state, pixels) in enumerate((("available", available), ("locked", locked))):
        atlas.paste(tube.base.quantize(pixels, {}), (column * 20, 0))
        sprites.append({"state": state, "direction": "none", "frame": 0, "part": "body", "anchor": [0, 0],
                        "box": [column * 20, 0, 20, 20], "source": original["path"], "source_sha256": original["sha256"],
                        "native_transform": transform, "lock_component": None if column == 0 else lock_provenance})
    return {"asset_id": purchase_id, "path": (tube.REVIEW / f"{stem}-purchase-authored.png").relative_to(tube.ROOT).as_posix(),
            "size": [40, 20], "native_cell": [20, 20], "sprites": sprites}, tube.indexed_png(atlas)


def build(document=None, asset_id="vehicle.model-09"):
    """Assemble144 load-preserving body poses and two real purchase presentations.

    @param document Optional isolated source provenance for focused malformed-input checks.
    @param asset_id Explicit public capsule body family; default preserves Mole artifacts.
    @return Complete source report and repository-path/indexed-PNG-byte map.
    """
    if document is None:
        document = json.loads(tube.RECORD.read_text(encoding="utf-8"))
    spec, family = family_spec(document, asset_id)
    name, stem = source_names(spec)
    components = spec["components"]
    cargo, donors = cargo_donors(components, document)
    indicator, lamps = status_lamps(components)
    selections = spec["selections"]
    if (len(selections) != 8 or {row["direction"] for row in selections} != set(DIRECTIONS)
            or any(row["state"] != "empty" for row in selections)):
        # Genuine base headings remain a prerequisite for every state/phase.
        raise ValueError("incomplete original capsule base view coverage")
    native_models, companies = {}, {}
    view_rows = []
    for direction in DIRECTIONS:
        selected = next(row for row in selections if row["direction"] == direction)
        original = tube.retained_candidate(selected["source"], document)
        # A complete selection uses this model's accepted physical hulls;
        # another capsule's valid source cannot substitute for its own design.
        if original["status"] not in ("selected", "partially_selected") or original["asset_id"] != asset_id:
            raise ValueError("unselected original capsule model")
        pixels, transform = models.native_pose(original, direction)
        view_rows.append({"direction": direction, "source": original["path"], "source_sha256": original["sha256"], "native_transform": transform})
        native_models[direction] = pixels, original, transform
        # Some genuine opposite views hide a model's side plaque behind its
        # opaque body. Only explicitly visible original fittings can remap;
        # default visibility preserves the existing Mole/Crate source bytes.
        visible_companies = components.get("company_visible_directions", [d for d in DIRECTIONS if d not in ("N", "S")])
        company_box = components.get("company_source_boxes", {}).get(direction)
        companies[direction] = company_plaque(original, direction, transform, pixels, company_box) if direction in visible_companies else None
    model_signature = component_signature({"model_views": view_rows})
    # Actual original native contacts were reviewed before state authoring.
    # A newly shifted/scaled crop cannot inherit those accepted registrations
    # just because the unchanged source image still has the same hash.
    if model_signature != family["geometry_notes"]["approved_model_views_sha256"]:
        raise ValueError("unapproved native capsule model registration")
    atlas = Image.new("P", (512, 864), 0)
    atlas.putpalette(tube.base.PALETTE_BYTES)
    sprites = []
    # One immutable hull keeps all real cargo levels through service/power
    # waits. Only approved original cargo pixels, glass and company fitting change.
    for state_index, state in enumerate(STATES):
        load, mode = spec["state_map"][state]["load"], spec["state_map"][state]["mode"]
        for column, direction in enumerate(DIRECTIONS):
            baseline, original, transform = native_models[direction]
            company = companies[direction]
            loaded = baseline.copy()
            if company is not None:
                loaded.putpixel(tuple(company["target"]), company["palette_index"])
            cargo_info = None
            if load != "empty":
                bx, by, width, height = components["cargo_native_boxes"][load][direction]
                donor = donors[load, direction].crop((bx, by, bx + width, by + height)).convert("RGBA")
                masks = components["hold_masks"][direction][:1 if load == "partial" else 2]
                heavy = components.get("cargo_presentation") == "heavy_modules"
                placement = [index for index in components["cargo_placement_order"][direction] if index < len(masks)] if heavy else range(len(masks))
                # Each approved cargo/level fitting clips only original
                # material. Structural frames, armour, chassis alpha and
                # neighboring native source cells stay exact.
                # Actual far-to-near module order handles overlapping solid
                # loads. Logical rear/front mask order still owns cargo level
                # and provenance; it must not become accidental drawing order.
                for mask_index in placement:
                    mask = masks[mask_index]
                    # An explicitly occluded fluid gauge has no visible
                    # material target. Its opaque tank remains exact through
                    # every actual cargo quantity and service/power condition.
                    if not mask:
                        continue
                    if heavy:
                        left, top, target_width, target_height = components["cargo_target_boxes"][direction][mask_index]
                        right, bottom = left + target_width, top + target_height
                    else:
                        left, top = min(p[0] for p in mask), min(p[1] for p in mask)
                        right, bottom = max(p[0] for p in mask) + 1, max(p[1] for p in mask) + 1
                    patch = tube.base.quantize(donor.resize((right - left, bottom - top), Image.Resampling.BOX), {})
                    for x, y in mask:
                        value = patch.getpixel((x - left, y - top))
                        # Genuine transparent module corners preserve pads
                        # and background. Only approved opaque heavy pixels
                        # may fill an originally open load aperture.
                        if heavy and value == 0:
                            continue
                        if baseline.getpixel((x, y)) == 0 and not heavy:
                            raise ValueError("original capsule cargo leaves hull alpha")
                        loaded.putpixel((x, y), value)
                cargo_info = {"path": cargo["path"], "sha256": cargo["sha256"], "source_cell": [column, 1 if load == "partial" else 2],
                              "native_donor_box": [bx, by, width, height], "target_masks": masks}
                if cargo.get("component") == "sealed_package_load":
                    # A component sheet has one original package per heading,
                    # not two physical load rows. Record that actual ancestry
                    # while the approved target pockets define load quantity.
                    del cargo_info["source_cell"]
                    cargo_info.update(source_view=direction, source_crop=components["cargo_source_crops"][direction],
                                      cargo_form="sealed_package_module")
                if cargo.get("component") == "sealed_fluid_level_glass":
                    # The source is a true level component rather than a
                    # direction/load wagon sheet. Explicit occlusion keeps
                    # internal liquid from appearing through opaque tanks.
                    del cargo_info["source_cell"]
                    cargo_info.update(source_state=load, source_crop=components["cargo_source_crops"][load],
                                      cargo_form="sealed_fluid_sight_glass", level_visible=direction in components["cargo_visible_directions"])
                if cargo.get("component") == "sealed_roof_load_meter":
                    # The same original amber glass lights actual roof meter
                    # fittings; this is neither a cargo texture nor a wagon
                    # direction/load sheet. Preserve its genuine state/phase.
                    del cargo_info["source_cell"]
                    cargo_info.update(source_state=components["cargo_meter_state"], source_frame=components["cargo_meter_frame"],
                                      cargo_form="sealed_mineral_roof_meter")
                if cargo.get("component") == "heavy_pallet_load":
                    # Whole same-view modules add only their genuine alpha
                    # inside the approved frame volumes. Preserve fixed box,
                    # occlusion order and original crop ancestry explicitly.
                    del cargo_info["source_cell"]
                    cargo_info.update(source_view=direction, source_crop=components["cargo_source_crops"][direction],
                                      cargo_form="heavy_pallet_module", alpha_extension=True,
                                      target_boxes=components["cargo_target_boxes"][direction][:len(masks)],
                                      placement_order=list(placement))
            x, y = components["cue_targets"][direction]
            if baseline.getpixel((x, y)) == 0:
                raise ValueError("original capsule lamp leaves hull alpha")
            for frame in range(2):
                pixels = loaded.copy()
                pixels.putpixel((x, y), lamps[mode][frame])
                sx, sy = column * 64, (state_index * 2 + frame) * 48
                atlas.paste(pixels, (sx, sy))
                sprites.append({"state": state, "load": load, "mode": mode, "direction": direction, "frame": frame,
                                "part": "body", "anchor": [-32, -32], "box": [sx, sy, 64, 48],
                                "source": original["path"], "source_sha256": original["sha256"], "native_transform": transform,
                                "cargo": cargo_info, "company_plaque": company,
                                "indicator": {"path": indicator["path"], "sha256": indicator["sha256"], "target": [x, y],
                                              "state": components["indicator_state_map"][mode], "frame": components["indicator_frames"][frame]}})
    body = {"asset_id": asset_id, "path": (tube.REVIEW / f"{stem}-authored.png").relative_to(tube.ROOT).as_posix(),
            "size": [512, 864], "native_cell": [64, 48], "sprites": sprites}
    purchase, purchase_pixels = purchase_source(spec, native_models)
    report = {"_documentation": "Plan13 Group49/master10.4; archi-map-layers.md. Original native Mole hulls and genuine cargo/glass/company/UI components; operating/packing/runtime pending.",
              "source_record": tube.RECORD.relative_to(tube.ROOT).as_posix(), "schema_version": 1,
              "source_coverage_complete": True, "runtime_activated": False, "pose_count": 146,
              "state_map": spec["state_map"], "component_signature": component_signature(components),
              "model_views_signature": model_signature, "atlases": [body, purchase]}
    if stem != "mole":
        material = "sealed fluid level" if components["cargo_key"] == "cistern-level-glass-v1" else "sealed package"
        if components.get("cargo_presentation") == "sealed_roof_meter":
            material = "sealed mineral roof-meter"
        if components.get("cargo_presentation") == "heavy_modules":
            material = "solid heavy module"
        report["_documentation"] = f"Plan13 Group{spec['model_id'] + 40}/master10.4; archi-map-layers.md. Original native {name} hulls and genuine {material}/glass/company/UI components; operating/packing/runtime pending."
    return report, {tube.base.repository_path(body["path"]): tube.indexed_png(atlas),
                    tube.base.repository_path(purchase["path"]): purchase_pixels}


def validate(report, files, document=None, asset_id="vehicle.model-09"):
    """Reject incomplete poses, lost cargo, painted materials and false ancestry.

    @param report Complete original-model/component source handoff to inspect.
    @param files Repository-path/indexed-atlas bytes, optionally corrupted in memory.
    @param document Optional isolated original source record for refusal checks.
    @param asset_id Explicit public capsule family whose actual source must match.
    @return None; source acceptance never establishes packing or runtime operation.
    """
    if (report.get("source_coverage_complete") is not True or report.get("runtime_activated") is not False
            or report.get("pose_count") != 146 or report.get("source_record") != tube.RECORD.relative_to(tube.ROOT).as_posix()):
        # Source coverage and actual simulation activation are different gates.
        raise ValueError("false native capsule source/runtime claim")
    required_assets = {asset_id, asset_id.replace("vehicle.", "purchase.", 1)}
    atlases = report["atlases"]
    if len(atlases) != 2 or {row["asset_id"] for row in atlases} != required_assets:
        raise ValueError("incomplete native capsule body/purchase family coverage")
    for atlas in atlases:
        body = atlas["asset_id"] == asset_id
        expected = {(state, direction, frame) for state in (STATES if body else ("available", "locked"))
                    for direction in (DIRECTIONS if body else ("none",)) for frame in (range(2) if body else range(1))}
        actual = [(row["state"], row["direction"], row["frame"]) for row in atlas["sprites"]]
        if len(actual) != len(expected) or set(actual) != expected:
            # Counts alone cannot hide a missing loaded blocked/service pose,
            # a duplicate animation phase or an absent purchase presentation.
            raise ValueError("incomplete native capsule state/view/phase coverage")
        with Image.open(io.BytesIO(files[tube.base.repository_path(atlas["path"])])) as image:
            if (image.mode != "P" or image.size != ((512, 864) if body else (40, 20))
                    or image.info.get("transparency") != 0 or image.getpalette()[:768] != tube.base.PALETTE_BYTES):
                raise ValueError("native capsule palette/canvas mismatch")
            forbidden = tube.base.FORBIDDEN - {0} - (set(range(198, 206)) if body else set())
            if set(image.get_flattened_data()) & forbidden:
                raise ValueError("native capsule forbidden palette index")
            # Primary company remap belongs exclusively to the original blue
            # side fitting. Cargo, amber hull and status glass keep fixed colours.
            for row in atlas["sprites"]:
                x, y, width, height = row["box"]
                anchor = [-32, -32] if body else [0, 0]
                if row["anchor"] != anchor or row["part"] != "body":
                    raise ValueError("native capsule anchor/part mismatch")
                tile = image.crop((x, y, x + width, y + height))
                if tile.getbbox() is None:
                    raise ValueError("empty native capsule source pose")
                if body:
                    company = row["company_plaque"]
                    allowed = {tuple(company["target"])} if company else set()
                    remapped = {(px, py) for py in range(48) for px in range(64) if 198 <= tile.getpixel((px, py)) < 206}
                    if remapped != allowed:
                        raise ValueError("native capsule company mask leaves original fitting")
    expected, pixels = build(document, asset_id)
    for actual, canonical in zip(atlases, expected["atlases"]):
        if any(actual[key] != canonical[key] for key in ("asset_id", "path", "size", "native_cell")):
            raise ValueError("native capsule false atlas geometry")
        if actual["sprites"] != canonical["sprites"]:
            raise ValueError("native capsule false original/component ancestry")
    # Both model contacts and component targets belong to the approved source
    # handoff. Neither cargo condition nor a valid image hash authorizes drift.
    if (report["state_map"] != expected["state_map"] or report["component_signature"] != expected["component_signature"]
            or report["model_views_signature"] != expected["model_views_signature"]):
        raise ValueError("native capsule loses original cargo/mode identity")
    if files != pixels:
        raise ValueError("native capsule changes original hull/cargo/component pixels")


def contact_pages(report, files):
    """Expose all real load/mode views and both genuine phases at native scale.

    @param report Complete original source report containing144 body/two UI poses.
    @param files Exact indexed source atlas bytes for visible review.
    @return Named review PNG byte map; diagrams never become game art.
    """
    result = {}
    body, purchase = report["atlases"]
    document = json.loads(tube.RECORD.read_text(encoding="utf-8"))
    spec, unused = family_spec(document, body["asset_id"])
    name, stem = source_names(spec)
    cargo_name = "ore" if stem == "mole" else "package"
    if spec["components"]["cargo_key"] == "cistern-level-glass-v1":
        cargo_name = "sealed fluid level"
    if spec["components"].get("cargo_presentation") == "sealed_roof_meter":
        cargo_name = "sealed mineral meter"
    if spec["components"].get("cargo_presentation") == "heavy_modules":
        cargo_name = "solid heavy module"
    with Image.open(io.BytesIO(files[tube.base.repository_path(body["path"])])) as atlas:
        # Each mode page retains empty/partial/full cargo independently. Native
        # phase strips reveal repeated lamps and load loss in held/service poses.
        for mode in ("operating", "service", "blocked"):
            board = Image.new("RGB", (2048, 900), (48, 52, 58))
            draw = ImageDraw.Draw(board)
            draw.text((12, 12), f"{name} {mode}: original hull/{cargo_name}/lamp/company fitting; complete source, operating/packing/runtime pending", fill="white")
            for load_index, load in enumerate(("empty", "partial", "full")):
                top = 45 + load_index * 280
                for column, direction in enumerate(DIRECTIONS):
                    left = column * 256
                    draw.text((left + 30, top), f"{direction} {load};4x / two native phases", fill="white")
                    for frame in range(2):
                        row = next(row for row in body["sprites"] if (row["mode"], row["load"], row["direction"], row["frame"]) == (mode, load, direction, frame))
                        x, y, width, height = row["box"]
                        image = atlas.crop((x, y, x + width, y + height)).convert("RGBA")
                        board.paste(image, (left + 32 + frame * 96, top + 215), image)
                        if frame == 0:
                            enlarged = image.resize((256, 192), Image.Resampling.NEAREST)
                            board.paste(enlarged, (left, top + 20), enlarged)
            stream = io.BytesIO()
            board.save(stream, format="PNG")
            result[f"{stem}-{mode}-contact.png"] = stream.getvalue()
    with Image.open(io.BytesIO(files[tube.base.repository_path(purchase["path"])])) as atlas:
        board = Image.new("RGB", (580, 340), (48, 52, 58))
        draw = ImageDraw.Draw(board)
        draw.text((12, 12), f"{name} purchase: own original empty hull + genuine calendar-lock UI", fill="white")
        for column, state in enumerate(("available", "locked")):
            image = atlas.crop((column * 20, 0, column * 20 + 20, 20)).convert("RGBA")
            enlarged = image.resize((240, 240), Image.Resampling.NEAREST)
            board.paste(enlarged, (column * 280 + 20, 45), enlarged)
            board.paste(image, (column * 280 + 125, 300), image)
            draw.text((column * 280 + 80, 28), state, fill="white")
        stream = io.BytesIO()
        board.save(stream, format="PNG")
        result[f"{stem}-purchase-contact.png"] = stream.getvalue()
    return result


def publish(check=False, asset_id="vehicle.model-09"):
    """Rebuild complete source/contact proof or compare its exact bytes read-only.

    @param check True to refuse stale proof without changing repository files.
    @param asset_id Explicit public capsule family; default preserves existing Mole output.
    @return Complete source manifest path; no packed asset is written.
    """
    report, files = build(asset_id=asset_id)
    validate(report, files, asset_id=asset_id)
    spec, unused = family_spec(json.loads(tube.RECORD.read_text(encoding="utf-8")), asset_id)
    target = tube.REVIEW / (source_names(spec)[1] + "-source-manifest.json")
    for atlas in report["atlases"]:
        atlas["sha256"] = hashlib.sha256(files[tube.base.repository_path(atlas["path"])]).hexdigest()
    report["contact_pages"] = []
    for name, pixels in contact_pages(report, files).items():
        path = tube.REVIEW / name
        files[path] = pixels
        report["contact_pages"].append({"path": path.relative_to(tube.ROOT).as_posix(), "sha256": hashlib.sha256(pixels).hexdigest()})
    text = json.dumps(report, indent=2) + "\n"
    if check:
        if any(path.read_bytes() != pixels for path, pixels in files.items()) or target.read_text(encoding="utf-8") != text:
            # Pixels, actual source ancestry and every contact page regenerate
            # together; stale cached previews cannot inherit source acceptance.
            raise ValueError("native capsule source proof needs deterministic regeneration")
    else:
        tube.REVIEW.mkdir(parents=True, exist_ok=True)
        for path, pixels in files.items():
            path.write_bytes(pixels)
        target.write_text(text, encoding="utf-8")
    return target


def bind_source(asset_id="vehicle.model-09"):
    """Bind both complete original source families without activating runtime/packing.

    @param asset_id Explicit public capsule body family to publish with its purchase art.
    @return None; only the selected model's body/purchase source metadata is updated.
    """
    target = publish(check=True, asset_id=asset_id)
    report = json.loads(target.read_text(encoding="utf-8"))
    public = json.loads(tube.PUBLIC.read_text(encoding="utf-8"))
    originals = list(dict.fromkeys(row["source"] for atlas in report["atlases"] for row in atlas["sprites"]))
    for row in report["atlases"][0]["sprites"]:
        for component in (row["cargo"], row["indicator"]):
            if component is not None and component["path"] not in originals:
                originals.append(component["path"])
    lock = report["atlases"][1]["sprites"][1]["lock_component"]
    originals.append(lock["path"])
    # Body and purchase coverage share genuine model/components, while each
    # retains its own actual world/UI canvas, source count and packed identity.
    for atlas in report["atlases"]:
        family = next(row for row in public["assets"] if row["asset_id"] == atlas["asset_id"])
        first = atlas["sprites"][0]
        family["source"] = {"status": "authored", "path": first["source"], "sha256": first["source_sha256"],
                            "provenance": "first_party_original_model_and_component_assembly", "rights": "first_party_generated_original"}
        family["authoring"] = {"date": "2026-10-10", "status": "complete_native_original_capsule_source_runtime_pending",
                               "source_record": report["source_record"], "originals": originals,
                               "atlas_manifest": target.relative_to(tube.ROOT).as_posix(), "native_atlas": atlas["path"],
                               "native_sha256": atlas["sha256"], "staged_pose_count": len(atlas["sprites"]),
                               "coverage_complete": True, "runtime_activated": False}
    tube.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def main():
    """Build or verify one native capsule source with explicit public publication.

    @return None; print the proof path and preserve normal refusal diagnostics.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--check", action="store_true", help="Read-only exact source regeneration")
    modes.add_argument("--publish-source", action="store_true", help="Bind complete source coverage without packing")
    parser.add_argument("--asset", default="vehicle.model-09", help="Explicit public capsule body family")
    args = parser.parse_args()
    print(publish(args.check, args.asset))
    if args.publish_source:
        bind_source(args.asset)


if __name__ == "__main__":
    main()
