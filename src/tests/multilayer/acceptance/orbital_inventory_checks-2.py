"""Freeze both passing native packet/work adapter scopes and their actual process identities.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10B
Isolated consumed frames and declared packet inputs prove the adapter; native paid scheduling/save acceptance remains10C/11/12.
"""
import argparse
import json
from pathlib import Path
import re

from space_preview_checks import ROOT, identity

SOURCES = ("src/CMakeLists.txt", "src/tests/CMakeLists.txt", "src/orbital_recipe.h", "src/orbital_recipe.cpp",
           "src/orbital_station.h", "src/orbital_station.cpp", "src/space_structure.h", "src/space_structure.cpp",
           "src/saveload/space_structure_sl.cpp", "src/saveload/space_cargo_preview.h", "src/saveload/space_cargo_preview.cpp",
           "src/layer_recipe_work.h", "src/layer_recipe_work.cpp", "src/multilayer_industry.h", "src/layer_industry_recipe.cpp",
           "src/multilayer_recipe.h", "src/multilayer_recipe.cpp", "src/multilayer_space_content.cpp", "src/utility_power.h", "src/utility_power.cpp",
           "src/cargopacket.h", "src/cargopacket.cpp", "src/hub_cargo.h", "src/hub_cargo.cpp", "src/tests/orbital_recipe_inventory.cpp",
           "src/tests/orbital_module_activation.cpp", "src/tests/layer_recipe_work.cpp", "src/tests/layer_recipe_batch.cpp",
           "src/tests/excavation_fixture.h", "src/tests/layer_save_fixture.h", "src/tests/multilayer/acceptance/orbital_inventory_checks-1.py",
           "src/tests/multilayer/acceptance/orbital_inventory_checks-2.py", "src/tests/multilayer/acceptance/space_preview_checks.py",
           "src/tests/multilayer/support/runner.py", "src/tests/multilayer/support/artifact_paths.py", "src/tests/test_artifact_paths.cpp")
TAGS = "[orbital-activation-contract],[orbital-service-contract],[orbital-freight-contract],[layer-recipe-batch],[utility-power-recipe],[layer-recipe-work],[orbital-recipe-inventory]"


def read(path):
    """Read an exact retained report.

    @param path Explicit original JSON path, with no latest-file selection.
    @return Parsed unchanged evidence.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def inspect(value, references):
    """Check every retained original identity recursively.

    @param value Original report or identity tree.
    @param references Shared deduplicated direct artifact identities.
    """
    if isinstance(value, dict):
        # An actual executable, input or process must retain its original bytes; another source graph cannot replace dated evidence.
        if "path" in value and "sha256" in value:
            actual = identity(ROOT / value["path"])
            assert actual == {key: value[key] for key in ("path", "sha256")}
            assert references.get(actual["path"], actual) == actual
            references[actual["path"]] = actual
        # All nested retained artifacts belong to the same explicit process membership and require independent identity checks.
        for child in value.values():
            inspect(child, references)
    elif isinstance(value, list):
        for child in value:
            inspect(child, references)


def configuration(name, selector, references):
    """Qualify the actual adapter process and its passing terminal output.

    @param name Exact test build configuration.
    @param selector Explicit completed inventory-scope report selector.
    @param references Shared checked artifact identities.
    @return Frozen bounded process, assertions and executable evidence.
    """
    selector = selector.resolve()
    selected = read(selector)
    assert selected["configuration"] == name and selected["scope"] == "inventory" and selected["complete"] and len(selected["reports"]) == 1
    path = ROOT / selected["reports"][0]
    proof = read(path)
    inspect(proof, references)
    assert proof["configuration"] == name and proof["scope"] == "inventory" and proof["assertions"] == 3549 and proof["cases"] == 26 and len(proof["native_reports"]) == 1
    process_reference = proof["native_reports"][0]
    process = read(ROOT / process_reference["path"])
    inspect(process, references)
    assert process["configuration"] == name and process["exit_status"] == 0 and process["scenario"] == "orbital-inventory-inventory" and process["argv"][1:] == [TAGS]
    assert process["executable"] == identity(ROOT / "build" / name / "openttd_test.exe")
    log = identity(ROOT / "logs/space/orbital-inventory-inventory" / process["run_id"] / "inventory.log")
    inspect(log, references)
    evidence = (ROOT / log["path"]).read_text(encoding="utf-8", errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    assert match and (int(match[1]), int(match[2])) == (proof["assertions"], proof["cases"])
    return dict(configuration=name, selector=identity(selector), proof=identity(path), assertions=proof["assertions"],
                cases=proof["cases"], process=process_reference, terminal=log, executable=process["executable"])


def main():
    """Publish matching successful bounded adapter scopes; failed diagnostics cannot authorize completion."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, required=True)
    parser.add_argument("--debug", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    target = args.output.resolve()
    assert not target.exists()
    references = {}
    release = configuration("RelWithDebInfo", args.release, references)
    debug = configuration("Debug", args.debug, references)
    assert (release["assertions"], release["cases"]) == (debug["assertions"], debug["cases"])
    handoffs = ("logs/orbital-recipe-work-verification.json", "logs/heavy-network-verification.json")
    result = dict(_documentation=__doc__, configurations=[release, debug], sources=[identity(ROOT / path) for path in SOURCES],
                  direct_references=list(references.values()), dated_producer_indexes=[identity(ROOT / path) for path in handoffs],
                  saved_wire_layout_changed=False, native_orbital_processing_acceptance=False, procurement_acceptance=False)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), identity(target)["sha256"], flush=True)


if __name__ == "__main__":
    main()
