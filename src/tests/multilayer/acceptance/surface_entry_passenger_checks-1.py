"""Observe genuine passenger service costs in the physically produced-fuel world.

@file surface_entry_passenger_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEB.1.
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
    """@param configuration Original focused build. @return Pure current passenger service characterization with immutable physical ancestry."""
    foundation_path = ROOT / "logs/surface-entry-propellant-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"]
    entry, = [row for row in foundation["configurations"] if row["configuration"] == configuration]
    source = ROOT / entry["output"]["path"]
    assert identity(source) == entry["output"]
    context = create_run_context("space", "surface-entry-passenger-query")
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_launch\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    log = context.logs / "query.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-launch]"], cwd=ROOT / "build", stdout=sink,
                            stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_LAUNCH_PHASE="passenger-query"))
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_LAUNCH_COMPLETE phase=passenger-query" in terminal, log
    assert identity(source) == entry["output"]
    before, output = [context.saves / f"surface-launch-{name}-440.sav" for name in ("before-passenger-query", "passenger-query")]
    original, first, final = [chunks(path)[1] for path in (source, before, output)]
    assert first == final and len(first) == 121
    assert original.keys() == first.keys() and all(original[key] == first[key] for key in original if key != b"GLOG")
    if original[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    initial, observed = observe(before), observe(output)
    assert initial == observed and observed["propellant_truck"]["held"] == 20
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_PASSENGER_" + prefix + " ")]
            for name, prefix in dict(layout="LAYOUT", tariff="TARIFF", capital="CAPITAL").items()}
    capital, = rows["capital"]; layout, = rows["layout"]
    assert capital["cash"] == observed["company"]["cash"] == 46079
    assert capital["buses"] == 3 and capital["model"] == 119 and capital["capacity"] == 40
    assert capital["total"] == layout["infrastructure"] + capital["buses"] * capital["bus_cost"]
    assert capital["reserve"] == 20000 and capital["affordable"]
    assert min(layout["first_production"], layout["last_production"]) >= 4 and layout["distance"] >= 16
    return dict(foundation=identity(foundation_path), source=entry["output"], before=identity(before), output=identity(output),
                application=identity(application), terminal=identity(log), quotes=rows, initial=initial, final=observed,
                unchanged_native_families=121, simulation_ticks=0, complete_arrangement=False, retained_harness=identity(retained),
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_launch.cpp"), identity(ROOT / "src/tests/surface_passenger_layout.h"),
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
                                     phase="passenger-query", operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
