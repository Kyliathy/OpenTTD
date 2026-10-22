"""Review retained conventional freight bodies before load/state authoring.

@file convert_ground_fleet_art.py
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DC, retained original generation inputs.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group20
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group20C
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group20B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group21 shared native vehicle placement
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group22 sealed road placement
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
Native vehicle definitions own length, capacity, cargo and admission. These
base-view artifacts publish neither complete family coverage nor runtime art.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageChops, ImageDraw

import convert_m1_art as base
from convert_terrain_art import png_bytes
from tools.ground_fleet_contract import cargo_geometry, mine_truck_cargo_geometry, native_cargo_forms, variant_contract
from convert_space_art import source_reference


RECORD = base.ROOT / "media/multilayer/source/ground/fleet-generation-record.json"
OUTPUT = base.ROOT / "logs/proofs/mlm/native-art/ground"
DIRECTIONS = ("N", "NE", "E", "SE", "S", "SW", "W", "NW")
ENVELOPES = {"N": (16, 24), "NE": (24, 24), "E": (32, 24), "SE": (24, 24),
             "S": (16, 24), "SW": (24, 24), "W": (32, 24), "NW": (24, 24)}


def registration_signature(source, direction):
    """Identify one approved original/view transform independently of review notes.

    @param source Retained candidate containing immutable bytes and view crops.
    @param direction Genuine engine heading selected from that candidate.
    @return SHA256 freezing original identity, heading and native placement.
    """
    fields = {"sha256": source["sha256"], "view": direction,
              "native_transform": source["native_transforms"][direction]}
    return hashlib.sha256(json.dumps(fields, sort_keys=True).encode()).hexdigest()


def public_family(asset_id, public=None):
    """Require the selected ground freight body's shared canvas and source matrix.

    @param asset_id Delivered native model's public artwork family identity.
    @param public Optional isolated public contract for boundary refusal checks.
    @return Existing selected05..08 contract; no private cargo or engine identity.
    """
    if public is None:
        public = json.loads(base.PUBLIC.read_text(encoding="utf-8"))
    if asset_id not in ("vehicle.model-05", "vehicle.model-06", "vehicle.model-07", "vehicle.model-08"):
        raise ValueError("ground fleet base geometry is not approved for this model")
    matches = [row for row in public["assets"] if row["asset_id"] == asset_id]
    if len(matches) != 1:
        raise ValueError("missing or duplicate conventional wagon family")
    spec = matches[0]
    # Source selection cannot shorten the actual full-length vehicle, introduce
    # another part, or replace Plan7's native model with a generic icon canvas.
    if (spec["kind"] != "vehicle" or spec["consumer"] != 7
            or spec["directions"] != list(DIRECTIONS) or spec["frames"] != 2
            or spec["states"] != ["empty", "partial", "full", "service", "blocked"]
            or spec["native_size"] != [64, 48] or spec["anchor"] != [-32, -32]
            or spec["parts"] != ["body"]):
        raise ValueError("conventional wagon public geometry or state matrix changed")
    # Native admitted labels remain the sole cargo meaning owner. A shortened
    # art matrix or invented status source cannot silently narrow that wagon.
    model_id = int(asset_id.rsplit("-", 1)[1])
    if any(spec.get(key) != value for key, value in variant_contract(model_id).items()):
        raise ValueError("conventional wagon cargo variant/state ownership changed")
    # Only the ordinary open gondola admits actual exposed cargo apertures. The
    # sealed model retains its own closed exterior and packet-backed indicators.
    if model_id == 5 and spec.get("cargo_geometry") != cargo_geometry():
        raise ValueError("conventional wagon cargo geometry changed")
    # The road truck's lower open bed has its own actual floor and cab cuts;
    # matching canvas dimensions never authorize copying the rail aperture.
    if model_id == 7 and spec.get("cargo_geometry") != mine_truck_cargo_geometry():
        raise ValueError("mine truck cargo geometry changed")
    return spec


def native_view(key, direction, document=None, public=None):
    """Convert a complete measured source rectangle without repairing its pixels.

    @param key Exact retained candidate identity, never a latest-file lookup.
    @param direction Genuine source heading with its frozen view registration.
    @param document Optional isolated generation record for refusal checks.
    @param public Optional isolated public sprite contract.
    @return Indexed64x48 native body and its original/placement evidence.
    @throws ValueError On changed ancestry, clipping, separators or registration.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    matches = [row for row in document["candidates"] if row["key"] == key]
    if len(matches) != 1:
        raise ValueError("missing or ambiguous retained wagon original")
    source = matches[0]
    public_family(source["asset_id"], public)
    if source["status"].startswith("rejected"):
        raise ValueError("rejected wagon candidate cannot supply a selected view")
    if direction not in source.get("native_transforms", {}):
        raise ValueError("wagon original has no measured requested heading")
    # Every actual reference remains immutable, including the selected E body
    # used to preserve model identity across separately generated headings.
    for identity in [source] + source["references"]:
        source_reference(identity, document, feature="ground")
    signature = registration_signature(source, direction)
    approved = document["families"][source["asset_id"]]["approved_base_registration"].get(direction)
    if signature != approved:
        raise ValueError("selected wagon registration changed after approval")
    transform = source["native_transforms"][direction]
    # The same direction envelope and ground-contact baseline used by the
    # delivered M1 fleet keep a new body compatible with native vehicle drawing.
    if (transform["native_canvas"] != [64, 48] or transform["native_anchor"] != [-32, -32]
            or transform["maximum_subject_size"] != list(ENVELOPES[direction])
            or transform["alpha_crop_threshold"] != 16 or transform["transparent_clearance"] != 16):
        raise ValueError("wagon native placement differs from its approved geometry")
    original = Image.open(base.ROOT / source["path"]).convert("RGBA")
    left, top, right, bottom = transform["crop_box"]
    # Out-of-image rectangles can silently clip couplers or wheel contacts,
    # making a complete source claim incompatible with the actual train chain.
    if not (0 <= left < right <= original.width and 0 <= top < bottom <= original.height):
        raise ValueError("wagon source rectangle leaves its original")
    crop = original.crop((left, top, right, bottom))
    # Alpha16 measures every visible edge; the existing DOS quantizer owns final
    # native alpha. Low-alpha RGB in a transparent source background is not art.
    bounds = crop.getchannel("A").point(lambda alpha: 255 if alpha >= 16 else 0).getbbox()
    if bounds is None:
        raise ValueError("empty selected wagon rectangle")
    if list(bounds) != transform["measured_subject_bbox_in_crop"]:
        raise ValueError("wagon subject bounds differ from measured registration")
    clearance = min(bounds[0], bounds[1], crop.width - bounds[2], crop.height - bounds[3])
    if clearance < transform["transparent_clearance"]:
        raise ValueError("wagon original clips its transparent clearance")
    if any(alpha >= 16 and red > 220 and green < 60 and blue > 220
           for red, green, blue, alpha in crop.get_flattened_data()):
        # Source separators may not become freight, livery or contact pixels.
        # Refuse the crop rather than erase magenta or infer a replacement mask.
        raise ValueError("opaque source separator enters selected wagon crop")
    native = base.fit(crop.crop(bounds), (64, 48), ENVELOPES[direction])
    image = base.quantize(native, {})
    image.info["transparency"] = 0
    evidence = {"direction": direction, "state": "empty", "frame": 0, "part": "body",
                "source": key, "source_path": source["path"], "source_sha256": source["sha256"],
                "registration_sha256": signature, "source_crop": transform["crop_box"],
                "transparent_clearance": clearance, "anchor": [-32, -32],
                "native_opaque_bounds": list(image.getbbox())}
    return image, evidence


