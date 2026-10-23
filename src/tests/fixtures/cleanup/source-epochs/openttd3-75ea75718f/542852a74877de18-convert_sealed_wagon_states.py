"""Assemble sealed rail wagon sources from exact approved original components.

@file convert_sealed_wagon_states.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group20B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group22 shared original meter material
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
URAN drums and covered RARE retain native packet custody inside the closed body.
No shell, inventory, animation clock or native range is copied from model05.
"""

import argparse
import hashlib
import io
import json

from PIL import Image, ImageDraw

import convert_facility_art as facilities
import convert_ground_wagon_states as ordinary
from convert_terrain_art import png_bytes


fleet = ordinary.fleet
REPORT = fleet.OUTPUT / "vehicle-model-06-source-manifest.json"


def family_spec(document, public):
    """Require own sealed body geometry and explicitly shared component ancestry.

    @param document Current retained source record with inspected06 attachments.
    @param public Shared native world/purchase contract.
    @return Own06 geometry/components and exact approved05 component registrations.
    """
    spec = fleet.public_family("vehicle.model-06", public)
    family = document["families"]["vehicle.model-06"]
    geometry, components = family["state_component_geometry"], family["state_components"]
    donor_geometry, donor_components = ordinary.checked_geometry(document, public)
    donor = document["families"]["vehicle.model-05"]
    # Own-model attachments remain fixed even when a genuine generic axle or
    # glass interior is reused. Matching dimensions cannot authorize a foreign
    # shell, exposed cargo hold or copied native capability/state owner.
    if (ordinary.signature(geometry) != family["approved_state_component_geometry_sha256"]
            or ordinary.signature(components) != family["approved_state_components_sha256"]
            or components["geometry_sha256"] != ordinary.signature(geometry)
            or geometry["body_registrations"] != family["approved_base_registration"]
            or geometry["state_owner"] != spec["native_state_policy"]
            or geometry["native_canvas"] != [64, 48] or geometry["anchor"] != [-32, -32]
            or spec["company_colors"] != "explicit_mask"):
        raise ValueError("sealed wagon component geometry or ownership changed")
    own_record = fleet.RECORD.relative_to(fleet.base.ROOT).as_posix()
    # These two material/component owners are explicit approved originals,
    # not a search for a plausible model or the newest workstation cache file.
    for name in ("gear_source", "status_source"):
        source = components[name]
        if (source["source_record"] != own_record or source["source_family"] != "vehicle.model-05"
                or source["source_components_sha256"] != donor["approved_state_components_sha256"]):
            raise ValueError("foreign sealed wagon shared component owner")
    # Only the reviewed generic glass interior may be shared here; a foreign
    # vehicle's complete status sprite would transfer its shell/attachment.
    if components["status_source"]["source_key"] != "wagon-status-lenses-v2" or not components["status_source"]["material_only"]:
        raise ValueError("sealed wagon status source is not approved glass material")
    if set(components["gear_source"]["source_points"]) != set(fleet.DIRECTIONS):
        raise ValueError("sealed wagon lacks a genuine shared axle heading")
    return geometry, components, donor_geometry, donor_components


def component_sources(document, geometry, components, donor_geometry, donor_components):
    """Retarget original axle interiors and sample verified generic glass material.

    @param document Immutable original source record and approval signatures.
    @param geometry Inspected own06 body attachment rectangles and meter points.
    @param components Exact shared axle/status/meter/purchase original selection.
    @param donor_geometry Approved05 physical axle source rectangles.
    @param donor_components Approved05 genuine eight-way axle/four-glass sources.
    @return Own06 positioned axle donors, nondirectional lamps and amber evidence.
    """
    donors, lenses = ordinary.state_sources(document, donor_geometry, donor_components)
    gears = {}
    # Each actual native camera supplies its own original axle highlights.
    # Only those source points enter inspected06 wheel faces; no donor chassis,
    # coupling, alpha, cargo pixel or whole native body is reused.
    for direction in fleet.DIRECTIONS:
        donor, evidence = donors[direction]
        source_points = components["gear_source"]["source_points"][direction]
        targets = geometry["gear_regions"][direction]
        allowed = {(px, py) for x, y, width, height in donor_geometry["gear_regions"][direction]
                   for py in range(y, y + height) for px in range(x, x + width)}
        if len(source_points) != 2 or len(targets) != 2:
            raise ValueError("sealed wagon axle source/target count changed")
        native = Image.new("P", (64, 48), 0)
        native.putpalette(fleet.base.PALETTE_BYTES)
        for point, target in zip(source_points, targets):
            x, y, width, height = target
            # An explicitly registered original wheel interior is a reusable
            # material component; a blue shell point or enlarged target is not.
            if tuple(point) not in allowed or width != 1 or height != 1 or y >= 39:
                raise ValueError("sealed wagon shared axle leaves approved source/target")
            selected = donor.getpixel(tuple(point))
            if selected == 0:
                raise ValueError("sealed wagon shared axle has no original pixel")
            native.putpixel((x, y), selected)
        gears[direction] = native, {**evidence, "source_family": "vehicle.model-05",
                                    "shared_component_only": True, "source_points": source_points,
                                    "target_regions": targets}
    return gears, lenses, load_meter_source(components["meter_source"])


