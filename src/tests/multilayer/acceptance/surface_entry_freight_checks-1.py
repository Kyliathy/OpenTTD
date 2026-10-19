"""Verify one authentic game's actual normally paid conventional freight infrastructure.

@file surface_entry_freight_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BD.1.
The original completed factories remain untouched. Every new road/station/depot
is command-built, charged once and saved by its actual production owner.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import run_native

sys.path.insert(0, str(ROOT / "src/tests"))
from tbm_logistics_save_fixture import saved_table


def execute(configuration, source):
    """Fund actual useful stations, connected roads and a native depot in one bounded process.

    @param configuration Independently linked focused application.
    @param source Exact authentic completed-factory checkpoint.
    @return Actual original source/output, invoices and observed native process.
    """
    context = create_run_context("space", "surface-entry-roads")
    context.prepare_runtime()
    harness = Path(__file__).resolve()
    retained_harness = context.directory / "harness.py"
    retained_harness.write_bytes(harness.read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_roads\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "roads.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-roads]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source)))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_ENTRY_ROADS_COMPLETE" in terminal, log
    assert identity(source) == original
    before = context.saves / "surface-entry-before-roads-440.sav"
    output = context.saves / "surface-entry-roads-440.sav"
    assert saved_table(before, b"INDY") == saved_table(output, b"INDY")
    assert not saved_table(output, b"CAPA")
    company_before, = saved_table(before, b"PLYR").values()
    company_after, = saved_table(output, b"PLYR").values()
    invoices = [dict(command=command, tile=int(tile), cost=int(cost)) for command, tile, cost in re.findall(
        r"SURFACE_ENTRY_ROAD_INVOICE command=(\w+) tile=(\d+) cost=(-?\d+)", terminal)]
    expense = sum(row["cost"] for row in invoices)
    assert expense > 0 and company_before["money"] - company_after["money"] == expense
    assert sum(company_after["yearly_expenses"]) - sum(company_before["yearly_expenses"]) == expense
    assert company_before["current_loan"] == company_after["current_loan"] == 2000000
    assert company_after["cur_economy[0].income"] == 0
    original_date, = saved_table(before, b"DATE").values()
    actual_date, = saved_table(output, b"DATE").values()
    # Native station creation may draw appearance RNG; infrastructure commands
    # cannot grant a construction tick, calendar time or a new game identity.
    for name, value in original_date.items():
        if not name.startswith("random_state["):
            assert actual_date[name] == value, name
    stations = saved_table(output, b"STNN")
    assert len(stations) == 3 and len(saved_table(output, b"DEPT")) == 1
    assert not any(row["type"] in (0, 1, 2, 3, 6) for row in saved_table(output, b"VEHS").values())
    bays = [dict(industry=int(industry), station=int(station), tile=int(tile), entrance=int(entrance), direction=int(direction), radius=int(radius))
            for industry, station, tile, entrance, direction, radius in re.findall(
                r"SURFACE_ENTRY_FREIGHT_BAY industry=(\d+) station=(\d+) tile=(\d+) entrance=(\d+) direction=(\d+) radius=(\d+)", terminal)]
    assert len(bays) == 3 and [row["industry"] for row in bays[1:]] == [10, 11]
    return dict(source=original, output=identity(output), before=identity(before), application=identity(application),
                terminal=identity(log), invoices=invoices, expense=expense, cash=company_after["money"], debt=2000000,
                bays=bays, supplied_stock=False, simulation_ticks=0, retained_harness=identity(retained_harness),
                native_processes=[identity(p) for p in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(harness), identity(ROOT / "src/tests/surface_entry_roads.cpp")])


def main():
    """Run one bounded configuration and retain its exact original road/factory membership."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     operation=execute(args.configuration, source)), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
