"""Recall and recover a real empty hauler, then buy and dispatch chemical freight.

@file surface_entry_chemical_checks-8.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.2.
Only normal orders can clear the shared rail platform; actual depot arrival
precedes a native sale. Empty tanker purchases never supply fuel ingredients.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import run_native
from surface_chemical_ledger import FACTORY, TABLE, products, salt_ledger


def execute(configuration, receipt, phase):
    """Perform one original order, earned arrival/sale, or empty fleet boundary.

    @param configuration Independently linked focused native application.
    @param receipt Exact preceding genuine construction/recall/recovery receipt.
    @param phase Recall, earned actual depot settlement, or native empty tanker purchase.
    @return Native process, actual sale/purchase invoices and original cargo custody.
    """
    selected = json.loads(receipt.read_text(encoding="utf-8"))
    assert selected["complete"] and selected["configuration"] == configuration
    assert selected["phase"] == dict(recall="advance", settle="recall", fleet="settle")[phase]
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
    first, final = [products(path) for path in (before, output)]
    invoices = [dict(command=command, tile=int(tile), cost=int(cost)) for command, tile, cost in
                re.findall(r"SURFACE_CHEMICAL_INVOICE command=(\w+) tile=(\d+) cost=(-?\d+)", terminal)]
    assert first["debt"] == final["debt"] == 4650000
    assert first["cash"] - final["cash"] == final["expense"] - first["expense"]
    initial_salt, salt = [salt_ledger(path) for path in (before, output)]
    assert salt["wagon_held"] == initial_salt["wagon_held"] == 60 and salt["delivered"] == initial_salt["delivered"] == 0
    factory = FACTORY["observe_factory"](output)
    assert factory["complete"] and not any(factory["inputs"].values()) and not any(factory["outputs"].values())
    a, b = [chunks(path)[1] for path in (before, output)]
    dates = [TABLE(path, b"DATE")[0] for path in (before, output)]
    earned = dates[1]["tick_counter"] - dates[0]["tick_counter"]
    for family in (b"GEOS", b"GSPN", b"GVNT", b"XJOB", b"XJRT", b"XJTL"):
        assert a[family] == b[family], family
    old, new = [TABLE(path, b"VEHS") for path in (before, output)]
    if phase == "settle":
        assert 0 < earned < 3000
        assert set(new) == set(old) - set(range(13, 18))
        sale, = invoices
        assert sale["command"] == "CmdSellVehicle" and sale["cost"] < 0
        assert re.search(r"SURFACE_CHEMICAL_HAULER_SETTLED earned=" + str(earned) + r" sale=\d+", terminal)
    else:
        assert earned == 0 and a[b"DATE"] == b[b"DATE"] and a[b"CAPA"] == b[b"CAPA"]
        assert a[b"INDY"] == b[b"INDY"] and a[b"LMNR"] == b[b"LMNR"] and a[b"GDEP"] == b[b"GDEP"]
        assert first["cash"] - final["cash"] == sum(row["cost"] for row in invoices)
        if phase == "fleet":
            assert len(final["tankers"]) == 3 and len(new) == len(old) + 3
            assert all(row["held"] == 0 for row in final["tankers"])
            assert sum(row["cost"] for row in invoices) == 13615
        else:
            assert old.keys() == new.keys() and not invoices[0]["cost"]
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application),
                terminal=identity(log), initial=first, final=final, invoices=invoices, salt=salt, factory=factory, earned_ticks=earned,
                supplied_stock_or_pose=False, retained_harness=identity(retained),
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_chemical.cpp"),
                         identity(Path(__file__).with_name("surface_chemical_ledger.py"))])


def main():
    """Retain one bounded native recall, real depot sale or empty purchase receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("recall", "settle", "fleet"), required=True)
    parser.add_argument("--source-receipt", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    receipt, target = [(ROOT / path).resolve() for path in (args.source_receipt, args.output)]
    assert receipt.is_relative_to(ROOT / "logs") and receipt.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, receipt, args.phase)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase,
                                     operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), cash=result["final"]["cash"], earned=result["earned_ticks"])), flush=True)


if __name__ == "__main__":
    main()
