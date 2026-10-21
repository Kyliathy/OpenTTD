"""Retain actual selected-mine binding evidence and publish only its seven dispatch dispositions.

@file qualify_ground_mine_selection.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This qualifies current selection against real command-body fixtures. Actual
viewport/ordinary-game and transported lifecycle acceptance remain later groups.
"""

import json

from ground_pack_check_support import ROOT, handoff
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import checked_reference, reference

DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-consumers-v1"
QUALIFICATION = DIRECTORY / "mine-selection.json"
BASELINE = "logs/graphics/selected-ground-consumers/20261021-mines-01/before-dispatch.json"
RUN = ROOT / "logs/graphics/selected-ground-consumers/20261021-mines-01"
DISPATCH = ROOT / "media/multilayer/contracts/ground-consumer-dispatch.json"
FAMILIES = ["mine.COAL", "mine.IORE", "mine.CORE", "mine.SALT", "mine.LITH", "mine.COBT", "mine.RARE"]


def qualify():
    """Require actual current producer and selection cases before promoting the seven selected rows.

    @return Source/code/application-bound mine selection handoff, without a game-viewport claim.
    """
    producer_path = DIRECTORY / "mine-producer-boundary.json"
    producer = handoff.read_json(producer_path.relative_to(ROOT).as_posix())
    for row in producer["owner_inputs"]:
        checked_reference(row)
    for row in producer["native"]:
        checked_reference(row["retained_application"])
    native = []
    # Both actual native builds must execute the real seven-family ownership,
    # phase/part, blocked/depleted and refusal/purity cases. Retain their exact
    # tested bytes before the next processor group relinks its working target.
    for configuration in ("Debug", "RelWithDebInfo"):
        path = RUN / ("selection-" + configuration.lower()) / "run.json"
        proof = handoff.read_json(path.relative_to(ROOT).as_posix())
        observed = proof["observation"]
        if (proof.get("passed") is not True or proof["configuration"] != configuration
                or observed["filter"] != "[ground-mine-selection]" or observed["cases"] != 6
                or observed["assertions"] != 1965 or observed["exit_code"] != 0):
            raise ValueError("ground seven-mine current native selection qualification is incomplete")
        for row in proof["inputs"] + [proof["application"], observed["terminal"]]:
            checked_reference(row)
        text = handoff.bound(observed["terminal"]["path"]).read_text(encoding="utf-8")
        if "All tests passed (1965 assertions in 6 test cases)" not in text:
            raise ValueError("ground seven-mine actual terminal result is absent")
        application = handoff.bound(proof["application"]["path"])
        retained = DIRECTORY / "mine-selection-bin" / configuration / application.name
        retained.parent.mkdir(parents=True, exist_ok=True)
        if retained.exists() and retained.read_bytes() != application.read_bytes():
            raise ValueError("ground selected-mine retained image already has another identity")
        if not retained.exists():
            retained.write_bytes(application.read_bytes())
        native.append({"configuration": configuration, "execution": reference(path),
                       "executed_application": proof["application"], "retained_application": reference(retained),
                       "terminal": observed["terminal"], "cases": 6, "assertions": 1965})
    transitions, count = preserved(BASELINE)
    inputs = ["src/ground_mine_sprite.h", "src/ground_mine_sprite.cpp", "src/tests/ground_mine_selection.cpp",
              "src/multilayer_sprite.h", "src/multilayer_sprite.cpp", "src/CMakeLists.txt", "CMakeLists.txt",
              "src/tests/multilayer/ground_consumer_dispatch_checks.py"]
    result = {"_documentation": __doc__, "passed": True, "families": FAMILIES, "native": native,
              "producer_boundary": reference(producer_path), "inputs": [reference(handoff.bound(path)) for path in inputs],
              "baseline": reference(handoff.bound(BASELINE)), "preserved_file_count": count, "transitions": transitions,
              "native_selection_verified": True, "viewport_verified": False, "transported_bootstrap_claimed": False}
    QUALIFICATION.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    dispatch = handoff.read_json(DISPATCH.relative_to(ROOT).as_posix())
    # Only the exact seven selected mine entries receive this actual current
    # binding. Other current and later-owned consumers keep their disposition.
    for row in dispatch["families"]:
        if row["asset_id"] in FAMILIES:
            row.update(dispatch="current_bound", binding_header="src/ground_mine_sprite.h", binding_symbol="GetGroundMineSprite",
                       binding_qualification=reference(QUALIFICATION))
    DISPATCH.write_text(json.dumps(dispatch, indent=2) + "\n", encoding="utf-8")
    transitions, count = preserved(BASELINE)
    publication = {"_documentation": __doc__, "passed": True, "qualification": reference(QUALIFICATION),
                   "dispatch": reference(DISPATCH), "transitions": transitions, "preserved_file_count": count}
    (DIRECTORY / "mine-selection-publication.json").write_text(json.dumps(publication, indent=2) + "\n", encoding="utf-8")
    return result


if __name__ == "__main__":
    print(json.dumps({key: value for key, value in qualify().items() if key in ("passed", "families", "native_selection_verified", "viewport_verified")}))