def cargo_signature(family):
    """Freeze cargo placement together with the selected physical body registrations.

    @param family Retained model05 source family containing its eight approved views.
    @return SHA256 of native cargo meaning, load geometry and fixed body ancestry.
    """
    fields = {"variants": variant_contract(), "geometry": cargo_geometry(),
              "body_registrations": family["approved_base_registration"]}
    return hashlib.sha256(json.dumps(fields, sort_keys=True).encode()).hexdigest()


def cargo_mask(direction, load, geometry=None):
    """Select one approved bulk hold region while preserving its foreground rim.

    @param direction Actual engine heading of the unchanged wagon body.
    @param load Partial or full packet-backed visual category.
    @param geometry Optional isolated frozen geometry for source refusal checks.
    @return 64x48 alpha mask for original granules only; no body or cargo pixels.
    """
    if geometry is None:
        geometry = cargo_geometry()
    # Unknown headings or quantities cannot borrow another source slot and
    # manufacture an apparently complete refit/state matrix.
    if direction not in DIRECTIONS or load not in ("partial", "full"):
        raise ValueError("unknown ordinary wagon cargo view or fill category")
    mask = Image.new("L", (64, 48), 0)
    draw = ImageDraw.Draw(mask)
    draw.polygon([tuple(point) for point in geometry["bulk_" + load + "_polygons"][direction]], fill=255)
    if load == "partial":
        # Native one-pixel diagonal rounding must not put a partial bank beyond
        # its full hold. This clips geometry only, never authored source colors.
        mask = ImageChops.multiply(mask, cargo_mask(direction, "full", geometry))
        draw = ImageDraw.Draw(mask)
    # Front steel belongs to the fixed body, even when original granules fill
    # the entire hold behind it. Preserve its exact source colors and alpha.
    for line in geometry["foreground_rim_lines"][direction]:
        draw.line([tuple(point) for point in line], fill=0, width=1)
    return mask


