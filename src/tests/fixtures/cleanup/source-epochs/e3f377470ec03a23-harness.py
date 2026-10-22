"""Verify actual paid mine pickup, released support and clear earned foundations.

@file surface_entry_mine_checks-6.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BF.3.
Actual player commands retire settled work, convert paid rail, clear machinery
cells and build native pickup. No mine, reserve, floor or product is supplied.
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

GALLERY = runpy.run_path(str(Path(__file__).with_name("surface_entry_mine_checks-4.py")))
OWNERS = GALLERY["OWNERS"]


def execute(configuration, source):
    """Fund actual rail pickup and clear only paid machinery tracks on earned floor.

    @param configuration Independently linked focused application.
    @param source Exact completed gallery/haulage checkpoint, never a prepared cavity.
    @return Original native process, invoices, unchanged finite owners and actual station admission.
    """
    assert GALLERY["observe"](source)["ready"]
    context = create_run_context("space", "surface-entry-mine-facilities")
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_mine\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "facilities.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-mine]"], cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_MINE_PHASE="facilities"))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_MINE_FACILITIES_COMPLETE" in terminal, log
    assert identity(source) == original
    before = context.saves / "surface-mine-before-facilities-440.sav"
    output = context.saves / "surface-mine-facilities-440.sav"
    first, final = [OWNERS["observe"](path) for path in (before, output)]
    invoices = [dict(command=command, cost=int(cost)) for command, cost in re.findall(r"SURFACE_MINE_INVOICE command=(\w+) cost=(-?\d+)", terminal)]
    assert first["cash"] - final["cash"] == final["expense"] - first["expense"] == sum(row["cost"] for row in invoices)
    for name, value in first.items():
        if name not in ("cash", "expense"):
            assert final[name] == value, name
    a, b = chunks(before)[1], chunks(output)[1]
    for family in (b"GEOS", b"GDEP", b"GSPN", b"GVNT", b"CAPA", b"INDY", b"XJOB", b"XJRT", b"XJTL", b"DATE", b"DEPT"):
        assert a[family] == b[family], family
    native = GALLERY["observe"](output)
    assert not native["owners"]["projects"] and not native["owners"]["assignments"] and not native["owners"]["sets"]
    assert len(native["owners"]["trains"]) == 11 and native["salt_remaining"] == 24000
    assert len(native["floor"]) == 27 and all(not row["packets"] for row in native["owners"]["trains"].values())
    old, new = [OWNERS["saved_table"](path, b"STNN") for path in (before, output)]
    assert len(new) == len(old) + 1
    for key, value in old.items():
        assert new[key] == value, key
    mine, station, tile, capital, reserve = map(int, re.findall(
        r"SURFACE_MINE_FACILITIES_COMPLETE mine=(\d+) station=(\d+) station_tile=(\d+) capital=(\d+) reserve=(\d+)", terminal)[0])
    assert mine == 6236 and tile == 6109 and station == 5 and reserve == 24000 and capital > 0
    tiletypes = b[b"MAPT"][3:]
    # Native packed map independently proves all nine actual foundation cells
    # are clear and both real pickup cells are Station tiles. Paid construction
    # cannot declare success while leaving a rail buried under machinery.
    foundations = [4096 + y * 64 + x for y in range(33, 36) for x in range(28, 31)]
    assert all(tiletypes[cell] >> 4 == 0 for cell in foundations)
    assert all(tiletypes[cell] >> 4 == 5 for cell in (6109, 6173))
    return dict(source=original, before=identity(before), output=identity(output), application=identity(application), terminal=identity(log),
                initial=first, final=final, invoices=invoices, work=native, mine=mine, station=station, capital=capital, simulation_ticks=0,
                supplied_stock_or_floor=False, retained_harness=identity(retained),
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_mine.cpp")])


def main():
    """Publish one genuine paid pickup/foundation receipt."""
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
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), station=result["station"], mine_capital=result["capital"],
                          invoice=sum(row["cost"] for row in result["invoices"]))), flush=True)


if __name__ == "__main__":
    main()
