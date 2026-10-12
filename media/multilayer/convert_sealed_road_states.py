"""Assemble original model08 sealed road body, load/status and purchase sources.

@file convert_sealed_road_states.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group22
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
Actual uranium drums and covered rare earth remain in native road cargo packets.
The unchanged closed shell and external glass cues create no inventory or route.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_ground_wagon_states as ordinary
import convert_mine_truck_states as road
import convert_sealed_wagon_states as sealed
from convert_terrain_art import png_bytes
from tools.ground_fleet_contract import variant_contract


fleet = ordinary.fleet
ASSET = "vehicle.model-08"
REPORT = fleet.OUTPUT / "vehicle-model-08-source-manifest.json"


def family_spec(document, public):
    """Require own sealed-road geometry and exact original material ancestry.

    @param document Retained body/material registrations or isolated refusal data.
    @param public Shared native road body/purchase/cargo-state specification.
    @return Own08 geometry/components and approved05 material registrations.
    """
    spec = fleet.public_family(ASSET, public)
    family = document["families"][ASSET]
    geometry, components = family["state_component_geometry"], family["state_components"]
    donor_geometry, donor_components = ordinary.checked_geometry(document, public)
    donor = document["families"]["vehicle.model-05"]
    # A plausible sealed body cannot inherit rail states, another road cargo
    # table or changed attachments. Native model08 and own originals stay owners.
    if (any(family.get(key) != value for key, value in variant_contract(8).items())
            or ordinary.signature(geometry) != family["approved_state_component_geometry_sha256"]
            or ordinary.signature(components) != family["approved_state_components_sha256"]
            or components["geometry_sha256"] != ordinary.signature(geometry)
            or geometry["body_registrations"] != family["approved_base_registration"]
            or geometry["state_owner"] != spec["native_state_policy"]
            or geometry["native_canvas"] != [64, 48] or geometry["anchor"] != [-32, -32]
            or spec["company_colors"] != "explicit_mask"):
        raise ValueError("sealed road component geometry or native ownership changed")
    own_record = fleet.RECORD.relative_to(fleet.base.ROOT).as_posix()
    # Reusable neutral metal/glass each has an explicit approved source owner;
    # matching colors never authorize a complete donor wheel, body or fitting.
    for name in ("gear_source", "status_source"):
        source = components[name]
        if (source["source_record"] != own_record or source["source_family"] != "vehicle.model-05"
                or source["source_components_sha256"] != donor["approved_state_components_sha256"]):
            raise ValueError("foreign sealed road original material owner")
    status = components["status_source"]
    if status["source_key"] != "wagon-status-lenses-v2" or not status["material_only"]:
        # The own roof module receives original glass, never another housing.
        raise ValueError("sealed road status is not approved original glass")
    required = set(fleet.DIRECTIONS)
    # Every real camera needs its own whole body attachments, including explicit
    # end-on hub occlusion. A missing side heading cannot masquerade as hidden.
    for mapping in (geometry["gear_regions"], geometry["status_regions"], geometry["company_regions"],
                    geometry["gear_visibility"], geometry["sealed_meter_points"], components["gear_source"]["source_points"]):
        if set(mapping) != required:
            raise ValueError("sealed road attachment is missing a genuine heading")
    for direction in fleet.DIRECTIONS:
        hidden = direction in ("N", "S")
        visibility = "hidden_end_elevation" if hidden else "three_visible_road_hub_interiors"
        targets = geometry["gear_regions"][direction]
        points = components["gear_source"]["source_points"][direction]
        if geometry["gear_visibility"][direction] != visibility or len(targets) != (0 if hidden else 3) or len(points) != len(targets):
            # Preserve three actual road axles without inventing end-on hubs or
            # concealing an absent visible motion source behind an alias flag.
            raise ValueError("sealed road hub visibility or source count changed")
        meter = geometry["sealed_meter_points"][direction]
        if len(meter) != 2 or len({tuple(point) for point in meter}) != 2:
            # Two independent original roof panes distinguish partial and full.
            raise ValueError("sealed road load meter needs two distinct panes")
        if geometry["status_regions"][direction] != [meter[0][0], meter[0][1], 1, 1]:
            raise ValueError("sealed road empty status leaves its original roof pane")
    return geometry, components, donor_geometry, donor_components


def build(document=None, public=None):
    """Assemble all80 closed-road body and two own purchase original poses.

    @param document Optional isolated retained selections; defaults to originals.
    @param public Optional isolated shared native body/purchase contract.
    @return Complete source manifest and exact artifact bytes, without activation.
    """
    if document is None:
        document = json.loads(fleet.RECORD.read_text(encoding="utf-8"))
    if public is None:
        public = json.loads(fleet.base.PUBLIC.read_text(encoding="utf-8"))
    geometry, components, donor_geometry, donor_components = family_spec(document, public)
    gears, lenses = road.component_sources(document, geometry, components, donor_geometry, donor_components)
    meter = sealed.load_meter_source(components["meter_source"])
    base_report, files = fleet.build_base(ASSET, document, public)
    if not base_report["base_geometry_complete"]:
        raise ValueError("sealed road source needs all eight genuine body views")
    base_atlas = Image.open(io.BytesIO(files[fleet.base.ROOT / base_report["path"]]))
    bodies = {}
    # Actual own08 originals supply every whole-body pose. Shared material
    # helpers never import model06/07 chassis, cargo, alpha or attachment geometry.
    for row in base_report["sprites"]:
        x, y, width, height = row["box"]
        bodies[row["direction"]] = base_atlas.crop((x, y, x + width, y + height)), row
    atlas = Image.new("P", (512, 480), 0)
    atlas.putpalette(fleet.base.PALETTE_BYTES)
    atlas.info["transparency"] = 0
    sprites = []
    # Both actual protected cargos share one genuinely closed exterior. Native
    # packet categories change only roof glass; road-owned motion keeps loads
    # visible and empty service/traffic cues retain stopped original tyres.
    for state_index, state in enumerate(ordinary.STATES):
        for frame in range(2):
            for direction_index, direction in enumerate(fleet.DIRECTIONS):
                body, ancestry = bodies[direction]
                load, load_evidence = sealed.closed_load(body, direction, state, geometry, meter)
                image, evidence = road.apply_components(load, direction, state, frame, geometry, gears, lenses)
                x, y = direction_index * 64, (state_index * 2 + frame) * 48
                atlas.paste(image, (x, y))
                sprites.append({"state": state, "frame": frame, "direction": direction, "tile_variant": 0,
                                "cargo_labels": ["URAN", "RARE"], "part": "body", "anchor": [-32, -32],
                                "box": [x, y, 64, 48], "body_source": ancestry, "load_source": load_evidence,
                                "components": evidence, "visual_alias": "actual URAN drums and RARE covered granules share own08 closed road exterior"})
    purchase, purchase_rows = ordinary.purchase_source(bodies["E"][0], components, public, "purchase.model-08")
    for row in purchase_rows:
        row["body_source"] = bodies["E"][1]
    atlases = []
    # Independent world/purchase canvases preserve the whole own-model truck;
    # the calendar badge is an original UI component, not an availability rule.
    for asset_id, name, image, rows in ((ASSET, "vehicle-model-08-authored.png", atlas, sprites),
                                        ("purchase.model-08", "purchase-model-08-authored.png", purchase, purchase_rows)):
        path = fleet.OUTPUT / name
        encoded = png_bytes(image)
        files[path] = encoded
        atlases.append({"asset_id": asset_id, "path": path.relative_to(fleet.base.ROOT).as_posix(),
                        "sha256": hashlib.sha256(encoded).hexdigest(), "size": list(image.size), "sprites": rows})
    # All state/frame/camera combinations remain visible at native1x/4x; no
    # closed-load page is replaced by another cargo's unrelated open-bed image.
    for state in ordinary.STATES:
        board = Image.new("RGB", (1048, 1120), (42, 46, 50))
        for frame in range(2):
            rows = [row for row in sprites if row["state"] == state and row["frame"] == frame]
            caption = "SEALED ROAD08 SOURCE: " + state + " phase" + str(frame) + "; native1x/4x; actual RoadVehicle owns selection"
            board.paste(fleet.base_contact(atlas, rows, ASSET, caption), (0, frame * 560))
        files[fleet.OUTPUT / ("vehicle-model-08-" + state + "-phases-contact.png")] = road.bulk.review_bytes(board)
    board = Image.new("RGB", (360, 160), (42, 46, 50))
    labels = ImageDraw.Draw(board)
    # A separate purchase contact retains both whole previews at their actual
    # UI resolution and enlargement, without claiming in-game display delivery.
    for index, row in enumerate(purchase_rows):
        x, y, width, height = row["box"]
        image = purchase.crop((x, y, x + width, y + height)).convert("RGBA")
        labels.text((12 + index * 180, 12), row["state"] + " native1x/4x", fill="white")
        board.paste(image, (20 + index * 180, 40), image)
        large = image.resize((80, 80), Image.Resampling.NEAREST)
        board.paste(large, (70 + index * 180, 40), large)
    files[fleet.OUTPUT / "purchase-model-08-contact.png"] = road.bulk.review_bytes(board)
    report = {"_documentation": "Owner: r2mlvs14-remaining-assets-and-runtime-integration.md Group22; complete sealed road source only",
              "schema_version": 1, "source_record": fleet.RECORD.relative_to(fleet.base.ROOT).as_posix(),
              "body_pose_count": len(sprites), "purchase_pose_count": len(purchase_rows),
              "native_cargo_forms": fleet.native_cargo_forms(8),
              "geometry_sha256": ordinary.signature(geometry), "components_sha256": ordinary.signature(components),
              "source_coverage_complete": True, "packed": False, "runtime_activated": False, "atlases": atlases}
    files[REPORT] = (json.dumps(report, indent=2) + "\n").encode()
    return report, files


def publish_source(document, public, report, files):
    """Publish complete own08 sources after exact proof and consumer validation.

    @param document Retained own08 bodies and explicitly shared original materials.
    @param public Shared manifest whose existing native packed fields stay exact.
    @param report Fresh complete80-body/two-purchase source metadata.
    @param files Exact regenerated artifacts including this same source report.
    @return None; refuse stale or substituted evidence before public source writes.
    """
    # Validate all original-derived pixels and matching metadata before either
    # shared source family changes; scalar counts cannot substitute a foreign one.
    for path, encoded in files.items():
        if path.read_bytes() != encoded:
            raise ValueError("sealed road source publication requires exact artifacts")
    if report != json.loads(files[REPORT]):
        raise ValueError("sealed road publication metadata differs from verified proof")
    if report["body_pose_count"] != 80 or report["purchase_pose_count"] != 2 or not report["source_coverage_complete"]:
        raise ValueError("sealed road source publication needs complete coverage")
    if [(atlas["asset_id"], len(atlas["sprites"])) for atlas in report["atlases"]] != [(ASSET, 80), ("purchase.model-08", 2)]:
        raise ValueError("sealed road publication needs exact own-model consumers")
    family = document["families"][ASSET]
    donor = document["families"]["vehicle.model-05"]["state_components"]
    keys = set(family["selections"].values())
    keys.update(row["key"] for direction, row in donor["gear"].items() if direction not in ("N", "S"))
    keys.add(family["state_components"]["status_source"]["source_key"])
    originals = [row["path"] for row in document["candidates"] if row["key"] in keys]
    meter = next(row["load_source"] for row in report["atlases"][0]["sprites"] if row["load_source"] is not None)
    originals.append(meter["source"])
    originals.append(report["atlases"][1]["sprites"][1]["lock_component"]["source"])
    originals = list(dict.fromkeys(originals))
    # Source coverage is distinct from native loading: no new range or runtime
    # flag is granted by this canonical source metadata publication.
    for atlas in report["atlases"]:
        spec = next(row for row in public["assets"] if row["asset_id"] == atlas["asset_id"])
        source = atlas["sprites"][0]["body_source"]
        spec["source"] = {"status": "authored", "path": source["source_path"], "sha256": source["source_sha256"],
                          "provenance": "first_party_original_model_and_component_assembly", "rights": "first_party_generated_original"}
        spec["authoring"] = {"date": "2026-10-11", "status": "complete_native_original_sealed_road_source_runtime_pending",
                             "source_record": report["source_record"], "originals": originals,
                             "atlas_manifest": REPORT.relative_to(fleet.base.ROOT).as_posix(), "native_atlas": atlas["path"],
                             "native_sha256": atlas["sha256"], "staged_pose_count": len(atlas["sprites"]),
                             "coverage_complete": True, "runtime_activated": False}
    fleet.base.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def main():
    """Check or publish sealed road sources while retaining pending native binding.

    @return None; print exact coverage or refuse stale source/artifact evidence.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--publish", action="store_true", help="write exact source artifacts only")
    modes.add_argument("--publish-source", action="store_true", help="publish verified complete source coverage only")
    args = parser.parse_args()
    document = json.loads(fleet.RECORD.read_text(encoding="utf-8"))
    public = json.loads(fleet.base.PUBLIC.read_text(encoding="utf-8"))
    report, files = build(document, public)
    # Only explicit repository proof paths are writable in this branch. Shared
    # source publication follows independent focused geometry/state review.
    for path, encoded in files.items():
        if args.publish:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(encoded)
        if path.read_bytes() != encoded:
            raise ValueError("sealed road source proof needs exact regeneration")
    if args.publish_source:
        publish_source(document, public, report, files)
    print(f"Sealed road08: {report['body_pose_count']} body + {report['purchase_pose_count']} purchase; {len(files)} exact artifacts; packing/runtime pending")


if __name__ == "__main__":
    main()
