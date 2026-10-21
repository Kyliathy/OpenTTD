"""Retain the exact executed typed-ground contract and publish its qualified dispatch handoff.

@file qualify_ground_pose_contract.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native execution receipts keep their actual invocation paths. Byte-exact copies
of those executed images allow later current-consumer target rebuilds.
"""

import json

from ground_pack_check_support import ROOT, handoff
from ground_sprite_fixture import SUMMARY
from run_ground_consumer_checks import BASELINE, preserved
from tools.ground_payload_binding import checked_reference, reference

DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-consumers-v1"
QUALIFICATION = DIRECTORY / "pose-contract.json"
RUN = ROOT / "logs/graphics/selected-ground-consumers/20261021-types-01"
DISPATCH = ROOT / "media/multilayer/contracts/ground-consumer-dispatch.json"


def qualify():
    """Require both actual source-tuple cases and retain their real executed image bytes.

    @return Exact native type/ID qualification with unchanged simulation/packing epochs.
    """
    source = handoff.read_json(SUMMARY.relative_to(ROOT).as_posix())
    for name in ("fixture", "producer", "source_manifest", "api_header"):
        checked_reference(source[name])
    native = []
    # A completed source/API handoff needs two distinct actual builds/cases,
    # not a copied terminal or a declared compiler flag. Keep tested images
    # stable while later consumer groups legitimately relink their own target.
    for configuration in ("Debug", "RelWithDebInfo"):
        directory = RUN / configuration.lower()
        build = handoff.read_json((directory / "build.json").relative_to(ROOT).as_posix())
        proof_path = directory / "run.json"
        proof = handoff.read_json(proof_path.relative_to(ROOT).as_posix())
        observed = proof["observation"]
        if (proof.get("passed") is not True or proof["configuration"] != configuration
                or observed["cases"] != 2 or observed["assertions"] != 171349
                or observed["filter"] != "[ground-sprite-contract]" or observed["exit_code"] != 0
                or proof["application"] != build["application"]):
            raise ValueError("ground typed handoff lacks current dual-configuration native qualification")
        for row in proof["inputs"] + [proof["application"], observed["terminal"]]:
            checked_reference(row)
        text = handoff.bound(observed["terminal"]["path"]).read_text(encoding="utf-8")
        if ("GROUND_POSE_CONTRACT source_tuples=14254 families=47 native_start=7791 native_bound=22045 independent_ids=exact" not in text
                or "All tests passed (171349 assertions in 2 test cases)" not in text):
            # Actual independent-tuple and Catch markers must both survive;
            # another filtered test cannot stand in for the compiled export.
            raise ValueError("ground typed handoff actual independent source terminal is absent")
        application = handoff.bound(proof["application"]["path"])
        retained = DIRECTORY / "pose-contract-bin" / configuration / application.name
        retained.parent.mkdir(parents=True, exist_ok=True)
        if retained.exists() and retained.read_bytes() != application.read_bytes():
            raise ValueError("ground typed contract retained image already has another identity")
        if not retained.exists():
            retained.write_bytes(application.read_bytes())
        native.append({"configuration": configuration, "execution": reference(proof_path),
                       "executed_application": proof["application"], "retained_application": reference(retained),
                       "terminal": observed["terminal"], "cases": 2, "assertions": 171349})
    transitions, count = preserved(BASELINE)
    dispatch = handoff.read_json(DISPATCH.relative_to(ROOT).as_posix())
    boundary = dispatch["proposed_pose_api"]
    if boundary["header"] != "src/ground_sprite.h" or boundary["symbols"] != ["GetGroundSprite"]:
        # Qualification owns only this exact type/signature handoff; a changed
        # dispatch owner cannot redirect it to an uncompiled alternate API.
        raise ValueError("ground typed dispatch handoff was redirected")
    boundary.update(implemented=True, qualification=QUALIFICATION.relative_to(ROOT).as_posix())
    DISPATCH.write_text(json.dumps(dispatch, indent=2) + "\n", encoding="utf-8")
    transitions, count = preserved(BASELINE)
    result = {"_documentation": __doc__, "passed": True, "source_expectations": reference(SUMMARY),
              "api_header": source["api_header"], "fixture": source["fixture"], "native": native,
              "dispatch": reference(DISPATCH), "baseline": reference(handoff.bound(BASELINE)),
              "preserved_file_count": count, "transitions": transitions,
              "families": 47, "source_tuples": 14254, "native_signature_verified": True,
              "simulation_behavior_claimed": False, "runtime_activated": False}
    QUALIFICATION.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


if __name__ == "__main__":
    print(json.dumps({key: value for key, value in qualify().items() if key in ("passed", "families", "source_tuples", "native_signature_verified")}))
