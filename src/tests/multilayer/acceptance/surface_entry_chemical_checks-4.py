"""Verify actual retired-cutter recovery, matching credit and paid chemical construction.

@file surface_entry_chemical_checks-4.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.2.
Native commands alone own sale proceeds, debt and incomplete industry/road/bay
creation. No construction progress, material stock or transport is supplied.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group8.0; pure reader ownership.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity
from space_saved_state import observe_factory
from artifact_paths import create_run_context
from runner import run_native

SALT = runpy.run_path(str(Path(__file__).with_name("surface_entry_salt_mine_checks-6.py")))
OWNERS = SALT["OWNERS"]




def execute(configuration, receipt, phase):
    """Execute one original capital or construction boundary without simulation work.

    @param configuration Independently linked focused native application.
    @param receipt Exact genuine Salt source or preceding actual recovered-capital receipt.
    @param phase Actual retired-asset/bank settlement or actual paid infrastructure.
    @return Native process, invoices, protected material owners and incomplete factory state.
    """
    selected = json.loads(receipt.read_text(encoding="utf-8"))
    assert selected["complete"] and selected["configuration"] == configuration
    expected = "drive" if phase == "capital" else "capital"
    assert selected["phase"] == expected
    source = ROOT / selected["operation"]["output"]["path"]
    assert identity(source) == selected["operation"]["output"]
    context = create_run_context("space", "surface-entry-chemical-" + phase)
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_chemical\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / (phase + ".log")
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-chemical]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_CHEMICAL_PHASE=phase))
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_CHEMICAL_COMPLETE phase=" + phase in terminal, log
    assert identity(source) == original
    before, output = [context.saves / ("surface-chemical-" + name + "-440.sav") for name in ("before-" + phase, phase)]
    first, final = [OWNERS["observe"](path) for path in (before, output)]
    invoices = [dict(command=command, tile=int(tile), cost=int(cost)) for command, tile, cost in
                re.findall(r"SURFACE_CHEMICAL_INVOICE command=(\w+) tile=(\d+) cost=(-?\d+)", terminal)]
    paid = sum(row["cost"] for row in invoices)
    credit = 400000 if phase == "capital" else 0
    assert final["cash"] == first["cash"] + credit - paid
    assert final["debt"] == first["debt"] + credit == 4650000
    assert final["expense"] - first["expense"] == paid
    assert first["parked_steel"] == final["parked_steel"] == 36 and first["parked_goods"] == final["parked_goods"] == 14
    a, b = [chunks(path)[1] for path in (before, output)]
    for family in (b"CAPA", b"GEOS", b"GDEP", b"LMNR", b"GSPN", b"GVNT", b"XJOB", b"XJRT", b"XJTL"):
        assert a[family] == b[family], family
    dates = [OWNERS["saved_table"](path, b"DATE")[0] for path in (before, output)]
    # Actual industry construction consumes the native synchronized RNG.
    # All clock/pause/calendar fields remain exact; capital recovery consumes
    # no construction randomness and must retain the complete DATE family.
    random_fields = {"random_state[0]", "random_state[1]"} if phase == "fund" else set()
    assert all(dates[1][key] == value for key, value in dates[0].items() if key not in random_fields)
    salt = SALT["observe"](output)
    assert salt["surface_custody"] and salt["wagon"]["held"] == 60
    factory = None
    if phase == "capital":
        assert paid == -350868 and final["cash"] == 839142
        old, new = [OWNERS["saved_table"](path, b"VEHS") for path in (before, output)]
        assert set(new) == set(old) - set(range(7, 13))
        for key, row in new.items():
            assert row == old[key], key
        for family in (b"INDY", b"STNN", b"DEPT", b"LPCR", b"LPCC"):
            assert a[family] == b[family], family
    else:
        assert paid == 724638 and final["cash"] == 114504
        for family in (b"VEHS", b"DEPT"):
            assert a[family] == b[family], family
        old, new = [OWNERS["saved_table"](path, b"INDY") for path in (before, output)]
        assert set(new) == set(old) | {15}
        for key, row in old.items():
            assert new[key] == row, key
        factory = observe_factory(output)
        assert not factory["complete"] and not any(factory["inputs"].values()) and not any(factory["outputs"].values())
        assert factory["processor"]["work_ticks"] == factory["processor"]["dispatched"] == 0
        assert all(row["stage"] == row["counter"] == 0 for row in factory["cells"])
        assert all(value == 0 for family in factory["counters"].values() for value in family.values())
        bays = [dict(role=int(role), tile=int(tile), station=int(station), current_catchment=catchment == "true") for role, tile, station, catchment in
                re.findall(r"SURFACE_CHEMICAL_BAY_PAID role=(\d+) tile=(\d+) station=(\d+) current_catchment=(true|false)", terminal)]
        assert len(bays) == 5 and all(row["current_catchment"] for row in bays[:3])
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application),
                terminal=identity(log), initial=first, final=final, invoices=invoices, credit=credit, factory=factory, salt=salt,
                simulation_ticks=0, supplied_stock_or_progress=False, retained_harness=identity(retained),
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_chemical.cpp"), identity(ROOT / "src/tests/surface_chemical_layout.h")])


def main():
    """Publish one immutable real financial or incomplete construction receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("capital", "fund"), required=True)
    parser.add_argument("--source-receipt", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    receipt, target = [(ROOT / path).resolve() for path in (args.source_receipt, args.output)]
    assert receipt.is_relative_to(ROOT / "logs") and receipt.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, receipt, args.phase)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase,
                                     operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), cash=result["final"]["cash"], debt=result["final"]["debt"])), flush=True)


if __name__ == "__main__":
    main()
