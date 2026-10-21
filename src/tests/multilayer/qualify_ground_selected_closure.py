"""Close exactly the selected50-family source/packing/current-consumer result.

@file qualify_ground_selected_closure.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHD.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.4D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Historical native view epochs remain immutable. Complete unaffected tuple
identity links them to the current corrected container; later gameplay stays open.
"""

import json
import math
import struct

from ground_pack_check_support import ROOT, handoff
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import checked_reference, reference

RUN = ROOT / "logs/graphics/selected-ground-runtime/20261021-selected-closure-01"
DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-consumers-v1"
BASELINE = (RUN / "before-audit.json").relative_to(ROOT).as_posix()


def native_domain(path):
    """Read the complete independently sourced fixture's exact native ID domain.

    @param path Hash-verified native byte fixture already exercised by both actual loaders.
    @return All22,018 native offsets, without decoding new sprites or changing source ownership.
    """
    data = path.read_bytes()
    if data[:8] != b"GRDART1\0" or struct.unpack_from("<HHHH", data, 8) != (22018, 22045, 7791, 14254):
        raise ValueError("selected current native fixture header changed")
    cursor, offsets = 16, []
    # Every record includes complete palette bytes. Traverse their declared
    # lengths to preserve gaps/aliases rather than guessing a contiguous range.
    for _ in range(22018):
        offset, width, height, x, y, digest = struct.unpack_from("<HHHhhI", data, cursor)
        cursor += 14 + width * height
        offsets.append(offset)
    if cursor != len(data) or offsets != sorted(set(offsets)):
        raise ValueError("selected current native fixture has duplicate, truncated or trailing records")
    return set(offsets)


def immutable_views(name):
    """Validate retained actual execution/view receipts without rewriting their historical code/payload epoch.

    @param name Exact completed qualification basename beneath the native consumer proof directory.
    @return Existing immutable qualification and its exact reference.
    """
    path = DIRECTORY / (name + ".json")
    proof = handoff.read_json(path.relative_to(ROOT).as_posix())
    if proof.get("passed") is not True:
        raise ValueError("selected current consumer view proof is incomplete: " + name)
    # The actual old executing image survives later working-target relinks.
    # Its source/current-container hashes remain dated; never rehash that old
    # receipt and pretend its execution happened under a newer publication.
    for native in proof["native"]:
        checked_reference(native["retained_application"])
        for observation in native.get("scopes", [native]):
            checked_reference(observation["execution"])
            checked_reference(observation["pixels"])
    return proof, reference(path)