def load_meter_source(meter):
    """Sample verified original amber glass without importing a vehicle fitting.

    @param meter Explicit retained M1 glass record/key/state/frame registration.
    @return One indexed original glass sample and its immutable source evidence.
    """
    # This is the already delivered sealed-capsule meter material. One/two
    # original amber glass samples communicate packet buckets without opening
    # the body, drawing fake uranium granules or introducing another volume.
    if (meter["source_record"] != facilities.RECORD.relative_to(fleet.base.ROOT).as_posix()
            or meter["source_key"] != "facility-status" or meter["state"] != "construction"
            or meter["frame"] not in range(4) or not meter["material_only"]):
        raise ValueError("foreign sealed wagon load meter material")
    original = next(row for row in json.loads(facilities.RECORD.read_text(encoding="utf-8"))["components"]
                    if row["key"] == meter["source_key"])
    # The selected original lamp phase supplies material brightness only. Its
    # authored source frame is not a road/train clock or a production state.
    glass = facilities.indicator(original)[facilities.STATES.index("construction")][meter["frame"]]
    index = fleet.base.quantize(glass.resize((1, 1), Image.Resampling.BOX), {}).getpixel((0, 0))
    return index, {"source": original["path"], "source_sha256": original["sha256"], **meter}


def closed_load(body, direction, state, geometry, meter):
    """Keep the actual sealed shell and expose only packet-backed external fill.

    @param body Exact own-model selected empty body with fixed hatch/coupler alpha.
    @param direction Genuine native engine camera for its own attachment positions.
    @param state Empty/partial/full or actual empty service/reservation hold.
    @param geometry Inspected own-model two-point external fill fitting.
    @param meter Genuine original amber material and immutable source evidence.
    @return Unchanged closed body plus admitted meter pixels and source evidence.
    """
    image = body.copy()
    if state not in ("partial", "full"):
        return image, None
    points = geometry["sealed_meter_points"][direction]
    # Partial and full need independent physical fittings; duplicate points
    # would hide full-load information while falsely completing source coverage.
    if len(points) != 2 or len({tuple(point) for point in points}) != 2:
        raise ValueError("sealed wagon load meter points are absent or ambiguous")
    index, evidence = meter
    selected = points[:1] if state == "partial" else points
    # Partial/full are categories from actual native freight packets, never a second
    # capacity ledger. Full adds a second independent original light and leaves
    # the existing partial light and every opaque roof/hatch/body pixel intact.
    for x, y in selected:
        if body.getpixel((x, y)) == 0:
            raise ValueError("sealed load meter leaves original shell opacity")
        image.putpixel((x, y), index)
    if image.tobytes() == body.tobytes():
        raise ValueError("sealed load meter disappears at native palette scale")
    return image, {**evidence, "native_points": selected, "category": state}


