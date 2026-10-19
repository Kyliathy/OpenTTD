"""Verify actual empty native purchases and legal freight orders in the authentic game.

@file surface_entry_freight_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BD.2.
No cargo or simulation progress is supplied. Useful products must later earn
native production, physical loading, real trips and original delivery receipts.
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
    """Buy original available trucks and legal native loading/delivery/depot-halt orders.

    @param configuration Independently linked focused application.
    @param source Exact authentic paid-road output, with no preexisting paid hull.
    @return Actual source/output, original invoices and native child membership.
    """
    context = create_run_context("space", "surface-entry-fleet")
    context.prepare_runtime()
    harness = Path(__file__).resolve()
    retained = context.directory / "harness.py"
    retained.write_bytes(harness.read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_fleet\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "fleet.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-fleet]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source)))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_ENTRY_FLEET_COMPLETE" in terminal, log
    assert identity(source) == original
    before = context.saves / "surface-entry-before-fleet-440.sav"
    output = context.saves / "surface-entry-fleet-440.sav"
    for owner in (b"INDY", b"STNN", b"DEPT"):
        assert saved_table(before, owner) == saved_table(output, owner), owner
    assert not saved_table(output, b"CAPA")
    trucks = [dict(id=int(vehicle), engine=int(engine), cargo=int(cargo), capacity=int(capacity), invoice=int(invoice),
                   pickup=int(pickup), receiver=int(receiver), name=name)
              for vehicle, engine, cargo, capacity, invoice, pickup, receiver, name in re.findall(
                  r"SURFACE_ENTRY_TRUCK_PAID id=(\d+) engine=(\d+) cargo=(\d+) capacity=(\d+) invoice=(\d+) pickup=(\d+) receiver=(\d+) name=([^\r\n]+)", terminal)]
    assert len(trucks) == 5
    vehicles = saved_table(output, b"VEHS")
    assert {key for key, value in vehicles.items() if value["type"] == 1} == {row["id"] for row in trucks}
    company_before, = saved_table(before, b"PLYR").values()
    company_after, = saved_table(output, b"PLYR").values()
    expense = sum(row["invoice"] for row in trucks)
    assert company_before["money"] - company_after["money"] == expense
    assert sum(company_after["yearly_expenses"]) - sum(company_before["yearly_expenses"]) == expense
    assert company_before["current_loan"] == company_after["current_loan"] == 2000000
    date_before, = saved_table(before, b"DATE").values()
    date_after, = saved_table(output, b"DATE").values()
    # Ordinary vehicle appearance/reliability may use original RNG. Purchasing
    # and starting empty trucks still cannot earn any simulation/production work.
    for name, value in date_before.items():
        if not name.startswith("random_state["):
            assert date_after[name] == value, name
    return dict(source=original, output=identity(output), before=identity(before), application=identity(application), terminal=identity(log),
                trucks=trucks, expense=expense, cash=company_after["money"], debt=2000000, supplied_stock=False, simulation_ticks=0,
                retained_harness=identity(retained), native_processes=[identity(p) for p in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(harness), identity(ROOT / "src/tests/surface_entry_fleet.cpp")])


def main():
    """Run one bounded native purchase phase and retain its explicit actual membership."""
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
