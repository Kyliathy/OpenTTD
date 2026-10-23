"""Assemble model07 road motion, empty-status and own purchase source poses.

@file convert_mine_truck_states.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group21C
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group22 shared original road-hub/glass material
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
Native road head movement, depot admission and cargo packets own presentation.
No source phase grants traffic clearance, cargo custody or a packed range.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_ground_wagon_states as ordinary
import convert_mine_truck_art as bulk
from convert_terrain_art import png_bytes


fleet = bulk.fleet
REPORT = fleet.OUTPUT / "vehicle-model-07-source-manifest.json"


def family_spec(document, public):
    """Require own-road attachments, physical occlusion and shared original owners.

    @param document Retained fleet selections or isolated refusal fixture.
    @param public Shared native body/purchase specification.
    @return Own07 geometry/components and approved shared05 original registrations.
    """
    family, spec = bulk.checked_family(document, public)
    geometry, components = family["state_component_geometry"], family["state_components"]
    donor_geometry, donor_components = ordinary.checked_geometry(document, public)
    donor = document["families"]["vehicle.model-05"]
    # The assembly loop must cover the same actual native labels as the public
    # consumer; a shortened private family cannot claim complete source delivery.
    if any(family.get(key) != value for key, value in bulk.variant_contract(7).items()):
        raise ValueError("mine truck retained native cargo/state matrix changed")
    # Original cab/bed/tyre geometry and actual road controller facts govern
    # every attachment; a train flag or plausible foreign shell cannot replace it.
    if (ordinary.signature(geometry) != family["approved_state_component_geometry_sha256"]
            or ordinary.signature(components) != family["approved_state_components_sha256"]
            or components["geometry_sha256"] != ordinary.signature(geometry)
            or geometry["body_registrations"] != family["approved_base_registration"]
            or geometry["state_owner"] != spec["native_state_policy"]
            or geometry["native_canvas"] != [64, 48] or geometry["anchor"] != [-32, -32]
            or spec["company_colors"] != "explicit_mask"):
        raise ValueError("mine truck component geometry or road ownership changed")
    source_record = fleet.RECORD.relative_to(fleet.base.ROOT).as_posix()
    # Both reusable materials have a named approved original owner. A latest
    # file, same-colored body pixel or complete donor wheel is not a component.
    for name in ("gear_source", "status_source"):
        source = components[name]
        if (source["source_record"] != source_record or source["source_family"] != "vehicle.model-05"
                or source["source_components_sha256"] != donor["approved_state_components_sha256"]):
            raise ValueError("foreign mine truck shared original component owner")
    status = components["status_source"]
    if status["source_key"] != "wagon-status-lenses-v2" or not status["material_only"]:
        # Glass interiors can be reused; another vehicle's lamp housing cannot.
        raise ValueError("mine truck status is not approved original glass")
    required = set(fleet.DIRECTIONS)
    # Every actual camera needs explicit attachments and visibility, including
    # the physically hidden end-on hub faces; absence is not an occlusion alias.
    for mapping in (geometry["gear_regions"], geometry["status_regions"], geometry["company_regions"],
                    geometry["gear_visibility"], components["gear_source"]["source_points"]):
        if set(mapping) != required:
            raise ValueError("mine truck component is missing a genuine heading")
    for direction in fleet.DIRECTIONS:
        hidden = direction in ("N", "S")
        expected = "hidden_end_elevation" if hidden else "three_visible_road_hub_interiors"
        targets = geometry["gear_regions"][direction]
        sources = components["gear_source"]["source_points"][direction]
        if geometry["gear_visibility"][direction] != expected or len(targets) != (0 if hidden else 3) or len(sources) != len(targets):
            # A false hidden flag must not conceal an absent side-on rolling
            # original; end elevations must not invent visible side wheel hubs.
            raise ValueError("mine truck road hub visibility or source count changed")
    return geometry, components, donor_geometry, donor_components


def component_sources(document, geometry, components, donor_geometry, donor_components):
    """Retarget only genuine neutral hub material and original status-glass pixels.

    @param document Immutable selected original source record.
    @param geometry Inspected own07/08 hub/roof/door regions and visibility.
    @param components Explicit shared material and own purchase selections.
    @param donor_geometry Approved05 original wheel-interior extraction regions.
    @param donor_components Approved05 genuine camera/glass source registrations.
    @return Positioned road hub material and four original status lens samples.
    """
    donors, lenses = ordinary.state_sources(document, donor_geometry, donor_components)
    gears = {}
    # Six genuine headings contribute neutral metal interiors only. Retargeted
    # one-pixel road hubs preserve the own truck's three rubber tyres and axles;
    # no rail wheel silhouette, bogie, alpha, cargo or contact point is copied.
    for direction in fleet.DIRECTIONS:
        if geometry["gear_visibility"][direction] == "hidden_end_elevation":
            gears[direction] = None, {"occlusion": "actual end elevation hides all hub faces",
                                      "body_registration_sha256": geometry["body_registrations"][direction]}
            continue
        donor, evidence = donors[direction]
        points = components["gear_source"]["source_points"][direction]
        targets = geometry["gear_regions"][direction]
        allowed = {(px, py) for x, y, width, height in donor_geometry["gear_regions"][direction]
                   for py in range(y, y + height) for px in range(x, x + width)}
        native = Image.new("P", (64, 48), 0)
        native.putpalette(fleet.base.PALETTE_BYTES)
        if len({tuple(region) for region in targets}) != 3:
            raise ValueError("mine truck needs three independent actual hub targets")
        for point, target in zip(points, targets):
            x, y, width, height = target
            if tuple(point) not in allowed or width != 1 or height != 1 or y >= 39:
                # Source and target must be wheel interiors above ground contact;
                # a plausible roof pixel or enlarged region cannot become motion.
                raise ValueError("mine truck original hub material leaves approved region")
            selected = donor.getpixel(tuple(point))
            rgb = fleet.base.PALETTE[selected]
            if selected == 0 or max(rgb) - min(rgb) > 20:
                # Neutral material is reusable here; livery or tinted shell
                # would import another model's surface instead of hub metal.
                raise ValueError("mine truck hub donor is not opaque neutral metal")
            native.putpixel((x, y), selected)
        gears[direction] = native, {**evidence, "source_family": "vehicle.model-05", "material_only": True,
                                    "source_points": points, "target_regions": targets}
    return gears, lenses


def apply_components(body, direction, state, frame, geometry, gears, lenses):
    """Apply road parts while preserving the observed end-view hub occlusion.

    @param body Actual own07/08 empty/load pose with its fixed steel and tyres.
    @param direction Genuine native truck heading.
    @param state Actual packet-backed load or empty road service/traffic hold.
    @param frame Native owner-selected zero/one visual phase.
    @param geometry Frozen own07/08 road attachment and physical visibility contract.
    @param gears Genuine neutral hub materials or explicit end-view occlusion.
    @param lenses Four separately authored original service/blocked glass samples.
    @return Indexed own-road pose and truthful material/occlusion evidence.
    """
    # The actual axle hub faces disappear end-on. Keep both real frame slots
    # but alias their occluded appearance, rather than draw a fictitious side
    # wheel or treat a missing side-on source as a hidden component.
    if direction in ("N", "S") and state in ("empty", "partial", "full") and frame == 1:
        image, evidence = ordinary.apply_components(body, direction, state, 0, geometry, gears, lenses)
        evidence["gear"] = gears[direction][1]
        return image, evidence
    return ordinary.apply_components(body, direction, state, frame, geometry, gears, lenses)


def build(document=None, public=None):
    """Assemble all800 road body and two own purchase poses from verified originals.

    @param document Optional isolated source selections; defaults to retained inputs.
    @param public Optional isolated native body/purchase contract.
    @return Complete source report and exact artifact bytes, without activation.
    """
    if document is None:
        document = json.loads(fleet.RECORD.read_text(encoding="utf-8"))
    if public is None:
        public = json.loads(fleet.base.PUBLIC.read_text(encoding="utf-8"))
    geometry, components, donor_geometry, donor_components = family_spec(document, public)
    gears, lenses = component_sources(document, geometry, components, donor_geometry, donor_components)
    cargo_report, files = bulk.build(document, public)
    base_report = json.loads(files[fleet.OUTPUT / "vehicle-model-07-base-source-manifest.json"])
    bodies, loads = {}, {}
    # Regenerated whole own-road bodies and exact original cargo hold poses are
    # the only starting pixels for states; stale cached atlases cannot substitute.
    for report, destination in ((base_report, bodies), (cargo_report, loads)):
        atlas = Image.open(io.BytesIO(files[fleet.base.ROOT / report["path"]]))
        for row in report["sprites"]:
            x, y, width, height = row["box"]
            key = row["direction"] if destination is bodies else (row["cargo_label"], row["state"], row["direction"])
            destination[key] = atlas.crop((x, y, x + width, y + height)), row
    family = document["families"][bulk.ASSET]
    atlas = Image.new("P", (512, len(family["native_cargo_labels"]) * len(ordinary.STATES) * 2 * 48), 0)
    atlas.putpalette(fleet.base.PALETTE_BYTES)
    atlas.info["transparency"] = 0
    sprites = []
    # Native cargo labels keep separate slots. Only packet-backed load states
    # carry material; empty service/traffic status retains stationary tyres.
    for variant, label in enumerate(family["native_cargo_labels"]):
        for state_index, state in enumerate(ordinary.STATES):
            for frame in range(2):
                for direction_index, direction in enumerate(fleet.DIRECTIONS):
                    body, ancestry = loads[label, state, direction] if state in ("partial", "full") else bodies[direction]
                    image, evidence = apply_components(body, direction, state, frame, geometry, gears, lenses)
                    x, y = direction_index * 64, ((variant * len(ordinary.STATES) + state_index) * 2 + frame) * 48
                    atlas.paste(image, (x, y))
                    sprites.append({"state": state, "frame": frame, "direction": direction, "tile_variant": variant,
                                    "cargo_label": label, "part": "body", "anchor": [-32, -32], "box": [x, y, 64, 48],
                                    "body_source": bodies[direction][1], "load_source": ancestry if state in ("partial", "full") else None,
                                    "components": evidence, "visual_alias": None if state in ("partial", "full") else "cargo-independent empty " + state})
    purchase, purchase_rows = ordinary.purchase_source(bodies["E"][0], components, public, "purchase.model-07")
    for row in purchase_rows:
        row["body_source"] = bodies["E"][1]
    atlases = []
    # World and purchase retain independent native canvases and counts, sharing
    # only this truck's whole original E preview and genuine calendar badge.
    for asset_id, name, image, rows in ((bulk.ASSET, "vehicle-model-07-authored.png", atlas, sprites),
                                        ("purchase.model-07", "purchase-model-07-authored.png", purchase, purchase_rows)):
        path = fleet.OUTPUT / name
        encoded = png_bytes(image)
        files[path] = encoded
        atlases.append({"asset_id": asset_id, "path": path.relative_to(fleet.base.ROOT).as_posix(),
                        "sha256": hashlib.sha256(encoded).hexdigest(), "size": list(image.size), "sprites": rows})
    # Representative actual Coal phase pages expose every heading and state;
    # all ten earlier material pages retain each refit's unchanged cargo pixels.
    for state in ordinary.STATES:
        board = Image.new("RGB", (1048, 1120), (42, 46, 50))
        for frame in range(2):
            rows = [row for row in sprites if row["cargo_label"] == "COAL" and row["state"] == state and row["frame"] == frame]
            caption = "ROAD MODEL07 SOURCE: " + state + " phase" + str(frame) + "; native1x/4x; actual RoadVehicle owns selection"
            board.paste(fleet.base_contact(atlas, rows, bulk.ASSET, caption), (0, frame * 560))
        files[fleet.OUTPUT / ("vehicle-model-07-" + state + "-phases-contact.png")] = bulk.review_bytes(board)
    board = Image.new("RGB", (360, 160), (42, 46, 50))
    draw = ImageDraw.Draw(board)
    # Both genuine purchase states remain visible at native1x/4x; the lock badge
    # never changes the actual model/date/capability admission owner.
    for index, row in enumerate(purchase_rows):
        x, y, width, height = row["box"]
        image = purchase.crop((x, y, x + width, y + height)).convert("RGBA")
        draw.text((12 + index * 180, 12), row["state"] + " native1x/4x", fill="white")
        board.paste(image, (20 + index * 180, 40), image)
        large = image.resize((80, 80), Image.Resampling.NEAREST)
        board.paste(large, (70 + index * 180, 40), large)
    files[fleet.OUTPUT / "purchase-model-07-contact.png"] = bulk.review_bytes(board)
    report = {"_documentation": "Owner: r2mlvs14-remaining-assets-and-runtime-integration.md Group21C; complete road sources only",
              "schema_version": 1, "source_record": fleet.RECORD.relative_to(fleet.base.ROOT).as_posix(),
              "body_pose_count": len(sprites), "purchase_pose_count": len(purchase_rows),
              "native_cargo_forms": fleet.native_cargo_forms(7),
              "geometry_sha256": ordinary.signature(geometry), "components_sha256": ordinary.signature(components),
              "source_coverage_complete": True, "packed": False, "runtime_activated": False, "atlases": atlases}
    files[REPORT] = (json.dumps(report, indent=2) + "\n").encode()
    return report, files


def publish_source(document, public, report, files):
    """Publish complete verified07 sources through the existing public owner.

    @param document Retained own-model originals and explicit shared materials.
    @param public Current shared manifest; preserve every native packed field.
    @param report Fresh complete800-body/two-purchase source metadata.
    @param files Fresh exact generated artifacts including the source report.
    @return None; refuse incomplete or substituted proof before metadata writes.
    """
    # Stale images or metadata substitutions cannot impersonate delivered art.
    # Verify every artifact and its exact report before changing either family.
    for path, encoded in files.items():
        if path.read_bytes() != encoded:
            raise ValueError("mine truck source publication requires exact artifacts")
    if report != json.loads(files[REPORT]):
        raise ValueError("mine truck publication metadata differs from verified proof")
    if report["body_pose_count"] != 800 or report["purchase_pose_count"] != 2 or not report["source_coverage_complete"]:
        # Whole source coverage closes only when both actual consumers are full.
        raise ValueError("mine truck publication requires complete source coverage")
    expected = [(bulk.ASSET, 800), ("purchase.model-07", 2)]
    if [(atlas["asset_id"], len(atlas["sprites"])) for atlas in report["atlases"]] != expected:
        raise ValueError("mine truck publication needs exact own-model consumers")
    family = document["families"][bulk.ASSET]
    donor = document["families"]["vehicle.model-05"]["state_components"]
    keys = set(family["selections"].values())
    keys.update(row["source"] for row in family["approved_bulk_materials"].values())
    keys.update(row["key"] for direction, row in donor["gear"].items() if direction not in ("N", "S"))
    keys.add(family["state_components"]["status_source"]["source_key"])
    originals = [row["path"] for row in document["candidates"] if row["key"] in keys]
    originals.append(report["atlases"][1]["sprites"][1]["lock_component"]["source"])
    originals = list(dict.fromkeys(originals))
    # Source coverage uses the canonical manifest fields. Native sprite ranges,
    # drawing, traffic, cargo and calendar capabilities remain with their owners.
    for atlas in report["atlases"]:
        spec = next(row for row in public["assets"] if row["asset_id"] == atlas["asset_id"])
        source = atlas["sprites"][0]["body_source"]
        spec["source"] = {"status": "authored", "path": source["source_path"], "sha256": source["source_sha256"],
                          "provenance": "first_party_original_model_and_component_assembly", "rights": "first_party_generated_original"}
        spec["authoring"] = {"date": "2026-10-11", "status": "complete_native_original_mine_truck_source_runtime_pending",
                             "source_record": report["source_record"], "originals": originals,
                             "atlas_manifest": REPORT.relative_to(fleet.base.ROOT).as_posix(), "native_atlas": atlas["path"],
                             "native_sha256": atlas["sha256"], "staged_pose_count": len(atlas["sprites"]),
                             "coverage_complete": True, "runtime_activated": False}
    fleet.base.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def main():
    """Check or publish exact road sources while keeping native activation pending.

    @return None; print coverage or refuse changed source/metadata/artifact proof.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--publish", action="store_true", help="write exact source proof artifacts only")
    modes.add_argument("--publish-source", action="store_true", help="publish complete verified source coverage only")
    args = parser.parse_args()
    document = json.loads(fleet.RECORD.read_text(encoding="utf-8"))
    public = json.loads(fleet.base.PUBLIC.read_text(encoding="utf-8"))
    report, files = build(document, public)
    # Only explicit repository proof artifacts are written by this branch;
    # public source publication requires exact regenerated outputs first.
    for path, encoded in files.items():
        if args.publish:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(encoded)
        if path.read_bytes() != encoded:
            raise ValueError("mine truck source proof needs exact regeneration")
    if args.publish_source:
        publish_source(document, public, report, files)
    print(f"Mine truck: {report['body_pose_count']} body + {report['purchase_pose_count']} purchase; {len(files)} exact artifacts; packing/runtime pending")


if __name__ == "__main__":
    main()