def cargo_geometry_guide(document=None):
    """Show load apertures against actual body pixels before cargo source selection.

    @param document Optional retained source record; defaults to curated inputs.
    @return Opaque review board: green full, amber partial, cyan raised, red rim.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    family = document["families"]["vehicle.model-05"]
    if family["approved_cargo_geometry_sha256"] != cargo_signature(family):
        raise ValueError("ordinary wagon load geometry/body registration changed")
    geometry = cargo_geometry()
    canvas = Image.new("RGB", (1048, 560), (42, 46, 50))
    labels = ImageDraw.Draw(canvas)
    labels.text((12, 12), "GEOMETRY ONLY: green full / amber partial / cyan raised envelope / red retained front rim; no cargo artwork", fill="white")
    # Actual selected headings, rather than rotated templates, establish how
    # each load sits behind the rim and above untouched wheels/couplers.
    for index, direction in enumerate(DIRECTIONS):
        image, _evidence = native_view(family["selections"]["empty/" + direction], direction, document)
        overlay = Image.new("RGBA", (64, 48))
        draw = ImageDraw.Draw(overlay)
        draw.bitmap((0, 0), cargo_mask(direction, "full", geometry), fill=(0, 220, 100, 150))
        draw.bitmap((0, 0), cargo_mask(direction, "partial", geometry), fill=(255, 190, 0, 180))
        draw.line([tuple(point) for point in geometry["protected_load_envelopes"][direction]]
                  + [tuple(geometry["protected_load_envelopes"][direction][0])], fill=(0, 210, 255, 255), width=1)
        # The near steel edge is an occluder, not part of a newly painted cargo
        # texture. Its location must remain readable in every guide view.
        for line in geometry["foreground_rim_lines"][direction]:
            draw.line([tuple(point) for point in line], fill=(255, 80, 80, 255), width=1)
        pose = Image.alpha_composite(image.convert("RGBA"), overlay)
        left, top = 12 + index % 4 * 256, 40 + index // 4 * 250
        labels.text((left, top), direction, fill="white")
        canvas.paste(pose, (left + 80, top + 16), pose)
        large = pose.resize((256, 192), Image.Resampling.NEAREST)
        canvas.paste(large, (left, top + 58), large)
    return canvas


def base_contact(atlas, sprites, asset_id, caption=None):
    """Expose every selected engine view at native and nearest-enlarged scale.

    @param atlas Indexed source base atlas, not a screenshot from the game.
    @param sprites Measured source poses with actual native atlas rectangles.
    @param asset_id Shared family identity printed for review traceability.
    @param caption Optional truthful source-stage caption for load review pages.
    @return RGB contact sheet with complete unclipped1x/4x view windows.
    """
    canvas = Image.new("RGB", (1048, 560), (42, 46, 50))
    draw = ImageDraw.Draw(canvas)
    title = caption if caption is not None else f"SOURCE BASE ONLY: {asset_id}; empty frame0; native1x / nearest4x; states/purchase/runtime pending"
    draw.text((12, 12), title, fill="white")
    # Fixed engine ordering makes adjacent camera continuity visible without
    # synthesizing missing views or presenting blank cells as completed sources.
    for row in sprites:
        index = DIRECTIONS.index(row["direction"])
        left, top = 12 + index % 4 * 256, 40 + index // 4 * 250
        x, y, width, height = row["box"]
        pose = atlas.crop((x, y, x + width, y + height)).convert("RGBA")
        draw.text((left, top), row["direction"], fill="white")
        canvas.paste(pose, (left + 80, top + 16), pose)
        large = pose.resize((256, 192), Image.Resampling.NEAREST)
        canvas.paste(large, (left, top + 58), large)
    return canvas


def build_base(asset_id="vehicle.model-05", document=None, public=None):
    """Build only genuinely selected empty frame-zero poses from retained originals.

    @param asset_id Approved conventional wagon family to review.
    @param document Optional isolated source record; defaults to retained inputs.
    @param public Optional isolated shared contract for refusal checks.
    @return Source-only report and reproducible artifact bytes; no public writes.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    spec = public_family(asset_id, public)
    selection = document["families"][asset_id]["selections"]
    present = [direction for direction in DIRECTIONS if "empty/" + direction in selection]
    if not present:
        raise ValueError("wagon base review has no selected genuine views")
    atlas = Image.new("P", (64 * len(present), 48), 0)
    atlas.putpalette(base.PALETTE_BYTES)
    atlas.info["transparency"] = 0
    sprites = []
    # Each selected source contributes exactly one real view. Missing headings
    # remain explicit; the converter never rotates, mirrors or pads another view.
    for index, direction in enumerate(present):
        selected = [row for row in document["candidates"] if row["key"] == selection["empty/" + direction]]
        # Shared camera tooling cannot turn another model's approved silhouette
        # into this selected family merely because its dimensions agree.
        if len(selected) != 1 or selected[0]["asset_id"] != asset_id:
            raise ValueError("wagon base selection belongs to another native model")
        image, evidence = native_view(selection["empty/" + direction], direction, document, public)
        atlas.paste(image, (64 * index, 0))
        sprites.append({**evidence, "box": [64 * index, 0, 64, 48]})
    name = asset_id.replace(".", "-") + "-base"
    path = OUTPUT / (name + "-authored.png")
    encoded = png_bytes(atlas)
    report = {"_documentation": "Owner: r2mlvs14-remaining-assets-and-runtime-integration.md Group20; base geometry only",
              "schema_version": 1, "asset_id": asset_id, "source_record": RECORD.relative_to(base.ROOT).as_posix(),
              "path": path.relative_to(base.ROOT).as_posix(), "sha256": hashlib.sha256(encoded).hexdigest(),
              "base_geometry_complete": len(present) == len(DIRECTIONS),
              "missing_base_directions": [direction for direction in DIRECTIONS if direction not in present],
              "body_source_pose_count": len(sprites),
              "required_family_pose_count": len(spec["states"]) * spec["frames"] * len(DIRECTIONS) * spec["tile_variants"],
              "source_coverage_complete": False, "packed": False, "runtime_activated": False, "sprites": sprites}
    # The RGB review background is intentionally opaque. Indexed sprite alpha
    # belongs only to the atlas, so a palette transparency flag cannot encode it.
    contact = io.BytesIO()
    base_contact(atlas, sprites, asset_id).save(contact, format="PNG", optimize=False, compress_level=9)
    files = {path: encoded, OUTPUT / (name + "-source-manifest.json"): (json.dumps(report, indent=2) + "\n").encode(),
             OUTPUT / (name + "-contact.png"): contact.getvalue()}
    return report, files


