"""Publish only the qualified native launch bindings, retaining the paid-game gate.

@file launch_fleet_runtime_publication.py
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group28 retired cache publication.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JC.2.
The exact earlier packing publication supplies the immutable metadata baseline.
Only the two qualified public authoring/runtime fields change; original source
records, packed checkpoints, native pixels and all other public families stay exact.
"""
# Refuse this completed historical entry before importing any old cache readers.
# Current recipes preserve its original facts without rerunning source publication.
if __name__ == "__main__":
    raise SystemExit("Historical launch runtime publication is retired; current recipes must not read or archive its old EXE graph. Use artifact_rebuild.py launch-current-producers --configuration RelWithDebInfo for current evidence.")

import argparse
import copy
import hashlib
import json
from pathlib import Path
import launch_facility_selector_checks as selectors
import launch_fleet_payload_checks as payload

ROOT = selectors.ROOT
OUTPUT = ROOT / "logs/proofs/mlm/native-art/space/launch-fleet-runtime-v1"
PUBLIC = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
PACKING = ROOT / "logs/proofs/mlm/native-art/space/launch-fleet-packing-v1/final-verification.json"
RENDER = ROOT / "logs/proofs/mlm/native-art/space/launch-facility-selectors-v1/final-verification.json"
ASSETS = ("facility.launch-center", "vehicle.model-14")
STATUS = "source_packed_native_bound_game_review_pending"
TARGET_ADDITION = """    # Current original geometry, phase and custody producers: r2mlvs14 Group62JC.
    src/tests/launch_site.cpp
    src/tests/launch_production.cpp
    src/tests/space_logistics.cpp
    src/tests/launch_flight.cpp
    src/tests/space_descent.cpp
"""


def read(path):
    """Read one explicitly selected UTF8 evidence document.

    @param path Exact repository-owned JSON input.
    @return Parsed document without filename/latest inference.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def checked(reference):
    """Require an actual immutable source/artifact identity.

    @param reference Declared repository-relative path and exact SHA256.
    @return Same verified reference for inclusion in derived current evidence.
    @throws ValueError On a missing, changed or substituted artifact.
    """
    if selectors.art.source.identity(ROOT / reference["path"]) != reference:
        raise ValueError("changed native runtime input: " + reference["path"])
    return reference


def verify_inputs():
    """Refuse the completed historical publication and its obsolete cache graph.

    @return None; this retired entry always raises before reading or writing artifacts.
    @throws ValueError Current evidence requires the registered recipe and live source plan.
    """
    raise ValueError("Historical launch runtime publication is retired; current recipes must not read or archive its old EXE graph.")


def expected_public(original, manifest):
    """Change only the qualified families' explicit native/runtime metadata.

    @param original Exact earlier packing publication, preserving every source and MLVI field.
    @param manifest Current two-family native binding document, explicitly pending paid-game review.
    @return Complete expected public document with all unrelated bytes' semantics preserved.
    """
    document = copy.deepcopy(original)
    bindings = {row["asset_id"]: row for row in manifest["bindings"]}
    # Only these native consumers gained a qualified binding. All previously
    # published families and immutable source/geometry identities remain exact.
    for row in document["assets"]:
        if row["asset_id"] not in ASSETS:
            continue
        row["authoring"].update(status=STATUS, runtime_activated=True)
        row["runtime"] = dict(status="native_bound_game_review_pending", consumer=bindings[row["asset_id"]]["consumer"],
                              binding_manifest="logs/proofs/mlm/native-art/space/launch-fleet-runtime-v1/runtime-manifest.json",
                              ordinary_game_reviewed=False, user_review="deferred_until_integration")
    return document


def publish(commit=False):
    """Verify or publish current native bindings while retaining ordinary acceptance.

    @param commit Write the already qualified two-family public runtime metadata when true.
    @return Exact publication receipt with immutable source and previous-family boundaries.
    @throws ValueError On any unqualified metadata/source/native change.
    """
    packing, original, evidence, applications = verify_inputs()
    manifest = dict(_documentation=__doc__, complete=True, native_binding_verified=True, ordinary_game_reviewed=False,
                    source_handoff=packing["source_handoff"], packed_checkpoint=packing["packed_manifest"],
                    evidence=evidence, applications=applications, bindings=[
        dict(asset_id="facility.launch-center", native_offset=6143, sprite_count=1536, selector="GetLaunchFacilityArtPose/ResolveLaunchSprite",
             consumer=dict(source="src/station_cmd.cpp", symbol="DrawTile_Station", call="DrawLaunchFacilityTile"),
             state_authorities=["GetLaunchFacilityZone", "LaunchOperations", "SpaceConstructionState", "IsSpaceServiceWorkTick", "SpacePodState"],
             foreground_policy="per_native_zone_visitor_scope", gallery_status="native_component_verified_paid_game_pending"),
        dict(asset_id="vehicle.model-14", native_offset=7679, sprite_count=112, selector="GetLaunchRocketArtPose/GetLaunchRocketArtDirection/ResolveLaunchSprite",
             consumer=dict(source="src/space_vehicle.cpp", symbol="SpaceVehicle::GetImage"),
             state_authorities=["SpaceTransition.phase", "SpaceTransition.remaining_ticks", "SpaceTransition.outbound_burned/recovery_burned", "GetSpaceVehicleLayer"],
             surface_visibility="IsLaunchSurfaceCraftArtVisible", contact="GetLaunchCraftArtBounds", gallery_status="native_component_verified_paid_game_pending")])
    expected = expected_public(original, manifest)
    current = read(PUBLIC)
    # Only the exact original or this precise qualified publication is accepted.
    # Other family, source, range, authoring or gameplay metadata cannot piggyback.
    if current != original and current != expected:
        raise ValueError("public metadata differs beyond the two qualified runtime bindings")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    destination = OUTPUT / "runtime-manifest.json"
    destination.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    if commit:
        PUBLIC.write_text(json.dumps(expected, indent=2) + "\n", encoding="utf-8")
        if read(PUBLIC) != expected:
            raise ValueError("runtime publication readback differs")
    receipt = dict(_documentation=__doc__, complete=True, published=commit, ordinary_game_reviewed=False,
                   native_binding_verified=True, families=list(ASSETS), protected_public_rows=len(original["assets"])-2,
                   public_manifest=selectors.art.source.identity(PUBLIC), runtime_manifest=selectors.art.source.identity(destination),
                   publisher=selectors.art.source.identity(Path(__file__)), public_before=selectors.art.source.identity(OUTPUT/"public-before-runtime.json"),
                   immutable_packing_checkpoint=selectors.art.source.identity(PACKING), evidence=evidence, applications=applications)
    name = "publication-verification.json" if commit else "publication-preview.json"
    (OUTPUT/name).write_text(json.dumps(receipt, indent=2)+"\n", encoding="utf-8")
    print("qualified native launch bindings; published",commit,"ordinary paid-game acceptance remains open",flush=True)
    return receipt


def main():
    """Inspect current compatibility by default; publish only the explicit qualified scope."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--publish", action="store_true")
    publish(parser.parse_args().publish)


if __name__ == "__main__":
    main()
