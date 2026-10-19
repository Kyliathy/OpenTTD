"""Earn actual final road travel for useful native chemical depot custody.

@file surface_entry_chemical_checks-11.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.3.
The real loaded tanker must finish its original depot order. The observer stops
at actual arrival, without assigning a pose, stock or extra simulation ticks.
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
from surface_chemical_ledger import TABLE, products, salt_ledger, chemical_ledger


def execute(configuration, receipt):
    """Earn only the selected already-loaded chemical tanker's remaining native trip.

    @param configuration Independently linked focused native application.
    @param receipt Exact genuine42CHEM production checkpoint with21actually aboard.
    @return Actual paid tanker depot custody and all conserved original materials.
    """
    selected = json.loads(receipt.read_text(encoding="utf-8"))
    assert selected["complete"] and selected["configuration"] == configuration and selected["phase"] == "drive"
    source = ROOT / selected["operation"]["output"]["path"]
    assert identity(source) == selected["operation"]["output"]
    initial_chemicals = chemical_ledger(source)
    assert initial_chemicals["reserve"]["held"] == 21
    context = create_run_context("space", "surface-entry-chemical-depot-custody")
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_chemical\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "collect.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-chemical]"], cwd=ROOT / "build", stdout=sink,
                            stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_CHEMICAL_PHASE="collect"))
    terminal = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"SURFACE_CHEMICAL_COLLECTED vehicle=(\d+) earned=(\d+) held=21 depot=2443", terminal)
    assert result.returncode == 0 and "All tests passed" in terminal and match, log
    assert identity(source) == original
    output = context.saves / "surface-chemical-collect-440.sav"
    first, final = [products(path) for path in (source, output)]
    assert first["debt"] == final["debt"] == 4650000
    assert first["cash"] - final["cash"] == final["expense"] - first["expense"]
    dates = [TABLE(path, b"DATE")[0] for path in (source, output)]
    earned = dates[1]["tick_counter"] - dates[0]["tick_counter"]
    assert 0 < earned < 3000 and earned == int(match[2]) and dates[1]["id"] == dates[0]["id"]
    a, b = [chunks(path)[1] for path in (source, output)]
    for family in (b"GEOS", b"GSPN", b"GVNT", b"XJOB", b"XJRT", b"XJTL"):
        assert a[family] == b[family], family
    salt, chemicals = salt_ledger(output), chemical_ledger(output)
    assert salt["delivered"] == 60 and salt["wagon_held"] == 0
    assert chemicals["useful_custody"] and chemicals["reserve"]["id"] == int(match[1])
    return dict(receipt=identity(receipt), source=original, output=identity(output), application=identity(application), terminal=identity(log),
                initial=first, final=final, salt=salt, chemicals=chemicals, earned_ticks=earned, supplied_stock_or_pose=False,
                retained_harness=identity(retained), native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_chemical.cpp"),
                         identity(Path(__file__).with_name("surface_chemical_ledger.py"))])


def main():
    """Publish original-source21CHEM physically stopped in the real road depot."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source-receipt", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    receipt, target = [(ROOT / path).resolve() for path in (args.source_receipt, args.output)]
    assert receipt.is_relative_to(ROOT / "logs") and receipt.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, receipt)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase="collect",
                                     operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), earned=result["earned_ticks"], cash=result["final"]["cash"])), flush=True)


if __name__ == "__main__":
    main()