def material_signature(source, label):
    """Identify one selected original grain interior without workflow metadata.

    @param source Retained isotropic material candidate and measured source crops.
    @param label Actual native dry-bulk cargo label owning that material.
    @return SHA256 freezing original identity and both extraction rectangles.
    """
    fields = {"sha256": source["sha256"], "component_kind": source["component_kind"],
              "label": label, "material": source["component_materials"][label]}
    return hashlib.sha256(json.dumps(fields, sort_keys=True).encode()).hexdigest()


def bulk_material(label, document, model_id=5):
    """Read an approved opaque grain interior without fabricating directional art.

    @param label Actual admitted native cargo whose physical form is dry bulk.
    @param document Retained body/material provenance and frozen source matrix.
    @param model_id Actual open freight body05 or road truck07; default05.
    @return RGBA original material crop plus exact source/registration evidence.
    """
    if model_id not in (5, 7):
        raise ValueError("bulk material owner is not an open native freight body")
    family = document["families"][f"vehicle.model-{model_id:02}"]
    rows = [row for row in family["cargo_source_matrix"] if row["cargo_label"] == label]
    # Isotropic granules describe unstructured bulk only. This path cannot turn
    # a crate, protected concentrate or ingot into a colored grain substitute.
    if len(rows) != 1 or rows[0]["form"] != "dry_bulk" or native_cargo_forms(model_id).get(label) != "dry_bulk":
        raise ValueError("ordinary wagon grain donor is not an actual bulk refit")
    approved = family["approved_bulk_materials"].get(label)
    if approved is None:
        raise ValueError("ordinary wagon bulk material has no approved original")
    # Both fill levels must name the same genuinely selected material. A stale
    # source-matrix pointer cannot authorize another mineral or hide a missing
    # full/partial source behind the separately approved donor catalogue.
    for load in ("partial", "full"):
        reference = rows[0].get(load + "_source") or {}
        if reference.get("source") != approved["source"] or reference.get("material") != label:
            raise ValueError("wagon bulk fill source differs from its selected material")
    matches = [row for row in document["candidates"] if row["key"] == approved["source"]]
    if len(matches) != 1:
        raise ValueError("missing or ambiguous wagon grain original")
    source = matches[0]
    # A directional object carries shape/lighting that needs genuine views.
    # Only separately authored direction-independent grain interiors may be
    # resampled into the fixed physical hold already drawn in eight directions.
    if (source["component_kind"] != "isotropic_granules" or not source["status"].startswith("selected_")
            or label not in source["component_materials"]):
        raise ValueError("unselected or directional wagon grain component")
    # Source/reference bytes remain immutable even when workflow notes change;
    # material selection cannot silently replace the approved grain ancestry.
    for identity in [source] + source["references"]:
        source_reference(identity, document, feature="ground")
    signature = material_signature(source, label)
    if signature != approved["signature"]:
        raise ValueError("wagon grain crop changed after approval")
    registration = source["component_materials"][label]
    original = Image.open(base.ROOT / source["path"]).convert("RGBA")
    left, top, right, bottom = registration["source_window"]
    # Whole-patch extraction must preserve every visible edge before selecting
    # a genuinely opaque interior; cropped separators cannot masquerade as ore.
    if not (0 <= left < right <= original.width and 0 <= top < bottom <= original.height):
        raise ValueError("wagon material source window leaves original")
    whole = original.crop((left, top, right, bottom))
    bounds = whole.getchannel("A").point(lambda alpha: 255 if alpha >= 16 else 0).getbbox()
    if bounds is None:
        raise ValueError("empty wagon material patch")
    clearance = min(bounds[0], bounds[1], whole.width - bounds[2], whole.height - bounds[3])
    if list(bounds) != registration["whole_subject_bounds_in_window"]:
        raise ValueError("wagon material whole-patch registration changed")
    if clearance < 16:
        raise ValueError("wagon material whole patch clips its clearance")
    if any(alpha >= 16 and red > 220 and green < 60 and blue > 220
           for red, green, blue, alpha in whole.get_flattened_data()):
        # A colored separator is source layout, never mineral cargo. Refuse it
        # without erasing original pixels or inserting a procedural replacement.
        raise ValueError("opaque source separator enters wagon material patch")
    x1, y1, x2, y2 = registration["material_crop"]
    # An interior that leaves its approved whole patch could sample transparent
    # layout or another mineral and then falsely present a full physical hold.
    if not (left <= x1 < x2 <= right and top <= y1 < y2 <= bottom):
        raise ValueError("wagon grain interior leaves its complete source patch")
    crop = original.crop((x1, y1, x2, y2))
    if crop.getchannel("A").getextrema()[0] < 128:
        raise ValueError("wagon grain interior contains transparent background")
    evidence = {"source": source["key"], "source_path": source["path"], "source_sha256": source["sha256"],
                "material": label, "material_signature": signature, "source_window": registration["source_window"],
                "material_crop": registration["material_crop"], "whole_patch_clearance": clearance}
    return crop, evidence


