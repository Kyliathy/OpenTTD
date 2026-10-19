"""Verify paid native pickup shortening and actual public portal connection.

@file surface_entry_salt_mine_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BG.1.
Ordinary connection needs plain owned rail outside the door. Actual commands
retain station5on one far tile and restore its already paid near approach.
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
    """Repair only the actual near pickup tile and connect the earned empty pair.

    @param configuration Independently linked focused application.
    @param source Exact original paid-facility checkpoint with the still-sealed pair.
    @return Actual command invoices, native connected portal and conserved original owners.
    """
    context = create_run_context("space", "surface-entry-salt-mine-approach")
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_salt_mine\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "approach.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-salt-mine]"], cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_SALT_MINE_PHASE="repair"))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_SALT_MINE_APPROACH_REPAIRED" in terminal, log
    assert identity(source) == original
    before = context.saves / "surface-salt-mine-before-repair-440.sav"
    output = context.saves / "surface-salt-mine-repair-440.sav"
    first, final = [OWNERS["observe"](path) for path in (before, output)]
    invoices = [dict(command=command, cost=int(cost)) for command, cost in re.findall(r"SURFACE_SALT_MINE_INVOICE command=(\w+) cost=(-?\d+)", terminal)]
    assert first["cash"] - final["cash"] == final["expense"] - first["expense"] == sum(row["cost"] for row in invoices)
    for name, value in first.items():
        if name not in ("cash", "expense"):
            assert final[name] == value, name
    a, b = chunks(before)[1], chunks(output)[1]
    for family in (b"GEOS", b"GDEP", b"GSPN", b"GVNT", b"VEHS", b"CAPA", b"INDY", b"XJOB", b"XJRT", b"XJTL", b"DEPT", b"DATE"):
        assert a[family] == b[family], family
    old, new = [OWNERS["saved_table"](path, b"STNN") for path in (before, output)]
    assert old.keys() == new.keys()
    for key, value in old.items():
        if key != 5:
            assert new[key] == value
    tiletypes = b[b"MAPT"][3:]
    assert tiletypes[6109] >> 4 == 1 and tiletypes[6173] >> 4 == 5
    records = OWNERS["saved_table"](output, b"LPWK")
    portal, = records.values()
    assert portal["state"] == 2
    return dict(source=original, before=identity(before), output=identity(output), application=identity(application), terminal=identity(log),
                initial=first, final=final, invoices=invoices, portal=portal, simulation_ticks=0, supplied_stock_or_floor=False,
                retained_harness=identity(retained), native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_salt_mine.cpp")])


def main():
    """Publish one bounded actual native approach/connection repair receipt."""
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
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), invoice=sum(row["cost"] for row in result["invoices"]))), flush=True)


if __name__ == "__main__":
    main()
