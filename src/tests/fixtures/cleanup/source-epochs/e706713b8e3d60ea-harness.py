"""Characterize actual Propellant capital and native ordinary freight opportunities.

@file surface_entry_fuel_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BI.1.
All native families must survive pure quotes. Neither an unsold asset value nor
a quoted tariff becomes company cash, cargo or an accepted income route.
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
from surface_chemical_ledger import products, salt_ledger, chemical_ledger

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]


def execute(configuration, receipt):
    """Query original capital, actual industry providers and possible receiving bays.

    @param configuration Independently linked focused native application.
    @param receipt Exact original first21CHEM depot-custody receipt.
    @return Unchanged physical/financial owners and original native quote fields.
    """
    selected = json.loads(receipt.read_text(encoding="utf-8"))
    assert selected["complete"] and selected["configuration"] == configuration and selected["phase"] == "collect"
    source = ROOT / selected["operation"]["output"]["path"]
    assert identity(source) == selected["operation"]["output"]
    context = create_run_context("space", "surface-entry-fuel-query")
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_fuel\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "query.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-fuel]"], cwd=ROOT / "build", stdout=sink,
                            stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source)))
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_FUEL_COMPLETE pure=true" in terminal, log
    assert identity(source) == original
    before, output = [context.saves / name for name in ("surface-fuel-before-query-440.sav", "surface-fuel-query-440.sav")]
    assert chunks(before) == chunks(output)
    first, final = [products(path) for path in (source, output)]
    assert first == final and final["cash"] == 181985 and final["debt"] == 4650000
    assert chemical_ledger(source) == chemical_ledger(output) and chemical_ledger(output)["useful_custody"]
    assert salt_ledger(source) == salt_ledger(output)
    prefixes = dict(capital="CAPITAL", train="EXISTING_TRAIN", industries="INDUSTRY", providers="PROVIDER", receivers="RECEIVER",
                    towns="TOWN", tariffs="TARIFF", goods_bays="GOODS_BAY", goods_trucks="GOODS_TRUCK",
                    conversions="CONVERSION", rail_carriers="RAIL_CARRIER")
    quotes = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_FUEL_" + prefix + " ")]
              for name, prefix in prefixes.items()}
    assert len(quotes["capital"]) == 1 and len(quotes["train"]) == 1
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application),
                terminal=identity(log), quotes=quotes, products=final, unchanged_native_families=len(chunks(before)[1]),
                simulation_ticks=0, supplied_stock_or_capital=False, selected_income_route=False,
                retained_harness=identity(retained), native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_fuel.cpp"), identity(ROOT / "src/tests/surface_chemical_layout.h"),
                         identity(Path(__file__).with_name("surface_chemical_ledger.py"))])


def main():
    """Retain actual pure characterization without claiming an affordable completed arrangement."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source-receipt", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    receipt, target = [(ROOT / path).resolve() for path in (args.source_receipt, args.output)]
    assert receipt.is_relative_to(ROOT / "logs") and receipt.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, receipt)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), capital=result["quotes"]["capital"])), flush=True)


if __name__ == "__main__":
    main()