def compose_bulk(body, material, direction, load):
    """Place original granules behind the unchanged physical wagon's front rim.

    @param body Approved indexed64x48 empty frame-zero body for this heading.
    @param material Opaque original isotropic granule interior, never body pixels.
    @param direction Actual eight-way source heading of that same native body.
    @param load Partial or full packet-backed visual fill category.
    @return Indexed native load pose; all pixels/alpha outside its mask stay exact.
    """
    full = cargo_mask(direction, "full")
    mask = cargo_mask(direction, load)
    # Bulk stays contained in the existing opaque interior. No load projection
    # may extend the chassis silhouette or cover its contact hardware.
    if any(allowed and pixel == 0 for allowed, pixel in zip(full.get_flattened_data(), body.get_flattened_data())):
        raise ValueError("ordinary wagon bulk aperture extends outside its body")
    if material.getchannel("A").getextrema()[0] < 128:
        raise ValueError("ordinary wagon bulk donor has transparent gaps")
    left, top, right, bottom = full.getbbox()
    donor = base.quantize(material.resize((right - left, bottom - top), Image.Resampling.BOX), {})
    image = body.copy()
    # Both levels sample the same original material coordinates. Partial shows
    # only the rear occupied portion; full adds granules without repainting the
    # existing bank, drawing another wagon view or using a second cargo ledger.
    image.paste(donor, (left, top), mask.crop((left, top, right, bottom)))
    if image.tobytes() == body.tobytes():
        raise ValueError("wagon bulk fill disappeared at native palette scale")
    return image


