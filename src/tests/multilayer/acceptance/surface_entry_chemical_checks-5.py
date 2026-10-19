"""Earn bounded real construction/production ticks for the genuine chemical chain.

@file surface_entry_chemical_checks-5.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.2.
The ordinary native game loop alone advances machinery, finite mining and
freight. Every process earns exactly3000ticks and retains original packet stock.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import run_native
from surface_chemical_ledger import salt_ledger

CONSTRUCTION = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-4.py")))
OWNERS, SALT = CONSTRUCTION["OWNERS"], CONSTRUCTION["SALT"]


def execute(configuration, receipt, require_ready):
    """Earn one exact native tick interval from the actual paid chemical predecessor.

    @param configuration Independently linked focused native application.
    @param receipt Exact preceding paid/working native checkpoint receipt.
    @param require_ready Require all sixteen original machinery cells to be complete.
    @return Native work, sole factory inventory and protected original physical stock.
    """
    selected = json.loads(receipt.read_text(encoding="utf-8"))
    assert selected["complete"] and selected["configuration"] == configuration
    assert selected["phase"] in ("fund", "advance")
    source = ROOT / selected["operation"]["output"]["path"]
    assert identity(source) == selected["operation"]["output"]
    context = create_run_context("space", "surface-entry-chemical-construction")
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_production\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "advance.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-production]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source)))
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal, log
    assert identity(source) == original
    output = context.saves / "surface-entry-replayed-440.sav"
    first, final = [OWNERS["observe"](path) for path in (source, output)]
    assert first["debt"] == final["debt"] == 4650000
    assert first["cash"] - final["cash"] == final["expense"] - first["expense"]
    assert first["parked_steel"] == final["parked_steel"] == 36 and first["parked_goods"] == final["parked_goods"] == 14
    dates = [OWNERS["saved_table"](path, b"DATE")[0] for path in (source, output)]
    assert dates[1]["tick_counter"] - dates[0]["tick_counter"] == 3000 and dates[1]["id"] == dates[0]["id"]
    a, b = [chunks(path)[1] for path in (source, output)]
    for family in (b"GEOS", b"GSPN", b"GVNT", b"XJOB", b"XJRT", b"XJTL"):
        assert a[family] == b[family], family
    factory = CONSTRUCTION["observe_factory"](output)
    original_salt, salt = [salt_ledger(path) for path in (source, output)]
    assert salt["surface_custody"] and salt["wagon_held"] == original_salt["wagon_held"] == 60
    assert salt["delivered"] == original_salt["delivered"] == 0
    assert salt["ordinary_station_loss"] >= original_salt["ordinary_station_loss"]
    assert not any(factory["inputs"].values()) and not any(factory["outputs"].values())
    assert all(value == 0 for family in factory["counters"].values() for value in family.values())
    if require_ready:
        assert factory["complete"], factory
    return dict(receipt=identity(receipt), source=original, output=identity(output), application=identity(application), terminal=identity(log),
                initial=first, final=final, factory=factory, salt=salt, earned_ticks=3000, supplied_stock_or_progress=False,
                retained_harness=identity(retained), native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_production.cpp"),
                         identity(Path(__file__).with_name("surface_entry_chemical_checks-4.py")),
                         identity(Path(__file__).with_name("surface_chemical_ledger.py"))])


def main():
    """Retain one bounded genuine construction interval without a readiness grant."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source-receipt", type=Path, required=True)
    parser.add_argument("--require-ready", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    receipt, target = [(ROOT / path).resolve() for path in (args.source_receipt, args.output)]
    assert receipt.is_relative_to(ROOT / "logs") and receipt.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, receipt, args.require_ready)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase="advance",
                                     operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), cash=result["final"]["cash"], ready=result["factory"]["complete"])), flush=True)


if __name__ == "__main__":
    main()
