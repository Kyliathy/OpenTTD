"""Publish only the qualified native launch bindings, retaining the paid-game gate.

@file launch_fleet_runtime_publication.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JC.2.
The exact earlier packing publication supplies the immutable metadata baseline.
Only the two qualified public authoring/runtime fields change; original source
records, packed checkpoints, native pixels and all other public families stay exact.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import shutil
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
    """Bind executed current producers to unchanged original source and rendered pixels.

    @return Earlier packing, exact public baseline, native proof references and archived current applications.
    @throws ValueError On stale native execution, renderer drift, prefix mutation or missing original metadata.
    """
    packing = read(PACKING)
    # Native activation consumes the exact accepted packing/source checkpoint;
    # it cannot authorize a replacement source record, payload or decoder archive.
    for key in ("payload", "packed_manifest", "source_record", "source_handoff", "qualified_native_applications"):
        checked(packing[key])
    checked(packing["original_applications"])
    payload.preserved_applications()
    selectors.boundary()
    original = OUTPUT / "public-before-runtime.json"
    # The exact earlier qualified publication owns this baseline. Never seed a
    # new expected document from changed or already-activated current metadata.
    if not original.exists():
        if selectors.art.source.identity(PUBLIC) != packing["public_manifest"]:
            raise ValueError("original packing publication is unavailable for runtime baseline")
        original.write_bytes(PUBLIC.read_bytes())
    if hashlib.sha256(original.read_bytes()).hexdigest() != packing["public_manifest"]["sha256"]:
        raise ValueError("immutable pre-runtime public baseline changed")
    renderer = read(RENDER)
    references = [selectors.art.source.identity(PACKING), selectors.art.source.identity(RENDER)]
    # Later test-target additions change current binaries, so earlier actual
    # rendered evidence retains its own exact qualified native applications.
    for application in renderer["applications"]:
        checked(application["retained"])
    # The current target adds only original producer cases. Reconstruct its
    # exact preceding CMake bytes while every actual renderer source stays exact;
    # historical native apps remain archived instead of being called current.
    for reference in renderer["proofs"]:
        checked(reference)
        proof = read(ROOT / reference["path"])
        if "sources" not in proof:
            continue
        for source in proof["sources"]:
            if source["path"] == "CMakeLists.txt":
                data = (ROOT / source["path"]).read_bytes()
                addition = TARGET_ADDITION.replace("\n", "\r\n").encode() if b"\r\n" in data else TARGET_ADDITION.encode()
                if data.count(addition) != 1 or hashlib.sha256(data.replace(addition, b"")).hexdigest() != source["sha256"]:
                    raise ValueError("current target differs beyond its original producer additions")
            else:
                checked(source)
        checked(proof["native_observations"])
        # Every actual capture remains available, including empty/wrong-layer
        # views; a selected attractive frame cannot replace complete coverage.
        for image in proof["images"]:
            checked(image)
    applications = []
    # Both current compiler configurations require separate producer and real
    # decoder evidence; component mocks never substitute for source-byte loading.
    for configuration in ("Debug", "RelWithDebInfo"):
        for phase in ("producers", "native"):
            path = OUTPUT / (phase + "-" + configuration + ".json")
            proof = read(path)
            expected_cases = 20 if phase == "producers" else 1
            expected_assertions = 14943 if phase == "producers" else 163073
            # Exact full filters and the original complete decoder are required;
            # stale or partial execution cannot activate a current public binding.
            if not proof["complete"] or proof["cases"] != expected_cases or proof["assertions"] != expected_assertions:
                raise ValueError("incomplete current native producer/decoder proof")
            checked(proof["application"]); checked(proof["terminal"])
            references.append(selectors.art.source.identity(path))
            if phase == "producers":
                for source in proof["sources"]:
                    checked(source)
                checked(proof["decoded_payload"])
            else:
                for key in ("fixture", "expectations", "producer", "native_test", "payload", "copied_payload", "runtime_payload", "source_handoff"):
                    checked(proof[key])
        retained = OUTPUT / ("qualified-space-art-" + configuration + ".exe")
        if retained.exists():
            if selectors.art.source.identity(retained)["sha256"] != proof["application"]["sha256"]:
                raise ValueError("refusing to replace an earlier qualified current application")
        else:
            shutil.copyfile(ROOT / proof["application"]["path"], retained)
        applications.append(dict(observed=proof["application"], retained=selectors.art.source.identity(retained)))
    return packing, read(original), references, applications


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