def build_bulk(document=None, public=None):
    """Assemble selected bulk refits while exposing every missing frame/state/refit.

    @param document Optional isolated generation record with genuine selected donors.
    @param public Optional isolated shared sprite contract for boundary checks.
    @return Staged source report and exact base/load/review artifacts, without writes.
    """
    if document is None:
        document = json.loads(RECORD.read_text(encoding="utf-8"))
    family = document["families"]["vehicle.model-05"]
    spec = public_family("vehicle.model-05", public)
    if family["approved_cargo_geometry_sha256"] != cargo_signature(family):
        raise ValueError("ordinary wagon cargo geometry/body registration changed")
    expected = [(index, label) for index, label in enumerate(spec["native_cargo_labels"])]
    actual = [(row["variant"], row["cargo_label"]) for row in family["cargo_source_matrix"]]
    if actual != expected:
        raise ValueError("ordinary wagon source refits differ from the native catalogue")
    base_report, files = build_base(document=document, public=public)
    if not base_report["base_geometry_complete"]:
        raise ValueError("ordinary wagon load authoring requires all real body views")
    base_atlas = Image.open(io.BytesIO(files[base.ROOT / base_report["path"]]))
    bodies = {}
    # Crop each actual regenerated body once. All refits retain that exact
    # heading's couplers, wheels and source registration across both fill levels.
    for pose in base_report["sprites"]:
        x, y, width, height = pose["box"]
        bodies[pose["direction"]] = base_atlas.crop((x, y, x + width, y + height))
    rows = [row for row in family["cargo_source_matrix"] if row["cargo_label"] in family["approved_bulk_materials"]]
    if not rows:
        raise ValueError("ordinary wagon bulk handoff has no selected materials")
    atlas = Image.new("P", (8 * 64, len(rows) * 2 * 48), 0)
    atlas.putpalette(base.PALETTE_BYTES)
    atlas.info["transparency"] = 0
    sprites, materials = [], []
    # Material selection is explicit per actual cargo. There is no recolored
    # generic ore, missing-refit fallback or copied second animation frame.
    for row_index, row in enumerate(rows):
        label = row["cargo_label"]
        material, evidence = bulk_material(label, document)
        materials.append(evidence)
        for level_index, load in enumerate(("partial", "full")):
            for view_index, direction in enumerate(DIRECTIONS):
                image = compose_bulk(bodies[direction], material, direction, load)
                x, y = 64 * view_index, 48 * (row_index * 2 + level_index)
                atlas.paste(image, (x, y))
                sprites.append({"cargo_label": label, "tile_variant": row["variant"], "state": load,
                                "frame": 0, "direction": direction, "part": "body", "box": [x, y, 64, 48],
                                "anchor": [-32, -32], "material": evidence,
                                "body_atlas": base_report["path"], "body_sha256": base_report["sha256"]})
    # An empty wagon has the same physical body for every admitted refit. These
    # explicit aliases reuse only real frame0 views; moving/status/purchase
    # sources remain absent rather than being padded with that empty artwork.
    aliases = [{"tile_variant": variant, "cargo_label": label, "state": "empty", "frame": 0,
                "direction": pose["direction"], "part": "body", "source_atlas": base_report["path"],
                "source_sha256": base_report["sha256"], "source_box": pose["box"]}
               for variant, label in expected for pose in base_report["sprites"]]
    present = {(row["state"], row["frame"], row["direction"], row["tile_variant"]) for row in sprites + aliases}
    # Coverage follows the entire public matrix, including all currently absent
    # protected/heavy refits and gear/status phases. Material reuse is not full
    # source delivery and cannot activate the existing native payload.
    required = {(state, frame, direction, variant) for state in spec["states"] for frame in range(spec["frames"])
                for direction in DIRECTIONS for variant in range(spec["tile_variants"])}
    path = OUTPUT / "vehicle-model-05-bulk-authored.png"
    encoded = png_bytes(atlas)
    report = {"_documentation": "Owner: r2mlvs14-remaining-assets-and-runtime-integration.md Group20C; bulk frame0 only",
              "schema_version": 1, "asset_id": "vehicle.model-05", "source_record": RECORD.relative_to(base.ROOT).as_posix(),
              "path": path.relative_to(base.ROOT).as_posix(), "sha256": hashlib.sha256(encoded).hexdigest(),
              "materials": materials, "bulk_frame0_pose_count": len(sprites), "empty_frame0_alias_count": len(aliases),
              "staged_body_matrix_pose_count": len(present), "required_body_matrix_pose_count": len(required),
              "missing_pose_count": len(required - present), "missing": [list(identity) for identity in sorted(required - present)],
              "source_coverage_complete": False, "packed": False, "runtime_activated": False,
              "sprites": sprites, "empty_aliases": aliases}
    files[path] = encoded
    files[OUTPUT / "vehicle-model-05-bulk-source-manifest.json"] = (json.dumps(report, indent=2) + "\n").encode()
    comparison = Image.new("RGB", (1048, 440), (42, 46, 50))
    labels = ImageDraw.Draw(comparison)
    labels.text((12, 12), "SOURCE MATERIAL REVIEW: full N/E at native4x; frame0 only; other cargo/status/frame/purchase/runtime pending", fill="white")
    # End-on and side-on holds expose different native sample sizes. Comparing
    # every material in both catches palette collapse hidden by a large original.
    for index, row in enumerate(rows):
        left, top = 12 + index % 4 * 256, 42 + index // 4 * 196
        labels.text((left, top), row["cargo_label"] + "  N / E", fill="white")
        for direction, dx, dy in (("N", 12, 28), ("E", 100, 76)):
            pose = next(item for item in sprites if item["cargo_label"] == row["cargo_label"] and item["state"] == "full" and item["direction"] == direction)
            x, y, width, height = pose["box"]
            native = atlas.crop((x, y, x + width, y + height))
            subject = native.crop(native.getbbox()).convert("RGBA")
            large = subject.resize((subject.width * 4, subject.height * 4), Image.Resampling.NEAREST)
            comparison.paste(large, (left + dx, top + dy), large)
    comparison_bytes = io.BytesIO()
    comparison.save(comparison_bytes, format="PNG", optimize=False, compress_level=9)
    files[OUTPUT / "vehicle-model-05-bulk-materials-contact.png"] = comparison_bytes.getvalue()
    # A separate complete partial/full board per material makes crop/identity
    # errors visible at1x/4x without confusing source evidence with a game view.
    for row in rows:
        label = row["cargo_label"]
        board = Image.new("RGB", (1048, 1120), (42, 46, 50))
        for level_index, load in enumerate(("partial", "full")):
            selected = [pose for pose in sprites if pose["cargo_label"] == label and pose["state"] == load]
            page = base_contact(atlas, selected, "vehicle.model-05 " + label + " " + load,
                                caption="SOURCE LOAD ONLY: " + label + " " + load + "; frame0; native1x/nearest4x; other frames/status/purchase/runtime pending")
            board.paste(page, (0, 560 * level_index))
        buffer = io.BytesIO()
        board.save(buffer, format="PNG", optimize=False, compress_level=9)
        files[OUTPUT / ("vehicle-model-05-" + label.lower() + "-contact.png")] = buffer.getvalue()
    return report, files


