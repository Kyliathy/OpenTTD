"""Admit the current complete finite-Salt and actual chemical-refill arrangement.

@file surface_entry_resupply_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEG.1.
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
from surface_resupply_ledger import TABLE, observe
from surface_propellant_ledger import observe as observe_original

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))["known_revision"]
RESTORE_PAUSE = runpy.run_path(str(Path(__file__).with_name("surface_entry_fuel_checks-12.py")))["restore_pause"]


def execute(configuration, receipt, recover_run=None):
    """@param configuration Actual independently linked focused build.
    @param receipt Exact selected original Goods continuation receipt.
    @param recover_run Existing successful native query to inspect without executing again.
    @return Whole original admission, unchanged gameplay and actual native process evidence.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8")); assert predecessor["complete"] and predecessor["configuration"] == configuration
    original = predecessor["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    scenario = "surface-entry-resupply-query"
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_resupply\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"; assert retained.is_file()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"; log = context.logs / "native.log"
    if recover_run is None:
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-resupply]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source)))
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and "SURFACE_RESUPPLY_COMPLETE phase=query" in terminal, log
    before, output = [context.saves / f"surface-resupply-{name}-440.sav" for name in ("before-query", "query")]
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
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_RESUPPLY_" + prefix + " ")]
            for name, prefix in dict(plan="PLAN", rail="RAIL", crossings="CROSSING").items()}
    initial, final_snapshot = observe(before), observe(output); assert initial == final_snapshot
    assert observe_original(before) == observe_original(output)
    assert {k: v for k, v in initial.items() if k != "chemical_refill"} == observe_original(before)
    refill = initial["chemical_refill"]; assert refill["new_delivered_salt"] == refill["new_held_chemicals"] == 0 and not refill["carriers"]
    plan, = rows["plan"]; rail, = rows["rail"]
    assert plan["gross"] == plan["head_cost"] + plan["wagon_cost"] and plan["salt_capacity"] == 60 and plan["oil"] >= 60
    assert plan["chemical_capacity"] == plan["chemicals"] == 21 and plan["chemical_pickup"] == 8
    assert plan["protected_trains"] == 3 and plan["infrastructure"] == 0 and plan["reserve"] == 100000
    assert plan["cash"] + plan["credit"] - plan["gross"] >= 100000 and plan["credit"] % 10000 == 0
    assert plan["debt"] + plan["credit"] <= plan["ceiling"] == 5000000 and not plan["unsold_value_counted"]
    assert rail["full_return_first"] and rail["chemical_waits_at_source"] and rail["short_crossings"] == 0 and rail["short_tiles"] == 4
    assert len(rows["crossings"]) == plan["road_crossings"] and all(row["y"] > 24 and row["north_delivery_clear"] for row in rows["crossings"])
    mine = TABLE(before, b"INDY")[14]; assert mine["produced[0].waiting"] >= 60
    assert identity(source) == original
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert processes
    for reference in processes:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application),
                terminal=identity(log), rows=rows, initial=initial, final=final_snapshot, source_pause=source_pause,
                unchanged_families=121, native_processes=processes, retained_harness=identity(retained),
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_resupply.cpp"),
                         identity(Path(__file__).with_name("surface_resupply_ledger.py")), identity(Path(__file__).with_name("surface_propellant_ledger.py"))])


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
