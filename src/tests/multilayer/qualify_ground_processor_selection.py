"""Retain current four-processor source selection and publish only its native dispatch dispositions.

@file qualify_ground_processor_selection.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FD.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Actual command-body/native inventory fixtures qualify source selection only;
viewport, earned transport and later plant lifecycle remain their owning groups.
"""

import json

from ground_pack_check_support import ROOT, handoff
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import checked_reference, reference

DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-consumers-v1"
QUALIFICATION = DIRECTORY / "processor-selection.json"
BASELINE = "logs/graphics/selected-ground-consumers/20261021-processors-01/before.json"
RUN = ROOT / "logs/graphics/selected-ground-consumers/20261021-processors-01"
DISPATCH = ROOT / "media/multilayer/contracts/ground-consumer-dispatch.json"
FAMILIES = ["facility.metal-refinery", "facility.chemical-works", "facility.battery-works", "facility.electronics-works"]


def qualify():
    """Require current producer/selection cases and retain the exact actually executed images.

    @return Source/code/application-bound four-family selection handoff without viewport acceptance.
    """
    producer_path = DIRECTORY / "processor-producer-boundary.json"
    producer = handoff.read_json(producer_path.relative_to(ROOT).as_posix())
    for row in producer["owner_inputs"]:
        checked_reference(row)
    for row in producer["native"]:
        checked_reference(row["retained_application"])
    native = []
    # Each configuration must execute all current owner/input/output/phase/
    # refusal cases. Keep exact tested bytes so later freight groups cannot
    # invalidate this completed epoch by relinking their working executable.
    for configuration in ("Debug", "RelWithDebInfo"):
        path = RUN / ("selection-" + configuration.lower()) / "run.json"
        proof = handoff.read_json(path.relative_to(ROOT).as_posix())
        observed = proof["observation"]
        if (proof.get("passed") is not True or proof["configuration"] != configuration
                or observed["filter"] != "[ground-processor-selection]" or observed["cases"] != 6
                or observed["assertions"] != 1334 or observed["exit_code"] != 0):
            raise ValueError("ground four-processor current native selection qualification is incomplete")
        for row in proof["inputs"] + [proof["application"], observed["terminal"]]:
            checked_reference(row)
        text = handoff.bound(observed["terminal"]["path"]).read_text(encoding="utf-8")
        if "All tests passed (1334 assertions in 6 test cases)" not in text:
            raise ValueError("ground four-processor actual native terminal result is absent")
        application = handoff.bound(proof["application"]["path"])
        retained = DIRECTORY / "processor-selection-bin" / configuration / application.name
        retained.parent.mkdir(parents=True, exist_ok=True)
        if retained.exists() and retained.read_bytes() != application.read_bytes():
            raise ValueError("ground processor retained test image already has another identity")
        if not retained.exists():
            retained.write_bytes(application.read_bytes())
        native.append({"configuration": configuration, "execution": reference(path),
                       "executed_application": proof["application"], "retained_application": reference(retained),
                       "terminal": observed["terminal"], "cases": 6, "assertions": 1334})
    transitions, count = preserved(BASELINE)
    inputs = ["src/ground_processor_sprite.h", "src/ground_processor_sprite.cpp", "src/tests/ground_processor_selection.cpp",
              "src/multilayer_sprite.h", "src/multilayer_sprite.cpp", "src/CMakeLists.txt", "CMakeLists.txt"]
    result = {"_documentation": __doc__, "passed": True, "families": FAMILIES, "native": native,
              "producer_boundary": reference(producer_path), "inputs": [reference(handoff.bound(path)) for path in inputs],
              "baseline": reference(handoff.bound(BASELINE)), "preserved_file_count": count, "transitions": transitions,
              "native_selection_verified": True, "viewport_verified": False, "transported_bootstrap_claimed": False}
    QUALIFICATION.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    dispatch = handoff.read_json(DISPATCH.relative_to(ROOT).as_posix())
    # Promote only these four actual Surface source bindings. Unselected
    # processors and all tube/geothermal producers keep their existing owners.
    for row in dispatch["families"]:
        if row["asset_id"] in FAMILIES:
            row.update(dispatch="current_bound", binding_header="src/ground_processor_sprite.h",
                       binding_symbol="GetGroundProcessorSprite", binding_qualification=reference(QUALIFICATION))
    DISPATCH.write_text(json.dumps(dispatch, indent=2) + "\n", encoding="utf-8")
    transitions, count = preserved(BASELINE)
    publication = {"_documentation": __doc__, "passed": True, "qualification": reference(QUALIFICATION),
                   "dispatch": reference(DISPATCH), "transitions": transitions, "preserved_file_count": count}
    (DIRECTORY / "processor-selection-publication.json").write_text(json.dumps(publication, indent=2) + "\n", encoding="utf-8")
    return result


if __name__ == "__main__":
    print(json.dumps({key: value for key, value in qualify().items() if key in ("passed", "families", "native_selection_verified", "viewport_verified")}))
