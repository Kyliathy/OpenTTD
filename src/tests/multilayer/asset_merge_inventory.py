"""Observe logical art identities without publishing source, pixels or ranges.

@file asset_merge_inventory.py Read-only merge evidence; generated output is disposable.
@see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group8.
@see docs/zz-reach2/architecture/archi-historical-debug.md Storage ownership.
The donor is read only through Group2's exact retained originals in this checkout.
"""
import argparse
from functools import lru_cache
import hashlib
import itertools
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
PUBLIC = "media/multilayer/contracts/multilayer_sprite_manifest.json"
RETENTION = "src/tests/fixtures/cleanup/donor-import.json"
GEOMETRY = ("states", "frames", "directions", "tile_variants", "parts", "native_size",
            "anchor", "bounds", "footprint", "footprint_semantics", "palette",
            "transparent_index", "company_colors", "geometry")


def geometry_fields(left, right):
    """Include physical parent/child absence and signed native offsets in family review.

    @param left Destination public family, or no declaration.
    @param right Retained original donor family, or no declaration.
    @return Stable explicit fields; historical renderer hashes remain evidence only.
    """
    fields = set(GEOMETRY)
    # Native origins, relative children and structurally absent parts affect
    # placement even when the Cartesian pose counts and source PNGs match.
    for family in (left or {}, right or {}):
        fields.update(key for key in family if key.startswith(("native_", "nullable_")) or key == "source_part_policy")
    return sorted(fields)


def family_record_comparison(asset, left, right):
    """Bound differing shared records to the actual selected family instead of all authoring history.

    @param asset Exact family ID whose crops, selections and apparatus are compared.
    @param left Destination source facts with its original generation record.
    @param right Retained donor source facts with its original generation record.
    @return Explicit record/family equality or bounded differing field names.
    """
    a = (left or {}).get("source_record")
    b = (right or {}).get("source_record")
    # Missing original records cannot become equality through two absent values;
    # those families retain their explicit public/source contracts for review.
    if not a or not b:
        return {"status": "no_shared_record_comparison"}
    if a["sha256"] == b["sha256"]:
        return {"status": "identical_record"}
    first = json.loads((ROOT / a["path"]).read_text(encoding="utf-8")).get("families", {}).get(asset)
    second = json.loads((ROOT / b["path"]).read_text(encoding="utf-8")).get("families", {}).get(asset)
    # Two missing named sections are not proof of equal crops or component
    # selection; preserve that limitation instead of comparing absent values.
    if first is None or second is None:
        return {"status": "no_named_family_section"}
    differences = sorted(key for key in first.keys() | second.keys() if first.get(key) != second.get(key))
    return {"status": "different_family_section" if differences else "identical_family_section",
            "different_fields": differences}


@lru_cache(maxsize=None)
def fingerprint(relative):
    """Identify actual bytes once per observation, without assuming a historical hash.

    @param relative Repository-relative immutable original or live source file.
    @return Exact relative identity, or explicit absence for a generated/stale path.
    """
    path = ROOT / relative
    if not path.is_file():
        return {"path": relative, "present": False}
    payload = path.read_bytes()
    return {"path": relative, "present": True, "bytes": len(payload),
            "sha256": hashlib.sha256(payload).hexdigest()}


def retained_aliases():
    """Read the preserved routing facts; these are not another materialization catalogue.

    @return Original donor paths mapped to their exact declared A payload identity.
    """
    document = json.loads((ROOT / RETENTION).read_text(encoding="utf-8"))
    aliases = {}
    # Each original path must name one exact retained payload. A duplicated alias
    # with different bytes cannot choose a convenient newer art/source version.
    for row in document["payloads"]:
        for original in row["original_paths"]:
            if original in aliases and aliases[original]["sha256"] != row["sha256"]:
                raise ValueError("ambiguous retained original: " + original)
            aliases[original] = row
    return aliases


def retained(original, aliases):
    """Resolve one exact donor original through the preservation manifest, never caches.

    @param original Original repository-relative donor name.
    @param aliases Already checked Group2 routing facts.
    @return Hash-verified destination identity, or explicit missing-retention evidence.
    """
    row = aliases.get(original)
    if row is None:
        return {"original_path": original, "present": False}
    actual = fingerprint(row["destination"])
    # A routing entry alone is insufficient: changed bytes must stop the audit
    # before they can be described as an original selected graphical source.
    if actual.get("sha256") != row["sha256"] or actual.get("bytes") != row["bytes"]:
        raise ValueError("retained bytes changed: " + original)
    return {**actual, "original_path": original, "role": row["role"]}


