"""Earn genuine Salt/Oil delivery, native chemical batches and physical collection.

@file surface_entry_chemical_checks-9.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.2-3.
The original game loop alone moves real ingredient packets and earns complete
chemical work. Old Steel/Goods and every financial/finite material owner remain explicit.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess

from space_preview_checks import ROOT, chunks, identity
from artifact_paths import create_run_context
from runner import run_native
from surface_chemical_ledger import TABLE, products, salt_ledger, chemical_ledger


def execute(configuration, receipt, require_reserve):
    """Drive one exact real3000tick interval from the actual empty/working fleet.

    @param configuration Independently linked focused native application.
    @param receipt Exact preceding genuine empty fleet or actual work receipt.
    @param require_reserve Require21native CHEM in the halted actual road-depot tanker.
    @return Native work, finite ingredient settlement and actual chemical inventory.
    """
    selected = json.loads(receipt.read_text(encoding="utf-8"))
    assert selected["complete"] and selected["configuration"] == configuration
    assert selected["phase"] in ("fleet", "drive", "restore")
    source = ROOT / selected["operation"]["output"]["path"]
    assert identity(source) == selected["operation"]["output"]
    context = create_run_context("space", "surface-entry-chemical-production")
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_production\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "production.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-production]"], cwd=ROOT / "build", stdout=sink,
                            stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source)))
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal, log
    assert identity(source) == original
    output = context.saves / "surface-entry-replayed-440.sav"
    first, final = [products(path) for path in (source, output)]
    assert first["debt"] == final["debt"] == 4650000
    assert first["cash"] - final["cash"] == final["expense"] - first["expense"]
    dates = [TABLE(path, b"DATE")[0] for path in (source, output)]
    assert dates[1]["tick_counter"] - dates[0]["tick_counter"] == 3000 and dates[1]["id"] == dates[0]["id"]
    a, b = [chunks(path)[1] for path in (source, output)]
    for family in (b"GEOS", b"GSPN", b"GVNT", b"XJOB", b"XJRT", b"XJTL"):
        assert a[family] == b[family], family
    initial_salt, salt = [salt_ledger(path) for path in (source, output)]
    assert salt["mine"]["extracted"] >= initial_salt["mine"]["extracted"]
    assert salt["ordinary_station_loss"] >= initial_salt["ordinary_station_loss"]
    assert salt["delivered"] + salt["wagon_held"] == 60
    initial_chemicals, chemicals = [chemical_ledger(path) for path in (source, output)]
    assert chemicals["produced"] >= initial_chemicals["produced"]
    assert chemicals["accepted"]["SALT"] == salt["delivered"]
    if require_reserve:
        assert chemicals["useful_custody"], chemicals
    return dict(receipt=identity(receipt), source=original, output=identity(output), application=identity(application), terminal=identity(log),
                initial=first, final=final, salt=salt, chemicals=chemicals, earned_ticks=3000, supplied_stock_or_pose=False,
                retained_harness=identity(retained), native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_production.cpp"),
                         identity(Path(__file__).with_name("surface_chemical_ledger.py"))])


def main():
    """Retain one bounded genuine chemical production/physical collection interval."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source-receipt", type=Path, required=True)
    parser.add_argument("--require-reserve", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    receipt, target = [(ROOT / path).resolve() for path in (args.source_receipt, args.output)]
    assert receipt.is_relative_to(ROOT / "logs") and receipt.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, receipt, args.require_reserve)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase="drive",
                                     operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), cash=result["final"]["cash"], produced=result["chemicals"]["produced"],
                          held=result["chemicals"]["reserve"]["held"], useful=result["chemicals"]["useful_custody"])), flush=True)


if __name__ == "__main__":
    main()
