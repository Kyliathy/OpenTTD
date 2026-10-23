"""Publish the independently decoded and natively loaded facility-offset correction.

@file ground_station_offset_publication.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHCCB.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Only shared-container checksums and the two corrected facility provenance links
change. Raw artwork, gameplay identities, native ranges and earlier reports remain exact.
"""

import copy
import json

from ground_pack_check_support import ROOT, handoff
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import checked_reference, reference

DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-station-offsets-v1"
RUN = ROOT / "logs/graphics/selected-ground-runtime/20261021-station-offset-01"
BASELINE = (RUN / "before.json").relative_to(ROOT).as_posix()


def retain(source, destination):
    """Retain exactly qualified candidate bytes without replacing an existing different identity.

    @param source Current source-verified candidate file.
    @param destination Explicit repository proof destination.
    @return Exact retained reference.
    """
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists() and destination.read_bytes() != source.read_bytes():
        raise ValueError("corrected publication archive already has another identity")
    if not destination.exists():
        destination.write_bytes(source.read_bytes())
    return reference(destination)


def publish():
    """Join complete source/native/prefix qualification before replacing the ordinary published container.

    @return Exact corrected publication receipt, with actual station foreground acceptance still pending.
    """
    preserved(BASELINE)
    build_path = DIRECTORY / "build-proof.json"
    build = handoff.read_json(build_path.relative_to(ROOT).as_posix())
    # Both complete native encodings and unchanged decoded tuples must be
    # proven before a candidate may replace the ordinary shared container.
    if (build.get("passed") is not True or build["total_native_sprites"] != 22018 or build["changed_native_sprites"] != 432
            or build["unchanged_native_sprites"] != 21586 or len(build["builds"]) != 2):
        raise ValueError("corrected publication lacks complete source/prefix qualification")
    for row in build["inputs"] + [build["report"], build["staging"], build["fixture"]]:
        checked_reference(row)
    for candidate in build["builds"]:
        for row in candidate["decoded_images"] + [candidate["payload"], candidate["hash_file"]]:
            checked_reference(row)
    sha = build["builds"][0]["payload"]["sha256"]
    if build["builds"][1]["payload"]["sha256"] != sha:
        raise ValueError("qualified clean corrected containers differ")
    native = []
    # The exact source fixture and candidate must pass the actual normal
    # application loader in each configuration, not only an offline decoder.
    for configuration in ("Debug", "RelWithDebInfo"):
        path = RUN / ("native-" + configuration.lower() + "-02") / "run.json"
        proof = handoff.read_json(path.relative_to(ROOT).as_posix())
        observed = proof["observation"]
        if (proof.get("passed") is not True or observed["exit_code"] != 0 or observed["native_sprites"] != 22018
                or observed["assertions"] < 440000 or observed["build_proof"] != reference(build_path)
                or observed["fixture"] != build["fixture"] or observed["candidate"]["sha256"] != sha
                or observed["runtime_payload"]["sha256"] != sha):
            raise ValueError("actual corrected native loading is incomplete")
        for row in proof["inputs"] + [observed[key] for key in ("retained_application", "fixture", "candidate", "runtime_payload", "terminal")]:
            checked_reference(row)
        if observed["retained_application"]["sha256"] != observed["application"]["sha256"]:
            raise ValueError("corrected loader retention differs from the actually executed image")
        native.append({"configuration": configuration, "execution": reference(path),
                       "retained_application": observed["retained_application"], "assertions": observed["assertions"],
                       "native_sprites": 22018, "seconds": observed["seconds"]})
    report = handoff.read_json(build["report"]["path"])
    diagnostic = handoff.read_json(report["diagnostic"]["path"])
    before_public_path = checked_reference(diagnostic["archive"]["public"])
    before_public = handoff.read_json(before_public_path.relative_to(ROOT).as_posix())
    proposed = copy.deepcopy(before_public)
    old_sha = diagnostic["archive"]["payload"]["sha256"]
    corrected = set(report["corrected_families"])
    handoff_path = DIRECTORY / "packed-ground-payload.json"
    changed, links = [], []
    # Unaffected published families join the new shared container only by
    # checksum. Exactly two facilities acquire the corrected source handoff;
    # no gameplay/source/geometry field or draft/native ID is reassigned.
    for row in proposed["assets"]:
        packed = row.get("packed", {})
        # Retained orbital rows name the same container native_payload. Read
        # that established alias without rewriting their historical provenance.
        if (packed.get("payload") or packed.get("native_payload")) != "media/baseset/openttd.grf":
            continue
        if packed.get("sha256") == old_sha:
            packed["sha256"] = sha
            changed.append(row["asset_id"])
        if row["asset_id"] in corrected:
            packed["source_payload"] = handoff_path.relative_to(ROOT).as_posix()
            links.append(row["asset_id"])
    if len(changed) != 109 or set(links) != corrected:
        raise ValueError("corrected publication changed the shared native binding domain")
    actual_public = handoff.read_json(handoff.PUBLIC.relative_to(ROOT).as_posix())
    # Idempotence admits only the complete old or complete qualified new public
    # epoch; another edit must never be silently overwritten by publication.
    if actual_public != before_public and actual_public != proposed:
        raise ValueError("public artwork changed outside this exact correction")
    payload = ROOT / "media/baseset/openttd.grf"
    if reference(payload)["sha256"] not in (old_sha, sha):
        raise ValueError("ordinary payload changed outside this exact correction")
    candidate = checked_reference(build["builds"][0]["payload"])
    hash_file = checked_reference(build["builds"][0]["hash_file"])
    retained_payload = retain(candidate, DIRECTORY / "publication/openttd.grf")
    retained_hash = retain(hash_file, DIRECTORY / "publication/openttd.grf.hash")
    packed = copy.deepcopy(report)
    packed.update(passed=True, packed_grf={"path": "media/baseset/openttd.grf", "sha256": sha},
                  native_loading_verified=True, native=native, build_proof=reference(build_path),
                  actual_station_foreground_verification="pending", runtime_bindings_changed=False)
    payload.write_bytes(candidate.read_bytes())
    payload.with_name(payload.name + ".hash").write_bytes(hash_file.read_bytes())
    handoff_path.write_text(json.dumps(packed, indent=2) + "\n", encoding="utf-8")
    handoff.PUBLIC.write_text(json.dumps(proposed, indent=2) + "\n", encoding="utf-8")
    transitions, count = preserved(BASELINE)
    result = {"_documentation": __doc__, "passed": True, "payload": reference(payload), "public": reference(handoff.PUBLIC),
              "packed_handoff": reference(handoff_path), "retained_payload": retained_payload, "retained_hash": retained_hash,
              "native": native, "build_proof": reference(build_path), "corrected_families": sorted(corrected),
              "changed_native_sprites": 432, "unchanged_native_sprites": 21586, "native_loading_verified": True,
              "shared_checksum_updates": 109, "corrected_source_links": 2, "preserved_file_count": count,
              "transitions": transitions, "station_foreground_verification": "pending", "transported_bootstrap_claimed": False,
              "inputs": [reference(ROOT / path) for path in ("src/tests/multilayer/ground_station_offset_publication.py",
                         "src/tests/ground_station_offset_load.cpp", "src/tests/multilayer/ground_station_offset_native_checks.py")]}
    (DIRECTORY / "publication.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


if __name__ == "__main__":
    result = publish()
    print(json.dumps({key: result[key] for key in ("passed", "changed_native_sprites", "unchanged_native_sprites", "native_loading_verified")}))