def build(document=None, public=None):
    """Assemble all80 sealed-body and two purchase poses from real originals.

    @param document Optional isolated retained fleet source record for refusals.
    @param public Optional isolated shared native sprite contract.
    @return Complete sealed source manifest and exact source/review artifact bytes.
    """
    if document is None:
        document = json.loads(fleet.RECORD.read_text(encoding="utf-8"))
    if public is None:
        public = json.loads(fleet.base.PUBLIC.read_text(encoding="utf-8"))
    geometry, components, donor_geometry, donor_components = family_spec(document, public)
    gears, lenses, meter = component_sources(document, geometry, components, donor_geometry, donor_components)
    base_report, files = fleet.build_base("vehicle.model-06", document, public)
    if not base_report["base_geometry_complete"]:
        raise ValueError("sealed wagon source needs all genuine base headings")
    base_atlas = Image.open(io.BytesIO(files[fleet.base.repository_path(base_report["path"])]))
    bodies = {}
    # Every state retains its own actual heading and source ancestry; a reused
    # wheel/glass material may not replace the selected closed wagon model.
    for row in base_report["sprites"]:
        x, y, width, height = row["box"]
        bodies[row["direction"]] = base_atlas.crop((x, y, x + width, y + height)), row
    atlas = Image.new("P", (512, 480), 0)
    atlas.putpalette(fleet.base.PALETTE_BYTES)
    atlas.info["transparency"] = 0
    sprites = []
    # Both actual cargos have the same closed exterior. Only packet buckets,
    # speed-owned rolling phase and actual empty status change original parts.
    for state_index, state in enumerate(ordinary.STATES):
        for frame in range(2):
            for direction_index, direction in enumerate(fleet.DIRECTIONS):
                body, body_source = bodies[direction]
                load, load_evidence = closed_load(body, direction, state, geometry, meter)
                image, evidence = ordinary.apply_components(load, direction, state, frame, geometry, gears, lenses)
                x, y = direction_index * 64, (state_index * 2 + frame) * 48
                atlas.paste(image, (x, y))
                sprites.append({"state": state, "frame": frame, "direction": direction, "tile_variant": 0,
                                "cargo_labels": ["URAN", "RARE"], "part": "body", "anchor": [-32, -32],
                                "box": [x, y, 64, 48], "body_source": body_source,
                                "load_source": load_evidence, "components": evidence,
                                "visual_alias": "actual URAN sealed drums and RARE covered granules share one closed exterior"})
    purchase, purchase_rows = ordinary.purchase_source(bodies["E"][0], components, public, "purchase.model-06")
    for row in purchase_rows:
        row["body_source"] = bodies["E"][1]
    atlases = []
    # World and purchase consumers keep independent canvases, anchors/counts
    # and state admission while both preserve the own-model original silhouette.
    for asset_id, name, image, rows in (("vehicle.model-06", "vehicle-model-06-authored.png", atlas, sprites),
                                        ("purchase.model-06", "purchase-model-06-authored.png", purchase, purchase_rows)):
        path = fleet.OUTPUT / name
        encoded = png_bytes(image)
        files[path] = encoded
        atlases.append({"asset_id": asset_id, "path": path.relative_to(fleet.base.ROOT).as_posix(),
                        "sha256": hashlib.sha256(encoded).hexdigest(), "size": list(image.size), "sprites": rows})
    # Separate native/enlarged two-phase pages make wheel/status differences
    # inspectable without shrinking a tall all-state page below native scale.
    for state in ordinary.STATES:
        board = Image.new("RGB", (1048, 1120), (42, 46, 50))
        for frame in range(2):
            selected = [row for row in sprites if row["state"] == state and row["frame"] == frame]
            page = fleet.base_contact(atlas, selected, "Model06 " + state + " phase" + str(frame),
                                      caption="SEALED SOURCE ONLY; native1x/4x; " + state + " phase" + str(frame) + "; actual Train owns selection")
            board.paste(page, (0, frame * 560))
        stream = io.BytesIO()
        board.save(stream, format="PNG", optimize=False, compress_level=9)
        files[fleet.OUTPUT / ("vehicle-model-06-" + state + "-phases-contact.png")] = stream.getvalue()
    board = Image.new("RGB", (360, 160), (42, 46, 50))
    draw = ImageDraw.Draw(board)
    # The real20x20 preview and separate original calendar badge are reviewed
    # at native1x and nearest4x without implying actual build availability.
    for index, row in enumerate(purchase_rows):
        x, y, width, height = row["box"]
        image = purchase.crop((x, y, x + width, y + height)).convert("RGBA")
        draw.text((12 + index * 180, 12), row["state"] + " native1x/4x", fill="white")
        board.paste(image, (20 + index * 180, 40), image)
        large = image.resize((80, 80), Image.Resampling.NEAREST)
        board.paste(large, (70 + index * 180, 40), large)
    stream = io.BytesIO()
    board.save(stream, format="PNG", optimize=False, compress_level=9)
    files[fleet.OUTPUT / "purchase-model-06-contact.png"] = stream.getvalue()
    report = {"_documentation": "Owner: r2mlvs14-remaining-assets-and-runtime-integration.md Group20B.3; complete sealed rail source only",
              "schema_version": 1, "source_record": fleet.RECORD.relative_to(fleet.base.ROOT).as_posix(),
              "body_pose_count": len(sprites), "purchase_pose_count": len(purchase_rows),
              "native_cargo_forms": fleet.native_cargo_forms(6),
              "geometry_sha256": ordinary.signature(geometry), "components_sha256": ordinary.signature(components),
              "source_coverage_complete": True, "packed": False, "runtime_activated": False, "atlases": atlases}
    files[REPORT] = (json.dumps(report, indent=2) + "\n").encode()
    return report, files


