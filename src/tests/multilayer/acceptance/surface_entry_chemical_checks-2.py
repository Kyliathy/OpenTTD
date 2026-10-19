"""Characterize genuine chemical sites, native freight and capital without spending.

@file surface_entry_chemical_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.1.
The exact first Surface-Salt checkpoint is immutable. Native before/after saves
must preserve every owner, including finite reserves and physical cargo packets.
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

SALT = runpy.run_path(str(Path(__file__).with_name("surface_entry_salt_mine_checks-6.py")))
OWNERS = SALT["OWNERS"]


def fields(line):
    """Decode one bounded native diagnostic row without supplying gameplay state.

    @param line Native exported space-separated key=value diagnostic.
    @return Typed original fields, preserving stable labels and command names.
    """
    row = {}
    for part in line.split()[1:]:
        name, value = part.split("=", 1)
        if value in ("true", "false"):
            row[name] = value == "true"
        else:
            row[name] = int(value) if value.lstrip("-").isdigit() else value
    return row


def execute(configuration, receipt):
    """Query actual factories, compatible carriers and available original capital.

    @param configuration Independently linked focused native application.
    @param receipt Exact accepted first-Surface-Salt native haul receipt.
    @return Immutable producer input, all owner equality and actual native quotes.
    """
    selected = json.loads(receipt.read_text(encoding="utf-8"))
    # Continuation branches cannot replace the selected first real60Salt stock.
    assert selected["complete"] and selected["configuration"] == configuration
    assert selected["phase"] == "drive" and selected["operation"]["native"]["surface_custody"]
    source = ROOT / selected["operation"]["output"]["path"]
    assert identity(source) == selected["operation"]["output"]
    context = create_run_context("space", "surface-entry-chemical-query")
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_chemical\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "query.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-chemical]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source)))
    terminal = log.read_text(encoding="utf-8", errors="replace")
    # Both retained and current observers explicitly mark the pure boundary;
    # complete native before/after equality below remains the authority.
    pure = any(marker in terminal for marker in ("SURFACE_CHEMICAL_COMPLETE pure=true", "SURFACE_CHEMICAL_COMPLETE phase=query pure=true"))
    assert result.returncode == 0 and "All tests passed" in terminal and pure, log
    assert identity(source) == original
    before, output = [context.saves / name for name in ("surface-chemical-before-440.sav", "surface-chemical-query-440.sav")]
    first, final = [chunks(path)[1] for path in (before, output)]
    assert first == final, [key.decode() for key in first if first[key] != final[key]]
    initial, observed = [OWNERS["observe"](path) for path in (source, output)]
    assert initial == observed and observed["debt"] == 4250000 and observed["cash"] == 88274
    salt = SALT["observe"](output)
    assert salt["surface_custody"] and salt["wagon"]["held"] == 60 and salt["mine"]["reserve"] == 23679
    prefixes = dict(world="WORLD", recipes="RECIPE", sites="SITE", carriers="CARRIER", carrier_counts="CARRIER_COUNT", oil="OIL", recovery="RECOVERY",
                    bays="BAY", roads="ROAD", infrastructure="INFRASTRUCTURE", capital="CAPITAL")
    rows = {name: [fields(line) for line in terminal.splitlines() if line.startswith("SURFACE_CHEMICAL_" + prefix + " ")]
            for name, prefix in prefixes.items()}
    assert len(rows["world"]) == 1 and len(rows["recipes"]) == 2
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output),
                application=identity(application), terminal=identity(log), quotes=rows, products=observed, salt=salt,
                unchanged_native_families=len(first), simulation_ticks=0, supplied_stock_or_capital=False,
                retained_harness=identity(retained), native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_chemical.cpp"), identity(ROOT / "src/tests/surface_chemical_layout.h")])


def main():
    """Retain one pure native characterization; it does not close the whole task."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source-receipt", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    receipt, target = [(ROOT / path).resolve() for path in (args.source_receipt, args.output)]
    assert receipt.is_relative_to(ROOT / "logs") and receipt.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    operation = execute(args.configuration, receipt)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     operation=operation), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), quotes=operation["quotes"])), flush=True)


if __name__ == "__main__":
    main()
