"""Construct paid orbital presentation families and actual factory work through original commands.

@file orbital_final_art_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13L
Inherited starting-content procurement stays unverified. This observer pays actual
transport, capital, construction and activation; it grants no stock, work or pose.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, identity
from artifact_paths import create_run_context
from runner import run_native

CAPTURE = runpy.run_path(str(Path(__file__).with_name("orbital_final_art_checks-1.py")))
STAGES = ("initial", "power-blueprint", "power-reserved", "power-partial", "power-frame",
          "power-active", "power-blocked", "fuel-blueprint", "fuel-reserved", "fuel-partial", "fuel-frame")
MANUFACTURE_STAGES = ("initial", "assembly-blueprint", "assembly-reserved", "assembly-partial", "assembly-frame", "assembly-active",
                      "manufacture-reserved", "manufacture-partial", "manufacture-complete")


def focused(configuration):
    """Verify the corrected paused status and its original processor work boundaries.

    @param configuration Independently built current native test configuration.
    @return Direct focused process evidence, separate from paid-world procurement.
    """
    context = create_run_context("orbital-art", "focused-display")
    context.prepare_runtime()
    executable = ROOT / "build" / configuration / "openttd_test.exe"
    log = context.logs / "focused-display.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(executable), "[orbital-art-module],[orbital-recipe-runtime]"], cwd=ROOT / "build",
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration),
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    matched = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(errors="replace"))
    assert result.returncode == 0 and matched, log
    output = context.reports / "focused-display.json"
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration,
                                     scope="focused-display", assertions=int(matched[1]), cases=int(matched[2]),
                                     executable=identity(executable), terminal=identity(log),
                                     sources=[identity(ROOT / name) for name in ("src/orbital_recipe_runtime.cpp", "src/tests/orbital_recipe_inventory.cpp")],
                                     native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n")
    return identity(output)


def execute(configuration, manufacture=False):
    """Run one bounded original paid-world observer and bind its physical milestones.

    @param configuration Independently built current test executable configuration.
    @param manufacture Create one actual paid factory job instead of the two module projects.
    @return Semantic evidence with exact unchanged input and actual produced saves.
    """
    owner = "processor"
    mode = "manufacture" if manufacture else "construction"
    dated, source = CAPTURE["predecessor"](configuration, owner, "native/inspected")
    context = create_run_context("orbital-art", "paid-construction")
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# @see r2mlvs11-orbital-stations-and-manufacturing.md13L\norbital_art_test\n")
    env = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_ORBITAL_ART_SOURCE=str(ROOT / source["path"]), OTTD_ORBITAL_ART_MODE=mode)
    executable = ROOT / "build" / configuration / "openttd_test.exe"
    log = context.logs / "paid-construction.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(executable), "[.orbital-art-native]"], cwd=ROOT / "build", env=env,
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    terminal = log.read_text(errors="replace")
    matched = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and matched, log
    assert int(matched[2]) == 1
    expected = "paid_products=1" if manufacture else "paid_modules=2"
    assert "ORBITAL_ART native=true added_stock=false granted_work=false granted_pose=false " + expected in terminal
    checkpoints = {stage: identity(context.saves / ("art-" + stage + "-438.sav")) for stage in (MANUFACTURE_STAGES if manufacture else STAGES)}
    CAPTURE["compare_ordinary_owners"](source, checkpoints["initial"], configuration)
    assert (dated, source) == CAPTURE["predecessor"](configuration, owner, "native/inspected")
    output = context.reports / "paid-construction.json"
    proof = dict(_documentation=__doc__, complete=True, mode=mode, configuration=configuration, dated_producer=dated,
                 source=source, checkpoints=checkpoints, executable=identity(executable), terminal=identity(log),
                 assertions=int(matched[1]), cases=int(matched[2]), inherited_starting_content_procurement=False,
                 added_stock=False, granted_work=False, granted_pose=False,
                 sources=[identity(ROOT / name) for name in
                          ("src/tests/orbital_art_native.cpp", "src/tests/assembly_native_fixture.h",
                           Path(__file__).relative_to(ROOT).as_posix())],
                 native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))])
    output.write_text(json.dumps(proof, indent=2) + "\n")
    return identity(output)


def bind(members):
    """Bind exactly two independently constructed native configuration results.

    @param members Explicit membership files written by this producer, never a newest-file search.
    @return Combined direct immutable handoff for ordinary screenshot consumers.
    """
    rows = []
    # Every configuration must independently pay and produce all milestones;
    # another build's favorable checkpoint cannot stand in for its own world.
    for member in members:
        selected = CAPTURE["read"](member)["report"]
        assert identity(ROOT / selected["path"]) == selected
        row = CAPTURE["read"](ROOT / selected["path"])
        expected = MANUFACTURE_STAGES if row.get("mode") == "manufacture" else STAGES
        assert row["complete"] and set(row["checkpoints"]) == set(expected)
        assert not any(row[field] for field in ("added_stock", "granted_work", "granted_pose"))
        for reference in [row["source"], row["dated_producer"], row["executable"], row["terminal"],
                          *row["checkpoints"].values(), *row["sources"], *row["native_processes"]]:
            assert identity(ROOT / reference["path"]) == reference
        rows.append(dict(report=selected, **row))
    assert len(rows) == 2 and {row["configuration"] for row in rows} == {"RelWithDebInfo", "Debug"}
    assert len({row.get("mode", "construction") for row in rows}) == 1
    return dict(_documentation=__doc__, complete=True, configurations=rows)


def main():
    """Execute one paid observer or bind its two explicit results under the repository log owner."""
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    mode.add_argument("--bind", nargs=2, type=Path)
    scope = parser.add_mutually_exclusive_group()
    scope.add_argument("--focused", action="store_true")
    scope.add_argument("--manufacture", action="store_true")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    assert not (args.bind and (args.focused or args.manufacture))
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    result = bind([ROOT / path for path in args.bind]) if args.bind else dict(
        _documentation=__doc__, report=focused(args.configuration) if args.focused else execute(args.configuration, args.manufacture))
    output.write_text(json.dumps(result, indent=2) + "\n")
    print(identity(output), flush=True)


if __name__ == "__main__":
    main()
