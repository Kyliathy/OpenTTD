"""Retain current native conventional station and terminal source binding evidence.

@file qualify_ground_station_selection.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FG.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Command-created facilities and staged native loading-list boundaries qualify drawing;
actual viewport acceptance and transported lifecycle remain separate tasks.
"""

import argparse
import json
import re

from ground_pack_check_support import ROOT, handoff
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import checked_reference, reference

DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-consumers-v1"
RUN = ROOT / "logs/graphics/selected-ground-consumers/20261021-stations-01"
BASELINE = (RUN / "before.json").relative_to(ROOT).as_posix()
OWNERS = ["src/conventional_station_art.h", "src/conventional_station_art.cpp", "src/conventional_terminal_art.h",
          "src/conventional_terminal_art.cpp", "src/table/station_land.h", "src/sprite.h",
          "src/tests/conventional_station_art.cpp", "src/tests/conventional_terminal_art.cpp"]
FAMILIES = ["facility.rail-station", "facility.road-terminal"]


def geometry_export():
    """Hash the unchanged public layout producer separately from the executing renderer body.

    @return Actual symbol/body identity within station_cmd.cpp, whose rendering consumer is modified here.
    """
    text = (ROOT / "src/station_cmd.cpp").read_text(encoding="utf-8")
    start = text.index("const DrawTileSprites *GetStationTileLayout(")
    end = text.index("\n}\n", start) + 2
    import hashlib
    return {"path": "src/station_cmd.cpp", "symbol": "GetStationTileLayout",
            "sha256": hashlib.sha256(text[start:end].encode("utf-8")).hexdigest()}


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
            raise ValueError("station native boundary is incomplete")
        for row in proof["inputs"] + [proof["application"], observed["terminal"]]:
            checked_reference(row)
        terminal = handoff.bound(observed["terminal"]["path"]).read_text(encoding="utf-8")
        expected = f'All tests passed ({observed["assertions"]} assertions in {cases} test cases)'
        if expected not in terminal:
            raise ValueError("station actual native completion is absent")
        application = handoff.bound(proof["application"]["path"])
        retained = DIRECTORY / ("station-" + phase + "-bin") / configuration / application.name
        retained.parent.mkdir(parents=True, exist_ok=True)
        if retained.exists() and retained.read_bytes() != application.read_bytes():
            raise ValueError("station retained test image already has another identity")
        if not retained.exists():
            retained.write_bytes(application.read_bytes())
        result.append({"configuration": configuration, "execution": reference(path),
                       "executed_application": proof["application"], "retained_application": reference(retained),
                       "terminal": observed["terminal"], "cases": cases, "assertions": observed["assertions"]})
    return result


def qualify(producer):
    """Publish only the requested native producer or two selected facility bindings.

    @param producer True to freeze the before-binding native capability qualification.
    @return Completed bounded qualification without viewport or transport claims.
    """
    producer_path = DIRECTORY / "station-producer-boundary.json"
    if producer:
        result = {"_documentation": __doc__, "passed": True,
                  "native": native("producer", "[conventional-station-art],[conventional-terminal-art]", 6),
                  "owner_inputs": [reference(handoff.bound(path)) for path in OWNERS], "geometry_export": geometry_export(),
                  "executing_renderer_before": reference(ROOT / "src/station_cmd.cpp")}
        producer_path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
        return result
    boundary = handoff.read_json(producer_path.relative_to(ROOT).as_posix())
    for row in boundary["owner_inputs"]:
        checked_reference(row)
    for row in boundary["native"]:
        checked_reference(row["retained_application"])
    if boundary["geometry_export"] != geometry_export():
        raise ValueError("actual native station layout producer changed during consumer binding")
    transitions, count = preserved(BASELINE)
    inputs = ["src/ground_station_sprite.h", "src/ground_station_sprite.cpp", "src/tests/ground_station_selection.cpp",
              "src/station_cmd.cpp", "CMakeLists.txt", "src/CMakeLists.txt",
              "logs/proofs/mlm/native-art/ground/station-source-manifest.json",
              "logs/proofs/mlm/native-art/ground/terminal-source-manifest.json"]
    result = {"_documentation": __doc__, "passed": True, "families": FAMILIES,
              "native": native("selection", "[ground-station-selection]", 6),
              "producer_boundary": reference(producer_path), "inputs": [reference(handoff.bound(path)) for path in inputs],
              "baseline": reference(handoff.bound(BASELINE)), "preserved_file_count": count, "transitions": transitions,
              "native_selection_verified": True, "viewport_verified": False, "transported_bootstrap_claimed": False}
    qualification = DIRECTORY / "station-selection.json"
    qualification.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    dispatch_path = ROOT / "media/multilayer/contracts/ground-consumer-dispatch.json"
    dispatch = handoff.read_json(dispatch_path.relative_to(ROOT).as_posix())
    # Promote only real selected station/terminal bindings; tube and later operating
    # machinery retain their own consumer groups and native capability owners.
    for row in dispatch["families"]:
        if row["asset_id"] in FAMILIES:
            row.update(dispatch="current_bound", binding_header="src/ground_station_sprite.h",
                       binding_symbol="GetGroundStationTileLayout",
                       binding_qualification=reference(qualification))
    dispatch_path.write_text(json.dumps(dispatch, indent=2) + "\n", encoding="utf-8")
    transitions, count = preserved(BASELINE)
    publication = {"_documentation": __doc__, "passed": True, "qualification": reference(qualification),
                   "dispatch": reference(dispatch_path), "transitions": transitions, "preserved_file_count": count}
    (DIRECTORY / "station-selection-publication.json").write_text(json.dumps(publication, indent=2) + "\n", encoding="utf-8")
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--producer", action="store_true", help="Retain current facility geometry boundary before dependent art changes")
    args = parser.parse_args()
    print(json.dumps({key: value for key, value in qualify(args.producer).items() if key in ("passed", "families")}))
