"""Verify the original orbital refusal boundaries in one bounded focused burst.

@file orbital_final_art_checks-6.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13L.2
Declared component fixtures test refusal semantics only. Immutable qualified paid
producers supply native delivery/reservation/congestion/removal acceptance; their
historical executable/source graphs are not replayed or promoted to current builds.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from space_preview_checks import ROOT, identity
from artifact_paths import create_run_context
from runner import run_native

TAGS = ("[space-construction-contract],[orbital-core-contract],[orbital-assembly-contract],"
        "[orbital-activation-contract],[manufacturing-reservation],[orbital-recipe-inventory],"
        "[orbital-recipe-runtime],[orbital-dock-contract],[orbital-removal-contract],[orbital-evacuation-contract]")
PRODUCERS = ("orbital-module-reservation-verification.json", "orbital-handoff-debug-verification.json",
             "manufacturing-reservation-verification.json", "orbital-processor-verification.json",
             "heavy-dock-service-verification.json", "orbital-removal-verification.json",
             "orbital-evacuation-verification.json", "orbital-lifecycle-verification.json")
SOURCES = ("src/space_structure.cpp", "src/orbital_station.cpp", "src/orbital_manufacturing.cpp",
           "src/orbital_recipe.cpp", "src/orbital_recipe_runtime.cpp", "src/orbital_service.cpp",
           "src/tests/space_construction.cpp", "src/tests/orbital_core.cpp", "src/tests/orbital_module_assembly.cpp",
           "src/tests/orbital_module_activation.cpp", "src/tests/manufacturing_reservation.cpp",
           "src/tests/orbital_recipe_inventory.cpp", "src/tests/orbital_dock.cpp", "src/tests/orbital_utility_retirement.cpp",
           "src/tests/orbital_removal.cpp", "src/tests/orbital_evacuation.cpp")


def read(path):
    """Read one explicit repository evidence file.

    @param path Exact immutable index or current receipt. @return Its original parsed JSON.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def execute(configuration):
    """Run only the original refusal contracts against one current native test executable.

    @param configuration Independently built Debug or RelWithDebInfo configuration.
    @return Current bounded component receipt, without any new paid procurement claim.
    """
    context = create_run_context("orbital-art", "refusal-boundaries")
    context.prepare_runtime()
    executable = ROOT / "build" / configuration / "openttd_test.exe"
    binary = read(ROOT / "logs/orbital-art-ordinary-binary-verification.json")
    assert identity(executable) == binary["builds"][configuration]["test_application"]
    log = context.logs / "refusal-boundaries.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(executable), TAGS], cwd=ROOT / "build",
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration),
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    matched = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(errors="replace"))
    assert result.returncode == 0 and matched, log
    output = context.reports / "refusal-boundaries.json"
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration,
                                     assertions=int(matched[1]), cases=int(matched[2]), tags=TAGS,
                                     component_only=True, new_procurement_acceptance=False,
                                     executable=identity(executable), terminal=identity(log),
                                     sources=[identity(ROOT / name) for name in (*SOURCES, Path(__file__).relative_to(ROOT).as_posix())],
                                     native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n")
    return identity(output)


def bind(members):
    """Join current component refusals with the already qualified native producer boundaries.

    @param members Exactly two explicit current configuration membership files.
    @return Final original13.2 evidence with dated scope preserved and seven refusal areas covered.
    """
    configurations = []
    # Both actual current executables must pass the entire focused boundary
    # selection; another configuration's receipt cannot supply its authority.
    for member in members:
        reference = read(member)["report"]
        assert identity(ROOT / reference["path"]) == reference
        row = read(ROOT / reference["path"])
        assert row["complete"] and row["component_only"] and not row["new_procurement_acceptance"]
        for reference in [row["executable"], row["terminal"], *row["sources"], *row["native_processes"]]:
            assert identity(ROOT / reference["path"]) == reference
        configurations.append(dict(report=read(member)["report"], **row))
    assert len(configurations) == 2 and {row["configuration"] for row in configurations} == {"RelWithDebInfo", "Debug"}
    handoff = read(ROOT / "logs/orbital-handoff-debug-verification.json")
    assert set(handoff["configurations"]) == {"RelWithDebInfo", "Debug"}
    assert handoff["debug_native_phases"] == 22
    coverage = {
        "insufficient_deliveries": ["orbital-module-reservation-verification.json", "space-construction-contract", "manufacturing-reservation"],
        "overlapping_blueprints": ["orbital-module-reservation-verification.json", "space-construction-contract"],
        "competing_reservations": ["orbital-module-reservation-verification.json", "manufacturing-reservation-verification.json", "manufacturing-reservation"],
        "missing_utilities": ["orbital-processor-verification.json", "orbital-assembly-contract", "orbital-recipe-runtime", "orbital-dock-contract"],
        "full_outputs": ["orbital-processor-verification.json", "orbital-recipe-inventory", "orbital-evacuation-contract"],
        "invalid_dock_references": ["heavy-dock-service-verification.json", "orbital-dock-contract"],
        "unsafe_removal": ["orbital-handoff-debug-verification.json", "orbital-removal-contract", "orbital-evacuation-contract", "orbital-recipe-runtime"],
    }
    return dict(_documentation=__doc__, complete=True, configurations=configurations, coverage=coverage,
                dated_native_boundaries=[identity(ROOT / "logs" / name) for name in PRODUCERS],
                superseded_notes="Original removal/evacuation/lifecycle Debug-pending notes are superseded by the22-phase two-configuration construction handoff; original indexes remain immutable.",
                scope="Original13.2 refusal semantics only; no new initial-content procurement, malformed-file replay, end-to-end or performance acceptance.",
                ordinary_presentation=identity(ROOT / "logs/orbital-art-ordinary-verification.json"))


def main():
    """Run or bind an explicit focused refusal membership under the repository logs."""
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    mode.add_argument("--bind", nargs=2, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    result = bind([ROOT / path for path in args.bind]) if args.bind else dict(_documentation=__doc__, report=execute(args.configuration))
    output.write_text(json.dumps(result, indent=2) + "\n")
    print(identity(output), flush=True)


if __name__ == "__main__":
    main()
