"""Freeze both actual shared staged-work producer scopes before orbital packet/runtime consumers begin.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10
The proof binds current sources, actual executables and terminal logs. Native orbital inventory, processing saves and useful products remain Groups10B/10C/11/12.
"""
import argparse
import json
from pathlib import Path
import re

from space_preview_checks import ROOT, identity

SOURCES = ("src/CMakeLists.txt", "src/tests/CMakeLists.txt", "src/multilayer_industry.h", "src/layer_industry_recipe.cpp", "src/layer_recipe_work.h", "src/layer_recipe_work.cpp",
           "src/multilayer_recipe.h", "src/multilayer_recipe.cpp", "src/utility_power.h", "src/utility_power.cpp", "src/multilayer_utility.h", "src/multilayer_utility.cpp",
           "src/tests/layer_recipe_work.cpp", "src/tests/layer_recipe_batch.cpp", "src/tests/utility_power_fixture.h", "src/tests/excavation_fixture.h",
           "src/tests/multilayer/acceptance/orbital_recipe_checks-1.py", "src/tests/multilayer/acceptance/orbital_recipe_checks-2.py", "src/tests/multilayer/acceptance/space_preview_checks.py",
           "src/tests/multilayer/support/runner.py", "src/tests/multilayer/support/artifact_paths.py", "src/tests/test_artifact_paths.cpp")
TAGS = "[layer-recipe-batch],[utility-power-recipe],[utility-power-ledger],[utility-power-mutation],[utility-power-effects],[utility-power-stage],[layer-recipe-work]"


def read(path):
    """Read one explicit retained original JSON proof.

    @param path Exact repository selector/report path.
    @return Original parsed evidence, without newest-file selection.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def inspect(value, references):
    """Verify every exact retained identity in one actual process/acceptance report.

    @param value Original JSON value containing nested artifact identities.
    @param references Shared checked direct identities, deduplicated by repository path.
    """
    if isinstance(value, dict):
        # Every explicitly identified input, executable and output must still match its original bytes; copied report text cannot replace an actual passing process.
        if "path" in value and "sha256" in value:
            actual = identity(ROOT / value["path"]); assert actual == {key: value[key] for key in ("path", "sha256")}
            assert references.get(actual["path"], actual) == actual; references[actual["path"]] = actual
        for child in value.values():
            inspect(child, references)
    elif isinstance(value, list):
        for child in value:
            inspect(child, references)


def configuration(name, selector, references):
    """Qualify the exact complete staged-work scope and its actual current test process.

    @param name Matching actual current test executable configuration.
    @param selector Explicit accepted work-scope membership.
    @param references Shared checked direct artifact identities.
    @return Bound actual assertion/case/process/terminal evidence for this configuration.
    """
    selector = selector.resolve(); selected = read(selector); assert selected["configuration"] == name and selected["scope"] == "work" and selected["complete"] and len(selected["reports"]) == 1
    path = ROOT / selected["reports"][0]; proof = read(path); inspect(proof, references)
    assert proof["configuration"] == name and proof["scope"] == "work" and proof["assertions"] == 2447 and proof["cases"] == 23 and len(proof["native_reports"]) == 1
    process_reference = proof["native_reports"][0]; process = read(ROOT / process_reference["path"]); inspect(process, references)
    assert process["configuration"] == name and process["exit_status"] == 0 and process["scenario"] == "orbital-recipe-work" and process["argv"][1:] == [TAGS]
    assert process["executable"] == identity(ROOT / "build" / name / "openttd_test.exe")
    log = identity(ROOT / "logs/space/orbital-recipe-work" / process["run_id"] / "work.log"); inspect(log, references)
    evidence = (ROOT / log["path"]).read_text(encoding="utf-8", errors="replace"); match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    assert match and (int(match[1]), int(match[2])) == (proof["assertions"], proof["cases"])
    return dict(configuration=name, selector=identity(selector), proof=identity(path), assertions=proof["assertions"], cases=proof["cases"], process=process_reference, terminal=log, executable=process["executable"])


def main():
    """Publish only both actual passing current scopes; earlier failed diagnostics cannot supply acceptance membership."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--release", type=Path, required=True); parser.add_argument("--debug", type=Path, required=True); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    release = configuration("RelWithDebInfo", args.release, references); debug = configuration("Debug", args.debug, references)
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group10; shared private recipe-bound progress earned by actual atomic electrical work, one-opportunity/split-grant limits, original complete-batch settlement and unchanged whole-grant callers. Unit facilities and external sources are explicit isolated fixtures; native orbital inventory/runtime/save integration remains10B/10C/11/12.",
                  configurations=[release, debug], sources=[identity(ROOT / path) for path in SOURCES], direct_references=list(references.values()),
                  saved_wire_layout_changed=False, native_orbital_processing_acceptance=False, procurement_acceptance=False)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(target.relative_to(ROOT).as_posix(), identity(target)["sha256"], flush=True)


if __name__ == "__main__":
    main()
