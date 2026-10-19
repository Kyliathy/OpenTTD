"""Observe genuine launch-entry costs in the physically produced-fuel world.

@file surface_entry_launch_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIE.1.
An unchanged native query characterizes actual sites, recipes and capital.
It cannot complete the full funded arrangement or substitute predicted income.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from artifact_paths import create_run_context
from runner import run_native
from space_preview_checks import ROOT, chunks, identity
from surface_propellant_ledger import observe

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))["known_revision"]


def execute(configuration):
    """@param configuration Original focused build. @return Pure current launch characterization with immutable physical ancestry."""
    foundation_path = ROOT / "logs/surface-entry-passenger-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"]
    entry, = [row for row in foundation["configurations"] if row["configuration"] == configuration]
    source = ROOT / entry["output"]["path"]
    assert identity(source) == entry["output"]
    context = create_run_context("space", "surface-entry-launch-funded-query")
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_launch\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    log = context.logs / "query.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-launch]"], cwd=ROOT / "build", stdout=sink,
                            stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source)))
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_LAUNCH_COMPLETE phase=query" in terminal, log
    assert identity(source) == entry["output"]
    before, output = [context.saves / f"surface-launch-{name}-440.sav" for name in ("before-query", "query")]
    original, first, final = [chunks(path)[1] for path in (source, before, output)]
    assert first == final and len(first) == 121
    assert original.keys() == first.keys() and all(original[key] == first[key] for key in original if key != b"GLOG")
    if original[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    initial, observed = observe(before), observe(output)
    assert initial == observed and observed["propellant_truck"]["held"] == 20
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_LAUNCH_" + prefix + " ")]
            for name, prefix in dict(recipes="RECIPE", inputs="INPUT", capacity="CAPACITY", sites="SITE",
                                     capital="CAPITAL", towns="TOWN", passenger_models="PASSENGER").items()}
    capital, = rows["capital"]
    assert capital["cash"] == observed["company"]["cash"] and len(observed["passenger_buses"]) == 3
    assert capital["debt"] == capital["ceiling"] == 5000000
    assert capital["craft_core"] == 1200000 and capital["steel"] == 32 and capital["goods"] == 48
    assert not capital["craft_core_affordable"] and not capital["complete_arrangement"]
    return dict(foundation=identity(foundation_path), source=entry["output"], before=identity(before), output=identity(output),
                application=identity(application), terminal=identity(log), quotes=rows, initial=initial, final=observed,
                unchanged_native_families=121, simulation_ticks=0, complete_arrangement=False, retained_harness=identity(retained),
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_launch.cpp"),
                         identity(ROOT / "src/tests/multilayer/acceptance/surface_propellant_ledger.py")])


def main():
    """Retain a current pure quote without claiming a funded or built launch entry."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     phase="query", operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
