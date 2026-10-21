"""Retain source-backed native road carrier selection without changing transport owners.

@file qualify_ground_road_selection.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FF.
@see docs/zz-reach2/architecture/vehicles/archi-vehicles.md.
Command-created freight and staged packet boundary fixtures qualify drawing;
actual viewport acceptance and transported lifecycle remain separate tasks.
"""

import json
import re

from ground_pack_check_support import ROOT, handoff
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import checked_reference, reference

DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-consumers-v1"
RUN = ROOT / "logs/graphics/selected-ground-consumers/20261021-road-01"
BASELINE = (RUN / "before.json").relative_to(ROOT).as_posix()
FAMILIES = ["vehicle.model-07", "purchase.model-07", "vehicle.model-08", "purchase.model-08"]


def registrations():
    """Compare native art registration and expected cases against accepted source metadata.

    @return Independent source registration identities, without relying on the selector's offset formula.
    """
    source = handoff.read_json("logs/proofs/mlm/native-art/ground/vehicle-model-07-source-manifest.json")
    rows = source["atlases"][0]["sprites"]
    registered = sorted({(row["tile_variant"], row["cargo_label"]) for row in rows})
    expected = [(index, label) for index, (_, label) in enumerate(registered)]
    if registered != expected:
        raise ValueError("accepted open truck source registration is not contiguous")
    labels = [label for _, label in registered]
    owners = [(ROOT / "src/ground_road_sprite.cpp", "static constexpr std::array labels", "const auto found"),
              (ROOT / "src/tests/ground_road_selection.cpp", "const std::array labels", "CheckRoadSourceMatrix(7")]
    # Read each independently declared C++ registration against actual accepted
    # sprite metadata, so a shared mistaken order in test and selector cannot
    # conceal a wrong cargo view. This table grants no cargo/refit permission.
    for path, start, end in owners:
        text = path.read_text(encoding="utf-8").split(start, 1)[1].split(end, 1)[0]
        if re.findall(r'CargoLabel\{"([A-Z0-9]{4})"\}', text) != labels:
            raise ValueError("native road source cargo registration differs from accepted pixels")
    sealed = handoff.read_json("logs/proofs/mlm/native-art/ground/vehicle-model-08-source-manifest.json")
    variants = {(row["tile_variant"], tuple(row["cargo_labels"])) for row in sealed["atlases"][0]["sprites"]}
    if variants != {(0, ("URAN", "RARE"))}:
        raise ValueError("sealed carrier source no longer has its qualified shared closed variant")
    return {"open": registered, "sealed": [[variant, list(cargo)] for variant, cargo in variants]}


def native(phase, tags, cases):
    """Validate focused real executions and retain byte-identical tested images.

    @param phase Producer or selection run directory prefix.
    @param tags Exact requested native Catch filter.
    @param cases Required current-owner test count.
    @return Execution receipts identifying both invoked and retained applications.
    """
    result = []
    # Retain both actually tested configurations before any later consumer
    # relinks the working target; archived code cannot inherit a newer result.
    for configuration in ("Debug", "RelWithDebInfo"):
        path = RUN / (phase + "-" + configuration.lower()) / "run.json"
        proof = handoff.read_json(path.relative_to(ROOT).as_posix())
        observed = proof["observation"]
        # Require the exact boundary and completed native process, rather than
        # accepting a successful empty filter or an unrelated fixture's result.
        if (proof.get("passed") is not True or proof["configuration"] != configuration
                or observed["filter"] != tags or observed["cases"] != cases or observed["exit_code"] != 0):
            raise ValueError("road native boundary is incomplete")
        for row in proof["inputs"] + [proof["application"], observed["terminal"]]:
            checked_reference(row)
        terminal = handoff.bound(observed["terminal"]["path"]).read_text(encoding="utf-8")
        expected = f'All tests passed ({observed["assertions"]} assertions in {cases} test cases)'
        if expected not in terminal:
            raise ValueError("road actual native completion is absent")
        application = handoff.bound(proof["application"]["path"])
        retained = DIRECTORY / ("road-" + phase + "-bin") / configuration / application.name
        retained.parent.mkdir(parents=True, exist_ok=True)
        if retained.exists() and retained.read_bytes() != application.read_bytes():
            raise ValueError("road retained test image already has another identity")
        if not retained.exists():
            retained.write_bytes(application.read_bytes())
        result.append({"configuration": configuration, "execution": reference(path),
                       "executed_application": proof["application"], "retained_application": reference(retained),
                       "terminal": observed["terminal"], "cases": cases, "assertions": observed["assertions"]})
    return result


def qualify():
    """Publish only actual selected model7/8 bindings using the unchanged current fleet owner proof.

    @return Bounded road qualification without viewport or driven-transport claims.
    """
    producer_path = DIRECTORY / "rail-producer-boundary.json"
    boundary = handoff.read_json(producer_path.relative_to(ROOT).as_posix())
    for row in boundary["owner_inputs"]:
        checked_reference(row)
    for row in boundary["native"]:
        checked_reference(row["retained_application"])
    transitions, count = preserved(BASELINE)
    inputs = ["src/ground_road_sprite.h", "src/ground_road_sprite.cpp", "src/tests/ground_road_selection.cpp",
              "src/multilayer_sprite.h", "src/multilayer_sprite.cpp", "src/roadveh_cmd.cpp", "CMakeLists.txt", "src/CMakeLists.txt",
              "logs/proofs/mlm/native-art/ground/vehicle-model-07-source-manifest.json",
              "logs/proofs/mlm/native-art/ground/vehicle-model-08-source-manifest.json"]
    result = {"_documentation": __doc__, "passed": True, "families": FAMILIES,
              "native": native("selection", "[ground-road-selection]", 6),
              "source_registrations": registrations(),
              "producer_reuse": "Current EIDS/capability/filter proof covers actual models2..8; unchanged owners and retained images verified before road edits. No repeated producer test.",
              "producer_boundary": reference(producer_path), "inputs": [reference(handoff.bound(path)) for path in inputs],
              "baseline": reference(handoff.bound(BASELINE)), "preserved_file_count": count, "transitions": transitions,
              "native_selection_verified": True, "viewport_verified": False, "transported_bootstrap_claimed": False}
    qualification = DIRECTORY / "road-selection.json"
    qualification.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    dispatch_path = ROOT / "media/multilayer/contracts/ground-consumer-dispatch.json"
    dispatch = handoff.read_json(dispatch_path.relative_to(ROOT).as_posix())
    # Promote only real selected road bindings; tube and later operating
    # machinery retain their own consumer groups and native capability owners.
    for row in dispatch["families"]:
        if row["asset_id"] in FAMILIES:
            row.update(dispatch="current_bound", binding_header="src/ground_road_sprite.h",
                       binding_symbol="GetGroundRoadPurchaseSprite" if row["asset_id"].startswith("purchase.") else "GetGroundRoadSprite",
                       binding_qualification=reference(qualification))
    dispatch_path.write_text(json.dumps(dispatch, indent=2) + "\n", encoding="utf-8")
    transitions, count = preserved(BASELINE)
    publication = {"_documentation": __doc__, "passed": True, "qualification": reference(qualification),
                   "dispatch": reference(dispatch_path), "transitions": transitions, "preserved_file_count": count}
    (DIRECTORY / "road-selection-publication.json").write_text(json.dumps(publication, indent=2) + "\n", encoding="utf-8")
    return result


if __name__ == "__main__":
    print(json.dumps({key: value for key, value in qualify().items() if key in ("passed", "families")}))