def qualify():
    """Join current source/native/publication/viewport/retained-world facts with the exact selected dispatch boundary.

    @return Complete selected10.4 source/packing/current-view result; whole underground gameplay remains incomplete.
    """
    source_path, dispatch_path = RUN / "current-source-audit.json", RUN / "current-dispatch-audit.json"
    source = handoff.read_json(source_path.relative_to(ROOT).as_posix())
    dispatch_audit = handoff.read_json(dispatch_path.relative_to(ROOT).as_posix())
    # Complete originals and source matrices are prerequisites. A packing or
    # viewport success cannot silently drop any original selected family.
    if (source["selected_families"] != 50 or source["selected_source_poses"] != 15488 or source["source_coverage_complete"] is not True
            or dispatch_audit.get("passed") is not True or dispatch_audit["selected_families"] != 50
            or dispatch_audit["selected_poses"] != 15488
            or dispatch_audit["dispatch_counts"] != {"current_bound": 21, "current_existing": 3, "later_owner": 26}):
        raise ValueError("selected source/dispatch closure changed its exact fifty-family scope")
    checked_reference(source["public_manifest"])
    checked_reference(dispatch_audit["contract"])
    checked_reference(dispatch_audit["packed_handoff"])
    for row in source["families"]:
        for key in ("original", "native_atlas", "source_manifest"):
            checked_reference(row[key])
    publication_path = ROOT / "logs/proofs/mlm/native-art/ground/ground-station-offsets-v1/publication.json"
    publication = handoff.read_json(publication_path.relative_to(ROOT).as_posix())
    if publication.get("native_loading_verified") is not True or publication["unchanged_native_sprites"] != 21586:
        raise ValueError("selected current native container lacks complete decoded/native qualification")
    for key in ("payload", "public", "packed_handoff", "retained_payload", "build_proof"):
        checked_reference(publication[key])
    build = handoff.read_json(publication["build_proof"]["path"])
    domain = native_domain(checked_reference(build["fixture"]))
    changed = set(handoff.read_json(publication["packed_handoff"]["path"])["changed_native_offsets"])
    if len(changed) != 432:
        raise ValueError("selected corrected native tuple scope changed")
    industry, industry_ref = immutable_views("industry-states")
    bodies, bodies_ref = immutable_views("transport-bodies")
    purchases, purchases_ref = immutable_views("transport-purchases")
    station, station_ref = immutable_views("station-viewports")
    if station["corrected_publication"] != reference(publication_path):
        raise ValueError("selected station foreground proof belongs to another native publication")
    retained = []
    # Current actual application reloads the original funded input without
    # replacement construction/materials. Exact source pixels and complete old
    # RGB/index bytes establish preservation under the current container.
    for configuration in ("Debug", "RelWithDebInfo"):
        path = RUN / ("current-retained-" + configuration.lower()) / "retained.json"
        proof = handoff.read_json(path.relative_to(ROOT).as_posix())
        if (proof.get("passed") is not True or proof["assertions"] != 17 or proof["exit_code"] != 0
                or proof["pixels"]["matched_source_pixels"] != 8930 or proof["original_world_viewport_bytes_preserved"] is not True
                or proof["runtime_payload"]["sha256"] != publication["payload"]["sha256"]
                or proof["replacement_funding_or_materials"] is not False):
            raise ValueError("selected retained actual world/collector preservation is incomplete")
        for row in proof["inputs"] + proof["captures"] + [proof[key] for key in ("source", "terminal", "retained_application", "runtime_payload")]:
            checked_reference(row)
        retained.append({"configuration": configuration, "execution": reference(path),
                         "retained_application": proof["retained_application"], "native_assertions": 17,
                         "matched_source_pixels": 8930, "source": proof["source"]})
    public = handoff.read_json(handoff.PUBLIC.relative_to(ROOT).as_posix())
    families = {row["asset_id"]: row for row in public["assets"]}
    dispatch = handoff.read_json(dispatch_audit["contract"]["path"])
    if dispatch["packed_handoff"] != publication["packed_handoff"]:
        raise ValueError("selected dispatch does not name the current corrected native handoff")
    source_families = {row["asset_id"] for row in source["families"]}
    coverage, allocated, current = [], set(), set()
    # Every selected source keeps its whole native Cartesian range. Current
    # view evidence is joined explicitly; twenty-six later owners retain their
    # exact dispatched gameplay tasks and acquire no operating claim from art.
    for row in dispatch["families"]:
        asset = row["asset_id"]
        family = families[asset]
        begin, count = family["packed"]["native_offset"], family["packed"]["sprite_count"]
        required = math.prod((len(family["states"]), family["frames"], len(family["directions"]), family["tile_variants"], len(family["parts"])))
        offsets = set(range(begin, begin + count))
        if count != required or not offsets <= domain or offsets & allocated or family["packed"]["sha256"] != publication["payload"]["sha256"]:
            raise ValueError("selected native source domain/range/checksum changed: " + asset)
        allocated.update(offsets)
        evidence = None
        if row["dispatch"] != "later_owner":
            current.add(asset)
            if asset in ("facility.rail-station", "facility.road-terminal"):
                evidence = station_ref
            elif asset == "facility.vent-collector":
                evidence = {"current_retained": [item["execution"] for item in retained]}
            elif asset.startswith("vehicle.model-"):
                evidence = bodies_ref
            elif asset.startswith("purchase.model-"):
                evidence = purchases_ref
            elif asset in industry["families"]:
                evidence = industry_ref
            else:
                raise ValueError("selected current family lacks an actual native viewport: " + asset)
            # Earlier actual views retain their old publication epoch. Only
            # fully unchanged decoded tuples may carry their accepted pixels
            # into this current container; corrected facilities use fresh views.
            if asset not in ("facility.rail-station", "facility.road-terminal") and offsets & changed:
                raise ValueError("historical view reused after its native pixels/offsets changed: " + asset)
        elif not row.get("group") or not row.get("master") or not row.get("reason"):
            raise ValueError("selected later owner lost its explicit unfinished gameplay dispatch")
        coverage.append({"asset_id": asset, "native_offset": begin, "sprite_count": count, "dispatch": row["dispatch"],
                         "owner": row["owner"], "group": row.get("group"), "master": row.get("master"),
                         "actual_view_evidence": evidence, "native_decoded_and_loaded": True,
                         "operating_claimed": False if row["dispatch"] == "later_owner" else None})
    if {row["asset_id"] for row in coverage} != source_families or len(current) != 24 or len(allocated) != 15488:
        raise ValueError("selected current/later coverage omitted a required family or pose")
    transitions, count = preserved(BASELINE)
    result = {"_documentation": __doc__, "passed": True, "selected_families": 50, "selected_native_poses": 15488,
              "current_viewport_families": 24, "later_owner_families": 26, "coverage": coverage,
              "source_audit": reference(source_path), "dispatch_audit": reference(dispatch_path),
              "publication": reference(publication_path), "retained_current_world": retained,
              "native_current_views": [industry_ref, bodies_ref, purchases_ref, station_ref],
              "historical_view_epochs_preserved": True, "unchanged_tuple_bridge": publication["build_proof"],
              "baseline": reference(handoff.bound(BASELINE)), "preserved_file_count": count, "transitions": transitions,
              "inputs": [reference(ROOT / path) for path in ("src/tests/multilayer/qualify_ground_selected_closure.py",
                         "src/tests/multilayer/ground_selected_closure_checks.py")],
              "master_10_4_selected_scope_complete": True, "whole_underground_plan_complete": False,
              "later_tube_plant_riser_lifecycle_and_human_gates": "open", "transported_bootstrap_claimed": False}
    (DIRECTORY / "selected-current-closure.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


if __name__ == "__main__":
    result = qualify()
    print(json.dumps({key: result[key] for key in ("passed", "selected_families", "selected_native_poses", "current_viewport_families", "later_owner_families", "master_10_4_selected_scope_complete")}))