def main():
    """Publish or check the bounded source base artifacts without touching runtime.

    @return None; fail on stale originals, frozen geometry or review artifacts.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--publish-base", action="store_true", help="write reproducible source base review only")
    parser.add_argument("--cargo-guide", action="store_true", help="write the approved load-geometry guide only")
    parser.add_argument("--publish-bulk", action="store_true", help="write staged original bulk load sources only")
    parser.add_argument("--check-bulk", action="store_true", help="verify exact staged bulk artifacts without writes")
    args = parser.parse_args()
    if args.cargo_guide:
        path = base.ROOT / "media/multilayer/guides/ground/mineral-wagon-cargo-apertures-v1.png"
        path.parent.mkdir(parents=True, exist_ok=True)
        cargo_geometry_guide().save(path)
        print(path.relative_to(base.ROOT).as_posix())
        return
    # Explicit bulk selection checks its staged matrix while the default remains
    # the bounded original base audit; neither route changes public source status.
    bulk = args.publish_bulk or args.check_bulk
    report, files = build_bulk() if bulk else build_base()
    # Publication is confined to ignored art proof output. A normal verification
    # compares exact bytes and cannot overwrite the public manifest or payload.
    for path, encoded in files.items():
        if args.publish_base or args.publish_bulk:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(encoded)
        if path.read_bytes() != encoded:
            raise ValueError("wagon base artifact differs from retained originals")
    if bulk:
        print(f"Wagon bulk: {report['bulk_frame0_pose_count']} real load poses, {report['empty_frame0_alias_count']} explicit empty aliases; {report['missing_pose_count']} remaining source poses; runtime pending")
    else:
        print(f"Wagon base: {report['body_source_pose_count']}/8 genuine directions; exact source artifacts; full states/purchase/runtime pending")


if __name__ == "__main__":
    main()
