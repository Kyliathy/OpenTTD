"""Place genuine oriented cargo objects in the selected conventional wagon.

@file convert_ground_freight_objects.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group20D
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
Whole original containers/ingots/modules retain their real headings and alpha.
Native packet owners alone classify load; no new inventory or runtime binding.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageChops, ImageDraw

import convert_ground_fleet_art as fleet
from convert_terrain_art import png_bytes
from tools.ground_fleet_contract import native_cargo_forms


TUBE_RECORD = fleet.base.ROOT / "media/multilayer/source/ground/tube-generation-record.json"


def slot_signature(family):
    """Freeze the two cargo banks against actual approved wagon anatomy.

    @param family Retained model05 family containing body/hold registrations.
    @return SHA256 of bank/depth geometry and the existing body/cargo signature.
    """
    fields = {"slots": family["oriented_slot_geometry"],
              "body_cargo_geometry_signature": family["approved_cargo_geometry_sha256"]}
    return hashlib.sha256(json.dumps(fields, sort_keys=True).encode()).hexdigest()


def component_signature(registration, family):
    """Bind complete original objects to their reviewed native positions.

    @param registration Actual form, aliases, heading crops and native placements.
    @param family Retained model05 body/cargo and bank geometry owner.
    @return SHA256 independent of review and publication workflow flags.
    """
    fields = {"registration": registration,
              "body_cargo_geometry_signature": family["approved_cargo_geometry_sha256"],
              "slots": family["oriented_slot_geometry"]}
    return hashlib.sha256(json.dumps(fields, sort_keys=True).encode()).hexdigest()


def component_spec(name, document):
    """Require genuine oriented forms and a complete approved eight-view registration.

    @param name Explicit protected-package, station-module or metal-ingot identity.
    @param document Retained source family and original provenance.
    @return Registered object source with immutable form/heading/placement data.
    """
    family = document["families"]["vehicle.model-05"]
    if family["approved_cargo_geometry_sha256"] != fleet.cargo_signature(family):
        raise ValueError("oriented cargo body/hold geometry changed")
    if family["approved_oriented_slot_sha256"] != slot_signature(family):
        raise ValueError("oriented cargo bank/depth geometry changed")
    if name not in family["oriented_components"]:
        raise ValueError("unknown oriented wagon component")
    component = family["oriented_components"][name]
    registration = component["registration"]
    if not component["status"].startswith("selected_"):
        raise ValueError("unselected oriented wagon component")
    if component_signature(registration, family) != family["approved_oriented_components"].get(name):
        raise ValueError("oriented cargo registration changed after approval")
    # Covered contents require a real closed package; ingots and prefabricated
    # modules have their own solid object geometry. Grain textures cannot stand
    # in for those shapes or silently reinterpret an actual native cargo form.
    kinds = {"protected_package": {"covered_granules", "packaged"},
             "station_module": {"packaged"}, "metal_ingot": {"ingots"}}
    kind = registration["component_kind"]
    if kind not in kinds:
        raise ValueError("nonoriented cargo cannot supply an object refit")
    if set(registration["allowed_native_forms"]) != kinds[kind]:
        raise ValueError("oriented wagon component physical forms changed")
    forms = native_cargo_forms()
    labels = registration["cargo_labels"]
    # Every declared alias remains an actual independent native cargo label.
    # A shared protected exterior is permitted; a new cargo or duplicate source
    # meaning cannot inherit that presentation just because its name is plausible.
    if not labels or len(labels) != len(set(labels)):
        raise ValueError("empty or ambiguous oriented cargo aliases")
    if any(forms.get(label) not in kinds[kind] for label in labels):
        raise ValueError("oriented component differs from actual native cargo form")
    if set(registration["views"]) != set(fleet.DIRECTIONS):
        raise ValueError("oriented component lacks a genuine source heading")
    return registration


def original_candidate(view, document, originals):
    """Resolve one exact repository-owned original without latest-file discovery.

    @param view Registered source record/key/path/hash for one actual heading.
    @param document Current isolated fleet source record for same-record originals.
    @param originals Read-only external original records cached by repository path.
    @return Actual selected retained candidate whose ancestry still hashes exactly.
    """
    own = fleet.RECORD.relative_to(fleet.base.ROOT).as_posix()
    tube = TUBE_RECORD.relative_to(fleet.base.ROOT).as_posix()
    source_record = view["source_record"]
    # Only the existing body/material and completed tube-source owners may
    # provide these selected originals. This cannot open private user caches or
    # invent another registry to bypass missing source/heading approval.
    if source_record not in (own, tube):
        raise ValueError("foreign oriented wagon source record")
    record = document if source_record == own else originals[source_record]
    matches = [row for row in record["candidates"] if row["key"] == view["source_key"]]
    if len(matches) != 1:
        raise ValueError("missing or ambiguous original cargo object")
    source = matches[0]
    if not source["status"].startswith("selected"):
        raise ValueError("rejected or unselected original cargo object")
    # The selected registration names one actual original, never a filename
    # alias that could replace the approved ingot/container geometry.
    if source["path"] != view["source_path"] or source["sha256"] != view["source_sha256"]:
        raise ValueError("oriented cargo source identity changed")
    # All selected originals/references stay repository-owned. Cache paths are
    # provenance only and cannot become an alternate source lookup authority.
    for identity in [source] + source["references"]:
        if fleet.base.digest(fleet.base.repository_path(identity["path"])) != identity["sha256"]:
            raise ValueError("immutable oriented cargo original/reference changed")
    return source


def native_objects(name, direction, document, originals):
    """Preserve a whole original object's shape in both fixed native cargo banks.

    @param name Explicit registered physical cargo component.
    @param direction Genuine engine heading belonging to this source view.
    @param document Retained body/object source record.
    @param originals Exact external original records, not copied game state.
    @return Two indexed64x48 whole-object canvases and immutable source evidence.
    """
    registration = component_spec(name, document)
    if direction not in registration["views"]:
        raise ValueError("oriented cargo original has no requested heading")
    view = registration["views"][direction]
    source = original_candidate(view, document, originals)
    kind = registration["component_kind"]
    # Completed source records own observed headings, including the corrected
    # neutral heavy module's bottom-row order. Whole-object reuse may expand an
    # old lid crop, but must contain its owned heading and no other view's crop.
    parent_views = source.get("object_view_windows", source.get("component_views", {}))
    if direction not in parent_views:
        raise ValueError("oriented cargo heading is absent from its original owner")
    expected_kind = {"protected_package": "sealed_package_load", "station_module": "heavy_pallet_load"}
    if kind in expected_kind:
        if source.get("component") != expected_kind[kind]:
            raise ValueError("oriented cargo borrowed another physical original")
    elif source.get("component_kind") != "oriented_metal_ingot":
        raise ValueError("ingot refit borrowed a plate or container original")
    # Complete component shapes retain the same native canvas as their wagon;
    # modern originals use alpha16/16px gutters. The retained preframed station
    # module's measured legacy clearance is guarded by its registered8px minimum.
    required_clearance = 8 if kind == "station_module" else 16
    if (view["native_canvas"] != [64, 48] or view["source_alpha_threshold"] != 16
            or view["minimum_source_clearance"] < required_clearance):
        raise ValueError("whole cargo source/native geometry policy changed")
    original = Image.open(fleet.base.repository_path(source["path"])).convert("RGBA")
    left, top, right, bottom = view["source_window"]
    if not (0 <= left < right <= original.width and 0 <= top < bottom <= original.height):
        # An out-of-image crop can truncate a pallet/strap and fake a complete
        # object before it is hidden behind the ordinary wagon's foreground.
        raise ValueError("whole cargo window leaves its immutable original")
    x1, y1, x2, y2 = parent_views[direction]
    if not (left <= x1 < x2 <= right and top <= y1 < y2 <= bottom):
        raise ValueError("whole cargo window differs from its actual source heading")
    for other, (x1, y1, x2, y2) in parent_views.items():
        if other == direction:
            continue
        if left < x2 and right > x1 and top < y2 and bottom > y1:
            # A larger whole-object rectangle cannot count a neighbouring
            # payload or its lifting lug as part of this heading's approved model.
            raise ValueError("whole cargo window crosses another source heading")
    whole = original.crop((left, top, right, bottom))
    # Alpha16 includes faint model edges before DOS/native alpha is sampled.
    # Retained preframed modules keep their measured legacy clearance; no
    # approved original is regenerated merely to add modern source frames.
    bounds = whole.getchannel("A").point(lambda alpha: 255 if alpha >= 16 else 0).getbbox()
    if bounds is None:
        raise ValueError("empty whole cargo original")
    if list(bounds) != view["whole_subject_bounds_in_window"]:
        raise ValueError("whole cargo bounds differ from their approved registration")
    clearance = min(bounds[0], bounds[1], whole.width - bounds[2], whole.height - bounds[3])
    if clearance < view["minimum_source_clearance"]:
        raise ValueError("whole cargo original clips its source clearance")
    if any(alpha >= 16 and red > 220 and green < 60 and blue > 220
           for red, green, blue, alpha in whole.get_flattened_data()):
        # Layout separators are never cargo. Refuse their source window without
        # erasing colors, trimming an object or replacing missing pixels.
        raise ValueError("opaque source separator enters whole cargo original")
    subject = whole.crop(bounds)
    slots = document["families"]["vehicle.model-05"]["oriented_slot_geometry"]["bank_boxes"][direction]
    canvases = []
    # Both native banks resample the same complete genuine object. Separate
    # actual depth/occupancy determines partial/full; no flat image rotation,
    # invented volume counter or donor wagon geometry is introduced.
    for index, (x, y, width, height) in enumerate(slots):
        scale = min(width / subject.width, height / subject.height)
        size = [max(1, round(subject.width * scale)), max(1, round(subject.height * scale))]
        position = [x + (width - size[0]) // 2, y + height - size[1]]
        # A later offset/scale change would move cargo away from its actual
        # support bank or across the wagon's fixed rim/contact hardware.
        if size != view["native_sizes"][index] or position != view["native_positions"][index]:
            raise ValueError("whole cargo native projection moved after approval")
        canvas = Image.new("RGBA", (64, 48))
        canvas.paste(subject.resize(tuple(size), Image.Resampling.BOX), tuple(position))
        native = fleet.base.quantize(canvas, {})
        native.info["transparency"] = 0
        canvases.append(native)
    evidence = {"component": name, "component_kind": kind, "direction": direction,
                "source_record": view["source_record"], "source_key": source["key"],
                "source_path": source["path"], "source_sha256": source["sha256"],
                "source_window": view["source_window"], "whole_subject_bounds": list(bounds),
                "source_clearance": clearance, "native_sizes": view["native_sizes"],
                "native_positions": view["native_positions"]}
    return canvases, evidence


def foreground_mask(body, direction):
    """Retain the real original front steel while oriented cargo sits behind it.

    @param body Approved native indexed empty body for this same actual heading.
    @param direction Source engine heading owning its raised load volume and rim.
    @return Original opaque foreground mask and approved visible load envelope.
    """
    geometry = fleet.cargo_geometry()
    allowed = Image.new("L", (64, 48), 0)
    ImageDraw.Draw(allowed).polygon([tuple(point) for point in geometry["protected_load_envelopes"][direction]], fill=255)
    rim = Image.new("L", (64, 48), 0)
    draw = ImageDraw.Draw(rim)
    # Full original side/end walls outside the raised aperture occlude a whole
    # object's low corners. The retained near rim also occludes within it; only
    # existing opaque steel participates, never an invented erasing stencil.
    for line in geometry["foreground_rim_lines"][direction]:
        draw.line([tuple(point) for point in line], fill=255, width=1)
    original_alpha = body.point([0] + [255] * 255, "L")
    mask = ImageChops.multiply(original_alpha, ImageChops.lighter(ImageChops.invert(allowed), rim))
    return mask, allowed


def compose_objects(body, objects, direction, load, family):
    """Render actual whole payload banks far-to-near behind unchanged front steel.

    @param body Exact approved native frame-zero body for this genuine heading.
    @param objects Two indexed complete-source object canvases in fixed bank order.
    @param direction Source/native heading owning both physical banks.
    @param load Partial or full packet-backed visual category.
    @param family Retained model05 bank geometry and approved body owner.
    @return Indexed native load pose; visible cargo extends only inside its envelope.
    """
    if load not in ("partial", "full"):
        raise ValueError("unknown oriented wagon fill category")
    if len(objects) != 2:
        raise ValueError("oriented wagon source requires both real bank projections")
    geometry = family["oriented_slot_geometry"]
    selected = geometry["back_to_front"] if load == "full" else [geometry["partial_bank_indices"][direction]]
    foreground, allowed = foreground_mask(body, direction)
    original_alpha = body.point([0] + [255] * 255, "L")
    image = body.copy()
    # The actual B-end bank supplies a partial load. Full adds the other object
    # in physical depth order, so a near package may naturally obscure a far
    # package; this is original object/steel occlusion, not copied cargo stock.
    for index in selected:
        pixels = objects[index]
        alpha = pixels.point([0] + [255] * 255, "L")
        unexpected = ImageChops.multiply(alpha, ImageChops.multiply(ImageChops.invert(allowed), ImageChops.invert(original_alpha)))
        if unexpected.getbbox() is not None:
            raise ValueError("whole cargo projects outside its approved envelope into background")
        image.paste(pixels, (0, 0), alpha)
    image.paste(body, (0, 0), foreground)
    if image.tobytes() == body.tobytes():
        raise ValueError("oriented wagon load disappeared at native scale")
    return image


def build(document=None, public=None, originals=None):
    """Publish only genuine selected object poses and expose the remaining matrix.

    @param document Optional isolated current fleet source record.
    @param public Optional isolated shared native sprite contract.
    @param originals Optional external original-record dictionary for refusal checks.
    @return Staged combined coverage report and exact source/review artifact bytes.
    """
    if document is None:
        document = json.loads(fleet.RECORD.read_text(encoding="utf-8"))
    if originals is None:
        originals = {TUBE_RECORD.relative_to(fleet.base.ROOT).as_posix(): json.loads(TUBE_RECORD.read_text(encoding="utf-8"))}
    family = document["families"]["vehicle.model-05"]
    spec = fleet.public_family("vehicle.model-05", public)
    bulk, files = fleet.build_bulk(document=document, public=public)
    base_report = json.loads(files[fleet.OUTPUT / "vehicle-model-05-base-source-manifest.json"])
    base_atlas = Image.open(io.BytesIO(files[fleet.base.ROOT / base_report["path"]]))
    bodies = {}
    # Every payload reuses the same actual regenerated body, preserving its
    # fixed eight-way couplers, wheel contacts and original source ancestry.
    for row in base_report["sprites"]:
        x, y, width, height = row["box"]
        bodies[row["direction"]] = base_atlas.crop((x, y, x + width, y + height))
    bindings, objects = {}, {}
    for name in family["oriented_components"]:
        registration = component_spec(name, document)
        # One physical exterior may explicitly serve several independent cargo
        # meanings; no cargo may silently choose between two competing sources.
        for label in registration["cargo_labels"]:
            if label in bindings:
                raise ValueError("ambiguous oriented cargo source alias")
            bindings[label] = name
        for direction in fleet.DIRECTIONS:
            objects[name, direction] = native_objects(name, direction, document, originals)
    rows = [row for row in family["cargo_source_matrix"] if row["cargo_label"] in bindings]
    if not rows:
        raise ValueError("oriented cargo handoff has no selected actual refits")
    atlas = Image.new("P", (8 * 64, len(rows) * 2 * 48), 0)
    atlas.putpalette(fleet.base.PALETTE_BYTES)
    atlas.info["transparency"] = 0
    sprites = []
    # Actual cargo labels retain distinct variant slots even where their real
    # protected exterior is shared. Whole objects supply both frame0 load levels;
    # missing animation/status/purchase never inherits a duplicate frame.
    for row_index, row in enumerate(rows):
        label, name = row["cargo_label"], bindings[row["cargo_label"]]
        if row["form"] != native_cargo_forms()[label]:
            raise ValueError("oriented refit form differs from the native content owner")
        for load_index, load in enumerate(("partial", "full")):
            if (row.get(load + "_source") or {}).get("component") != name:
                raise ValueError("oriented fill source pointer differs from selected component")
            for direction_index, direction in enumerate(fleet.DIRECTIONS):
                canvases, evidence = objects[name, direction]
                image = compose_objects(bodies[direction], canvases, direction, load, family)
                x, y = 64 * direction_index, 48 * (row_index * 2 + load_index)
                atlas.paste(image, (x, y))
                sprites.append({"cargo_label": label, "tile_variant": row["variant"], "state": load,
                                "frame": 0, "direction": direction, "part": "body", "box": [x, y, 64, 48],
                                "anchor": [-32, -32], "component": evidence,
                                "body_atlas": base_report["path"], "body_sha256": base_report["sha256"]})
    # Combine explicit real source identities across both bounded authoring
    # groups; the complete1360-pose matrix still exposes every absent gear/status
    # phase. No file glob or stale staged count grants complete family delivery.
    present = {(row["state"], row["frame"], row["direction"], row["tile_variant"])
               for row in sprites + bulk["sprites"] + bulk["empty_aliases"]}
    required = {(state, frame, direction, variant) for state in spec["states"] for frame in range(spec["frames"])
                for direction in fleet.DIRECTIONS for variant in range(spec["tile_variants"])}
    path = fleet.OUTPUT / "vehicle-model-05-objects-authored.png"
    encoded = png_bytes(atlas)
    report = {"_documentation": "Owner: r2mlvs14-remaining-assets-and-runtime-integration.md Group20D; whole objects/frame0 only",
              "schema_version": 1, "asset_id": "vehicle.model-05", "path": path.relative_to(fleet.base.ROOT).as_posix(),
              "sha256": hashlib.sha256(encoded).hexdigest(), "source_record": fleet.RECORD.relative_to(fleet.base.ROOT).as_posix(),
              "component_aliases": bindings, "oriented_frame0_pose_count": len(sprites),
              "bulk_frame0_pose_count": bulk["bulk_frame0_pose_count"], "empty_frame0_alias_count": bulk["empty_frame0_alias_count"],
              "staged_body_matrix_pose_count": len(present), "required_body_matrix_pose_count": len(required),
              "missing_pose_count": len(required - present), "missing": [list(identity) for identity in sorted(required - present)],
              "source_coverage_complete": False, "packed": False, "runtime_activated": False, "sprites": sprites,
              "bulk_source_manifest": "logs/proofs/mlm/native-art/ground/vehicle-model-05-bulk-source-manifest.json"}
    files[path] = encoded
    files[fleet.OUTPUT / "vehicle-model-05-objects-source-manifest.json"] = (json.dumps(report, indent=2) + "\n").encode()
    # Per-label contacts retain explicit cargo identities despite approved
    # exterior aliases, and show all genuine headings at1x/4x for both levels.
    for row in rows:
        label = row["cargo_label"]
        board = Image.new("RGB", (1048, 1120), (42, 46, 50))
        for load_index, load in enumerate(("partial", "full")):
            selected = [pose for pose in sprites if pose["cargo_label"] == label and pose["state"] == load]
            page = fleet.base_contact(atlas, selected, "vehicle.model-05 " + label + " " + load,
                                      caption="SOURCE OBJECT ONLY: " + label + " " + load + "; frame0; native1x/4x; moving/status/purchase/runtime pending")
            board.paste(page, (0, 560 * load_index))
        buffer = io.BytesIO()
        board.save(buffer, format="PNG", optimize=False, compress_level=9)
        files[fleet.OUTPUT / ("vehicle-model-05-" + label.lower() + "-contact.png")] = buffer.getvalue()
    return report, files


def main():
    """Publish or verify the bounded whole-object source slice without activation.

    @return None; fail on altered originals, forms, headings, projection or artifacts.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--publish", action="store_true", help="write genuine staged object/base/bulk source proofs only")
    args = parser.parse_args()
    report, files = build()
    # Artifact writes stay inside the repository's ignored proof cache; this
    # command cannot edit a public source status, native range or distributed GRF.
    for path, encoded in files.items():
        if args.publish:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(encoded)
        if path.read_bytes() != encoded:
            raise ValueError("oriented wagon artifact differs from retained originals")
    print(f"Wagon objects: {report['oriented_frame0_pose_count']} genuine load poses; {report['staged_body_matrix_pose_count']}/1360 staged, {report['missing_pose_count']} missing; runtime pending")


if __name__ == "__main__":
    main()
