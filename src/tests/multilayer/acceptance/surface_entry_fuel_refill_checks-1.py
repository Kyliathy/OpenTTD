"""Admit current original additional Propellant conversion and physical pickup.

@file surface_entry_fuel_refill_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEH.1.
Pure queries preserve all121original families. An explicitly named refill
observer retains the default original closed material contract unchanged.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from artifact_paths import RunContext, create_run_context, identifier
from runner import run_native
from space_preview_checks import ROOT, chunks, identity
from surface_fuel_refill_ledger import TABLE, observe
from surface_resupply_ledger import observe as observe_original

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))["known_revision"]
RESTORE_PAUSE = runpy.run_path(str(Path(__file__).with_name("surface_entry_fuel_checks-12.py")))["restore_pause"]


def execute(configuration, receipt, recover_run=None):
    """@param configuration Actual independently linked focused build.
    @param receipt Exact selected original chemical-refill continuation receipt.
    @param recover_run Existing successful native query to inspect without executing again.
    @return Whole original admission, unchanged gameplay and actual native process evidence.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8")); assert predecessor["complete"] and predecessor["configuration"] == configuration
    original = predecessor["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    scenario = "surface-entry-fuel-refill-query"
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_fuel_refill\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"; assert retained.is_file()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"; log = context.logs / "native.log"
    if recover_run is None:
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-fuel-refill]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source)))
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and "SURFACE_FUEL_REFILL_COMPLETE phase=query" in terminal, log
    before, output = [context.saves / f"surface-fuel-refill-{name}-440.sav" for name in ("before-query", "query")]
    saved, first, final = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == final.keys() and len(first) == 121 and first == final
    assert all(saved[key] == first[key] for key in saved if key not in (b"GLOG", b"DATE"))
    source_pause = None
    if saved[b"DATE"] != first[b"DATE"]:
        # Only a real restarted due-worker job may explain65to1; preserve
        # every other actual DATE/RNG/tick field through the known owner check.
        assert TABLE(source, b"DATE")[0]["pause_mode"] == 65 and TABLE(before, b"DATE")[0]["pause_mode"] == 1
        source_pause = RESTORE_PAUSE(before, source)
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_FUEL_REFILL_" + prefix + " ")]
            for name, prefix in dict(plan="PLAN", routes="ROUTE", bays="BAY").items()}
    initial, final_snapshot = observe(before), observe(output); assert initial == final_snapshot
    assert observe_original(before) == observe_original(output)
    refill = initial["propellant_refill"]
    assert refill["external_chemicals"] == 42 and refill["new_produced"] == refill["new_dispatched"] == refill["ordinary_loss"] == 0 and refill["truck"] is None
    assert initial["chemical_feed"]["held"] == 21 and initial["plant"]["chemicals"] == 1 and initial["plant"]["consumed"] == 20
    plan, = rows["plan"]
    assert plan["industry"] == 6 and plan["external_chemicals"] == 42 and plan["feed"] == 21 and plan["input"] == 1 and plan["consumed"] == 20
    assert plan["original_fuel"] == 20 and plan["flight_fuel"] == 30 and plan["expected_additional"] == 22 and plan["minimum_additional"] == 10
    assert plan["capacity"] >= 22 and plan["gross"] > 0 and plan["protected_trains"] == 3
    assert plan["cash"] + plan["credit"] - plan["gross"] >= plan["reserve"] == 100000
    assert plan["credit"] % 10000 == 0 and plan["debt"] + plan["credit"] <= plan["ceiling"] == 5000000
    assert plan["infrastructure"] == 0 and not plan["full_load"] and plan["output_refusal_retains_buffer"]
    assert len(rows["routes"]) == len(rows["bays"]) == 2
    assert {row["target"] for row in rows["routes"]} == {1895, 1896}
    assert sum(row["crossings"] for row in rows["routes"]) == plan["route_crossings"]
    assert all(row["infrastructure"] == 0 for row in rows["routes"])
    assert all(row["catchment"] and row["order_admitted"] and not row["full_load"] for row in rows["bays"])
    assert identity(source) == original
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert processes
    for reference in processes:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application),
                terminal=identity(log), rows=rows, initial=initial, final=final_snapshot, source_pause=source_pause,
                unchanged_families=121, native_processes=processes, retained_harness=identity(retained),
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_fuel_refill.cpp"),
                         identity(Path(__file__).with_name("surface_fuel_refill_ledger.py")), identity(Path(__file__).with_name("surface_propellant_ledger.py"))])


def main():
    """Retain one original complete pure query without advancing the selected world."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--receipt", type=Path, required=True); parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, (ROOT / args.receipt).resolve(), args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase="query", operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
