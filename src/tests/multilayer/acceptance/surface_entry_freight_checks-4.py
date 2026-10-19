"""Verify normally paid separate ore intake capacity before driving the actual fleet.

@file surface_entry_freight_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BD.3.
Full-load Steel pickups must not occupy every bay used by their own input truck.
Actual construction and ordinary order edits establish a separate delivery stop.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from space_preview_checks import ROOT, identity
from artifact_paths import create_run_context
from runner import run_native

sys.path.insert(0, str(ROOT / "src/tests"))
from tbm_logistics_save_fixture import saved_table


def execute(configuration, source):
    """Buy actual ore intake and redirect the exact empty paid truck without ticks.

    @param configuration Independently linked focused native executable.
    @param source Original empty paid-fleet checkpoint, never the stalled trial output.
    @return Actual original infrastructure/order process and native physical receipts.
    """
    context = create_run_context("space", "surface-entry-ore-capacity")
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_roads\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "capacity.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-roads]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source),
                                     OTTD_SURFACE_ENTRY_ORE_CAPACITY="true"))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_ENTRY_ORE_CAPACITY" in terminal, log
    assert identity(source) == original
    before = context.saves / "surface-entry-before-roads-440.sav"
    output = context.saves / "surface-entry-roads-440.sav"
    for owner in (b"INDY", b"DEPT"):
        assert saved_table(before, owner) == saved_table(output, owner), owner
    assert not saved_table(output, b"CAPA")
    assert saved_table(before, b"VEHS") == saved_table(output, b"VEHS")
    assert len(saved_table(output, b"STNN")) == 4
    first, = saved_table(before, b"PLYR").values()
    final, = saved_table(output, b"PLYR").values()
    invoices = [dict(command=command, tile=int(tile), cost=int(cost)) for command, tile, cost in re.findall(
        r"SURFACE_ENTRY_ROAD_INVOICE command=(\w+) tile=(\d+) cost=(-?\d+)", terminal)]
    expense = sum(row["cost"] for row in invoices)
    assert expense > 0 and first["money"] - final["money"] == expense
    assert sum(final["yearly_expenses"]) - sum(first["yearly_expenses"]) == expense
    assert first["current_loan"] == final["current_loan"] == 2000000
    original_date, = saved_table(before, b"DATE").values()
    actual_date, = saved_table(output, b"DATE").values()
    # Native appearance RNG belongs to real construction; no clock, identity
    # or production work may be supplied by station building or order changes.
    for name, value in original_date.items():
        if not name.startswith("random_state["):
            assert actual_date[name] == value, name
    return dict(source=original, before=identity(before), output=identity(output), application=identity(application), terminal=identity(log),
                invoices=invoices, expense=expense, cash=final["money"], debt=2000000,
                supplied_stock=False, simulation_ticks=0, retained_harness=identity(retained),
                native_processes=[identity(p) for p in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_roads.cpp")])


def main():
    """Select one exact original fleet and publish a fresh bounded capacity receipt."""
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
