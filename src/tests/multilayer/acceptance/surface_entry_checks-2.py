"""Characterize native Surface factories, fuel and launch placement without mutations.

@file surface_entry_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BB.2.
The source is the exact ordinary new-game company. Native pure command queries
and public recipes expose the next actual paid producer; no stock/work/cash is supplied.
"""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from space_preview_checks import ROOT, identity, chunks
from artifact_paths import create_run_context
from runner import run_native


def execute(configuration, source):
    """Query actual original native definitions and whole placement prices.

    @param configuration Independently linked focused native executable.
    @param source Exact original ordinary company/world, never a fixture predecessor.
    @return Original source, pure native rows, unchanged snapshots and child report.
    """
    context = create_run_context("space", "surface-entry-queries")
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_inspect\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "queries.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-native]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                                     OTTD_SURFACE_ENTRY_SOURCE=str(source)))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_ENTRY_QUERIES_COMPLETE" in terminal, log
    assert identity(source) == original
    before = context.saves / "surface-entry-before-queries-440.sav"
    after = context.saves / "surface-entry-after-queries-440.sav"
    first, second = chunks(before)[1], chunks(after)[1]
    assert first == second, [key for key in first if first[key] != second.get(key)]
    factories = [dict(family=family, type=int(kind), anchor=int(anchor), cost=int(cost)) for family, kind, anchor, cost
                 in re.findall(r"SURFACE_ENTRY_FACTORY family=(\w+) type=(\d+) anchor=(\d+) cost=(\d+)", terminal)]
    assert [row["family"] for row in factories] == ["STEL", "GOOD", "CHEM"]
    conversions = [dict(family=family, input=source_label, output=target_label, multiplier=int(multiplier))
                   for family, source_label, target_label, multiplier in re.findall(
                       r"SURFACE_ENTRY_CONVERSION family=(\w+) input=(\w+) output=(\w+) multiplier=(\d+)", terminal)]
    fuel = [dict(recipe=recipe, cargo=cargo, quantity=int(quantity)) for recipe, cargo, quantity in re.findall(
        r"SURFACE_ENTRY_FUEL_INPUT recipe=(\w+) cargo=(\w+) quantity=(\d+)", terminal)]
    found, anchor, rotation, cost, maximum = re.findall(
        r"SURFACE_ENTRY_LAUNCH found=(true|false) anchor=(\d+) rotation=(\d+) cost=(-?\d+) maximum_loan=(\d+)", terminal)[0]
    launch = dict(found=found == "true", anchor=int(anchor), rotation=int(rotation), cost=int(cost), maximum_loan=int(maximum))
    refusals = [dict(reason=int(reason), count=int(count)) for reason, count in re.findall(
        r"SURFACE_ENTRY_LAUNCH_REFUSAL reason=(\d+) count=(\d+)", terminal)]
    proof = dict(_documentation=__doc__, complete=True, configuration=configuration, source=original,
                 application=identity(application), terminal=identity(log), factories=factories, conversions=conversions,
                 fuel=fuel, launch=launch, launch_refusals=refusals,
                 unchanged_native_families=len(first), snapshots=[identity(before), identity(after)],
                 native_processes=[identity(p) for p in sorted(context.reports.glob("native-*.json"))],
                 sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_native.cpp")],
                 supplied_stock=False, completed_factory_or_launch=False)
    target = context.reports / "queries.json"
    target.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)
    return identity(target)


def main():
    """Select one bounded actual query process and publish its fresh exact membership."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source, output = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    reference = execute(args.configuration, source)
    output.write_text(json.dumps(dict(_documentation=__doc__, report=reference), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