def poses(family):
    """Expand the actual public pose domain, independent of physical packed offsets.

    @param family One public family declaration with named states/views/parts.
    @return Ordered logical (state, frame, direction, tile, part) keys.
    """
    return list(itertools.product(family["states"], range(family["frames"]),
                                 family["directions"], range(family["tile_variants"]), family["parts"]))


def source_facts(family, aliases=None):
    """Separate genuine media originals from historical generated source-atlas references.

    @param family Original public family declaration.
    @param aliases Donor routing facts, or None for current destination media.
    @return Declared primary source plus exact retained authored-original identities.
    """
    primary = family.get("source", {})
    names = set(family.get("authoring", {}).get("originals", []))
    path = primary.get("path")
    # Only authored media can become an original. A source field naming an old
    # log atlas is generated evidence and must be reconstructed by its producer.
    if path and path.startswith("media/"):
        names.add(path)
    generated_ancestry = sorted(name for name in names if not name.startswith("media/"))
    originals = []
    # Stable source ordering makes the audit comparable without using discovery
    # order as a family/pose identity or silently rewriting historical manifests.
    for name in sorted(name for name in names if name.startswith("media/")):
        originals.append(fingerprint(name) if aliases is None else retained(name, aliases))
    record = family.get("authoring", {}).get("source_record")
    if family["asset_id"] in ("control.tube", "control.geothermal"):
        record = "media/multilayer/source/ground/feature-control-generation-record.json"
    if family["asset_id"] in ("facility.utility-riser", "geothermal.conduit"):
        record = "media/multilayer/source/ground/utility-generation-record.json"
    record_identity = (fingerprint(record) if aliases is None else retained(record, aliases)) if record else None
    return {"declared_primary": primary, "originals": originals,
            "source_record": record_identity, "generated_ancestry": generated_ancestry,
            "generated_primary": bool(path and not path.startswith("media/"))}