def publish_source(document, public, report, files):
    """Bind complete verified06 originals through the existing shared source owner.

    @param document Current immutable body and explicitly shared component record.
    @param public Current shared manifest whose packed fields remain untouched.
    @param report Fresh complete80-body/two-purchase source manifest.
    @param files Fresh exact artifacts generated with the same source manifest.
    @return None; refuse stale or incomplete proof before shared metadata changes.
    """
    # Every current artifact must agree before either source family is changed;
    # a cached image or complete-looking metadata cannot conceal missing pixels.
    for path, encoded in files.items():
        if path.read_bytes() != encoded:
            raise ValueError("sealed wagon source publication requires exact artifacts")
    if report != json.loads(files[REPORT]):
        raise ValueError("sealed wagon publication metadata differs from verified proof")
    # Both exact own-model consumers must close together. A complete-looking
    # scalar count cannot bind another vehicle or replace actual source rows.
    if report["body_pose_count"] != 80 or report["purchase_pose_count"] != 2 or not report["source_coverage_complete"]:
        raise ValueError("sealed wagon source publication requires complete coverage")
    expected = [("vehicle.model-06", 80), ("purchase.model-06", 2)]
    if [(atlas["asset_id"], len(atlas["sprites"])) for atlas in report["atlases"]] != expected:
        raise ValueError("sealed wagon publication needs exact own-model consumers")
    family = document["families"]["vehicle.model-06"]
    donor = document["families"]["vehicle.model-05"]["state_components"]
    keys = set(family["selections"].values())
    keys.update(row["key"] for row in donor["gear"].values())
    keys.add(family["state_components"]["status_source"]["source_key"])
    originals = [row["path"] for row in document["candidates"] if row["key"] in keys]
    meter = next(row["load_source"] for row in report["atlases"][0]["sprites"] if row["load_source"] is not None)
    originals.append(meter["source"])
    originals.append(report["atlases"][1]["sprites"][1]["lock_component"]["source"])
    originals = list(dict.fromkeys(originals))
    # This publishes source coverage only through the canonical shared fields.
    # Native ranges, cargo identity, availability and drawing remain untouched.
    for atlas in report["atlases"]:
        spec = next(row for row in public["assets"] if row["asset_id"] == atlas["asset_id"])
        source = atlas["sprites"][0]["body_source"]
        spec["source"] = {"status": "authored", "path": source["source_path"], "sha256": source["source_sha256"],
                          "provenance": "first_party_original_model_and_component_assembly", "rights": "first_party_generated_original"}
        spec["authoring"] = {"date": "2026-10-11", "status": "complete_native_original_sealed_wagon_source_runtime_pending",
                             "source_record": report["source_record"], "originals": originals,
                             "atlas_manifest": REPORT.relative_to(fleet.base.ROOT).as_posix(), "native_atlas": atlas["path"],
                             "native_sha256": atlas["sha256"], "staged_pose_count": len(atlas["sprites"]),
                             "coverage_complete": True, "runtime_activated": False}
    fleet.base.PUBLIC.write_text(json.dumps(public, indent=2) + "\n", encoding="utf-8")


def main():
    """Publish or verify complete source proofs while leaving native binding pending.

    @return None; print coverage or refuse stale source/artifact provenance.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--publish", action="store_true", help="write exact sealed source proofs, without public activation")
    modes.add_argument("--publish-source", action="store_true", help="bind complete source only after exact proof verification")
    args = parser.parse_args()
    document = json.loads(fleet.RECORD.read_text(encoding="utf-8"))
    public = json.loads(fleet.base.PUBLIC.read_text(encoding="utf-8"))
    report, files = build(document, public)
    # Only the fixed repository-local source proof cache is writable here;
    # complete shared source publication follows independent focused review.
    for path, encoded in files.items():
        if args.publish:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(encoded)
        if path.read_bytes() != encoded:
            raise ValueError("sealed wagon source proof needs exact regeneration")
    if args.publish_source:
        publish_source(document, public, report, files)
    print(f"Sealed wagon sources: {report['body_pose_count']} body + {report['purchase_pose_count']} purchase poses; {len(files)} exact artifacts; packing/runtime pending")


if __name__ == "__main__":
    main()
