"""Query genuine PCHM factory, protected external chemicals and useful transport.

@file surface_entry_propellant_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BID.1.
All native families must remain unchanged. Neither a full-load assumption nor a
prepared recipe fixture can provide the actual company's useful fuel stock.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity
from surface_income_ledger import observe, reserve_identity
from artifact_paths import create_run_context
from runner import run_native

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]


def execute(configuration):
    """@param configuration Actual focused native application. @return Complete unchanged-world current Propellant quotes."""
    foundation_path = ROOT / "logs/surface-entry-income-capital-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"] and foundation["full_propellant_affordability"]
    entry, = [row for row in foundation["configurations"] if row["configuration"] == configuration]
    source = ROOT / entry["output"]["path"]; assert identity(source) == entry["output"]
    producer_path = ROOT / f"logs/surface-entry-propellant-producers-{configuration}.json"
    producer = json.loads(producer_path.read_text(encoding="utf-8"))
    assert producer["complete"] and producer["operation"]["cases"] == 2 and producer["configuration"] == configuration
    context = create_run_context("space", "surface-entry-propellant-query")
    context.prepare_runtime()
    retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_propellant\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    log = context.logs / "query.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-propellant]"], cwd=ROOT / "build", stdout=sink,
                            stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source)))
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_PROPELLANT_COMPLETE phase=query" in terminal, log
    assert identity(source) == entry["output"]
    before, output = [context.saves / f"surface-propellant-{name}-440.sav" for name in ("before-query", "query")]
    first, final = [chunks(path)[1] for path in (before, output)]; assert first == final and len(first) == 121
    initial, observed = observe(before), observe(output)
    assert initial == observed and reserve_identity(initial) == reserve_identity(observed)
    prefixes = dict(plan="PLAN", routes="ROUTE", bays="BAY", traffic="TRAFFIC")
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_PROPELLANT_" + prefix + " ")]
            for name, prefix in prefixes.items()}
    plan, = rows["plan"]
    assert plan["chemicals"] == 21 and plan["expected_product"] == 20 and plan["residual_chemical"] == 1 and not plan["full_load"]
    assert plan["requirement"] == plan["factory"] + plan["tanker"] + plan["buffer"]
    assert plan["buffer"] == 100000 and plan["cash"] + plan["ceiling"] - plan["debt"] >= plan["requirement"]
    assert len(rows["routes"]) == len(rows["bays"]) == 2 and len(rows["traffic"]) == 3
    assert all(row["infrastructure"] == 0 for row in rows["routes"])
    return dict(foundation=identity(foundation_path), producers=identity(producer_path), source=entry["output"], before=identity(before), output=identity(output),
                application=identity(application), terminal=identity(log), quotes=rows, initial=initial, final=observed,
                unchanged_native_families=121, simulation_ticks=0, retained_harness=identity(retained),
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_propellant.cpp"),
                         identity(ROOT / "src/tests/multilayer/acceptance/surface_income_ledger.py")])


def main():
    """Retain one pure current arrangement without purchasing a factory or credit."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase="query", operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