def observation():
    """Compare every declared family and all selected ranges without drawing or publishing.

    @return Reproducible disposition facts for later Groups48-52, not current art approval.
    """
    aliases = retained_aliases()
    donor_identity = retained(PUBLIC, aliases)
    donor = json.loads((ROOT / donor_identity["path"]).read_text(encoding="utf-8"))
    destination = json.loads((ROOT / PUBLIC).read_text(encoding="utf-8"))
    a = {row["asset_id"]: row for row in destination["assets"]}
    b = {row["asset_id"]: row for row in donor["assets"]}
    # Duplicate IDs would destroy one-to-one family comparison before any pose
    # difference can be classified; neither branch may replace a row by dict order.
    if len(a) != len(destination["assets"]) or len(b) != len(donor["assets"]):
        raise ValueError("duplicate public family")
    scopes = {}
    routes = {}
    # Every published destination scope protects its full range, including
    # families absent from the incoming donor selection and reserved gaps.
    for name in ("m1", "space", "orbital", "launch", "ground"):
        path = "media/multilayer/" + name + "-pack-scope.json"
        scope = json.loads((ROOT / path).read_text(encoding="utf-8"))
        scopes[name] = {"identity": fingerprint(path), "asset_ids": scope["asset_ids"],
                        "start": scope.get("native_start"), "end_exclusive": scope.get("native_end_exclusive"),
                        "preserved_ranges": scope.get("preserved_ranges", [])}
        for asset in scope["asset_ids"]:
            if asset in routes:
                raise ValueError("duplicate destination packing owner: " + asset)
            routes[asset] = name
    dispatch = json.loads((ROOT / "media/multilayer/contracts/ground-consumer-dispatch.json").read_text(encoding="utf-8"))
    ground = {row["asset_id"]: row for row in dispatch["families"]}
    rows = []
    # Compare the complete union, not only currently selected sprites: a donor
    # omission does not release A's Space slots or erase an unactivated declaration.
    for asset in sorted(a.keys() | b.keys()):
        left, right = a.get(asset), b.get(asset)
        left_poses = poses(left) if left else []
        right_poses = poses(right) if right else []
        left_keys, right_keys = set(left_poses), set(right_poses)
        left_source = source_facts(left) if left else None
        right_source = source_facts(right, aliases) if right else None
        geometry = {key: {"A": left.get(key), "B": right.get(key)} for key in geometry_fields(left, right)
                    if left and right and left.get(key) != right.get(key)}
        ah = left.get("source", {}).get("sha256") if left else None
        bh = right.get("source", {}).get("sha256") if right else None
        category = "identical_definition"
        # Two genuinely authored primaries with different bytes need an explicit
        # family-level choice. A newer filename or packed-bundle hash cannot choose.
        if ah and bh and ah != bh:
            category = "unresolved_source_conflict"
        elif right is None:
            category = "destination_only"
        elif left is None:
            category = "donor_only_append"
        elif routes.get(asset) == "ground":
            category = "destination_owned_with_donor_integration"
        elif geometry or (ah and not bh):
            category = "destination_delivered_over_donor_placeholder"
        render = right.get("runtime", {}).get("consumer") if right else None
        current = ground.get(asset, {})
        route = routes.get(asset)
        adapter = ("regenerate_ground_sources.py: converter/build_source; pack_ground_art.py: derive" if route == "ground"
                   else "regenerate_art_sources.py: produce_source/produce; pack_m1_art.py: derive" if route == "m1"
                   else "regenerate_space_sources.py: produce_source/produce_packing" if route
                   else "unselected declaration; no new runtime activation")
        current_draw = {key: current[key] for key in ("draw_owner", "draw_symbol", "selector", "binding_header", "binding_symbol") if key in current}
        # Existing M1 consumers already own their selected family meanings. The
        # preserved old terrain families remain reserved after hardness replacement.
        if not current_draw and route == "m1":
            current_draw = render or {"status": "superseded_reserved", "binding_header": "src/multilayer_sprite.h",
                                      "binding_symbol": "GetMultiLayerSprite"}
        if route in ("space", "orbital", "launch"):
            current_draw = {"binding_header": "src/" + route + "_sprite.h",
                            "binding_symbol": "Resolve" + route.capitalize() + "Sprite",
                            "draw_owner": "src/space_vehicle.cpp", "draw_symbol": "SpaceVehicle::GetImage"}
            if asset == "facility.launch-center":
                current_draw.update(draw_owner="src/launch_facility_sprite.cpp", draw_symbol="DrawLaunchFacilityTile")
            if asset.startswith(("module.", "asteroid.")):
                current_draw.update(draw_owner="src/asteroid_gui.cpp", draw_symbol="ViewportAddSpaceEntities")
            if asset == "vehicle.model-18":
                current_draw.update(draw_owner="src/asteroid_gui.cpp", draw_symbol="ViewportAddSpaceEntities")
        rows.append({"asset_id": asset, "disposition": category, "destination_packer": route,
                     "adapter": adapter, "geometry_differences": geometry,
                     "pose_counts": {"A": len(left_poses), "B": len(right_poses)},
                     "pose_keys_equal": left_poses == right_poses,
                     "first_A_only_pose": next((key for key in left_poses if key not in right_keys), None),
                     "first_B_only_pose": next((key for key in right_poses if key not in left_keys), None),
                     "first_common_pose": next((key for key in left_poses if key in right_keys), None),
                     "A_source": left_source, "B_source": right_source,
                     "source_record_comparison": family_record_comparison(asset, left_source, right_source),
                     "A_binding": left.get("packed") if left else None, "B_binding": right.get("packed") if right else None,
                     "donor_draw_contract": render,
                     "destination_draw_contract": current_draw})
    return {"_documentation": "M2O23-1 Group8 observation; archi-historical-debug.md owns storage. Not a runtime catalogue or pixel approval.",
            "schema_version": 1, "A_manifest": fingerprint(PUBLIC), "B_manifest": donor_identity,
            "protected_destination_scopes": scopes, "families": rows}


def main():
    """Write one disposable observation; no source/canonical/fixture publication is supported.

    @return None; malformed inputs fail before publishing an observation.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("test-cache/merge-assets/family-inventory.json"))
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    output.relative_to((ROOT / "test-cache").resolve())
    document = observation()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    counts = {}
    for row in document["families"]:
        counts[row["disposition"]] = counts.get(row["disposition"], 0) + 1
    print(json.dumps({"output": output.relative_to(ROOT).as_posix(), "families": len(document["families"]), "dispositions": counts}))


if __name__ == "__main__":
    main()
