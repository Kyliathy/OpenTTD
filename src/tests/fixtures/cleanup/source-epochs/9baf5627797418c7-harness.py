"""Verify actual whole mine/carrier quotes and minimum matching native bank credit.

@file surface_entry_salt_mine_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BG.1.
Only money and its matching native debt may change. Original products, floor,
reserves, mine absence, vehicles, packets and clocks remain exact.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import run_native

OWNERS = runpy.run_path(str(Path(__file__).with_name("surface_entry_freight_checks-3.py")))


def execute(configuration, source):
    """Borrow only the native rounded shortfall for actual capital and a stated buffer.

    @param configuration Independently linked focused native application.
    @param source Exact completed paid-facility predecessor, never an already borrowed output.
    @return Actual native process, pure quotes and cash/debt-only credit receipt.
    """
    context = create_run_context("space", "surface-entry-salt-mine-credit")
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_salt_mine\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "credit.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-salt-mine]"], cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source)))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_SALT_MINE_COMPLETE phase=credit" in terminal, log
    assert identity(source) == original
    before = context.saves / "surface-salt-mine-before-credit-440.sav"
    query = context.saves / "surface-salt-mine-query-440.sav"
    output = context.saves / "surface-salt-mine-credit-440.sav"
    assert chunks(before) == chunks(query)
    a, b = chunks(before)[1], chunks(output)[1]
    assert a.keys() == b.keys() and {key for key in a if a[key] != b[key]} == {b"PLYR"}
    initial, final = [OWNERS["saved_table"](path, b"PLYR")[0] for path in (before, output)]
    amount, buffer, cash, debt = map(int, re.findall(r"SURFACE_SALT_MINE_CREDIT amount=(\d+) buffer=(\d+) cash=(\d+) debt=(\d+)", terminal)[0])
    assert amount == 750000 and buffer == 100000 and cash == 889332 and debt == 4250000
    # Native bank credit has one matching liability. No remembered location,
    # old loan, company setting, expense or unrelated saved field may change.
    for name, value in initial.items():
        assert final[name] == (value + amount if name in ("money", "current_loan") else value), name
    quotes = [dict(zip(("model", "engine", "cost", "capacity"), map(int, values), strict=True)) for values in re.findall(
        r"SURFACE_SALT_MINE_CARRIER_QUOTE model=(\d+) engine=(\d+) cost=(\d+) capacity=(\d+)", terminal)]
    mine, total, quantity, work, capacity, ceiling = map(int, re.findall(
        r"SURFACE_SALT_MINE_CAPITAL mine=(\d+) total=(\d+) salt_quantity=(\d+) work=(\d+) output_capacity=(\d+) ceiling=(\d+)", terminal)[0])
    assert mine == 616584 and total == mine + sum(row["cost"] for row in quotes)
    assert quantity == 4 and work == 74 and capacity == 1024 and ceiling == 5000000
    assert cash >= total + buffer and cash - 10000 < total + buffer
    observations = [OWNERS["observe"](path) for path in (before, output)]
    for name, value in observations[0].items():
        if name not in ("cash", "debt"):
            assert observations[1][name] == value, name
    assert observations[1]["parked_steel"] == 36 and observations[1]["parked_goods"] == 14
    assert not OWNERS["saved_table"](output, b"LMNR")
    return dict(source=original, before=identity(before), query=identity(query), output=identity(output), application=identity(application), terminal=identity(log),
                initial=observations[0], final=observations[1], quotes=quotes, mine_quote=mine, capital=total, credit=amount,
                buffer=buffer, ceiling=ceiling, simulation_ticks=0, supplied_stock_or_floor=False, retained_harness=identity(retained),
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_salt_mine.cpp")])


def main():
    """Publish one bounded genuine mine capital/bank receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, source)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), capital=result["capital"], credit=result["credit"])), flush=True)


if __name__ == "__main__":
    main()
