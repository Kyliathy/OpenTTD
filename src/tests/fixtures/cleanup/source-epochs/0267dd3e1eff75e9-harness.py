"""Characterize the complete current genuine launch arrangement and capital target.

@file surface_entry_gateway_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEI.1-2.
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

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))["known_revision"]
RESTORE_PAUSE = runpy.run_path(str(Path(__file__).with_name("surface_entry_fuel_checks-12.py")))["restore_pause"]


def execute(configuration, receipt, recover_run=None):
    """@param configuration Actual independently linked focused build.
    @param receipt Exact selected original complete-fuel continuation receipt.
    @param recover_run Existing successful native query to inspect without executing again.
    @return Whole original admission, unchanged gameplay and actual native process evidence.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8")); assert predecessor["complete"] and predecessor["configuration"] == configuration
    original = predecessor["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    scenario = "surface-entry-gateway-query"
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_gateway\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"; assert retained.is_file()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"; log = context.logs / "native.log"
    if recover_run is None:
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-gateway]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source)))
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and "SURFACE_GATEWAY_COMPLETE " in terminal, log
    before, output = [context.saves / f"surface-gateway-{name}-440.sav" for name in ("before-query", "query")]
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
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_GATEWAY_" + prefix + " ")]
            for name, prefix in dict(plan="PLAN", zones="ZONE", member="MEMBER", carriers="CARRIER", recipes="RECIPE", inputs="INPUT", mass="MASS", service="SERVICE").items()}
    initial, final_snapshot = observe(before), observe(output); assert initial == final_snapshot
    plan, = rows["plan"]; member, = rows["member"]; mass, = rows["mass"]
    assert plan["site"] == 2890 and plan["rotation"] == 1 and plan["width"] == 3 and plan["height"] == 4
    assert plan["center_quote"] >= 500000 and plan["core_quote"] == 700000 and plan["hull_bound"] == 500000
    assert plan["steel"] == 36 and plan["goods"] == 50 and plan["flight_fuel"] == 30 and plan["shipment_units"] == 128
    assert plan["handling_ticks"] == 16 and plan["handling_jobs"] == 3 and plan["hull_work"] == 296
    assert plan["mission_fee"] == 10000 and plan["pad_bound"] == 3000 and plan["builder_work_bound"] == 5000 and plan["reserve"] == 150000
    assert plan["planned"] == sum(plan[key] for key in ("center_quote", "hull_bound", "core_quote", "handling_fee", "mission_fee", "pad_bound", "builder_work_bound", "reserve"))
    assert plan["target"] == ((plan["planned"] + 9999) // 10000) * 10000
    assert plan["spendable"] == plan["cash"] + plan["ceiling"] - plan["debt"] and plan["ceiling"] == 5000000
    assert plan["gap"] == plan["target"] - plan["spendable"] > 0 and not plan["complete_capital"]
    assert member["station"] == 0 and member["geometry_fits"] and not member["membership_admitted"] and member["infrastructure"] == 0
    assert member["physical_tiles"] > 0 and member["radius"] == 20 and member["crossings"] == 0
    assert len(rows["zones"]) == 4 and {row["kind"] for row in rows["zones"]} == {0, 1, 2, 3} and all(row["prospective_geometry"] for row in rows["zones"])
    assert len(rows["carriers"]) == 7 and all(row["transfer_admitted"] and row["return_admitted"] for row in rows["carriers"])
    assert sum(row["held"] for row in rows["carriers"] if row["label"] == "STEL") == 36
    assert sum(row["held"] for row in rows["carriers"] if row["label"] == "GOOD") == 50
    assert sum(row["held"] for row in rows["carriers"] if row["label"] == "PROP") == 42
    assert len(rows["recipes"]) == 3 and all(not row["future_assembly_admitted"] for row in rows["recipes"])
    assert sum(row["quantity"] for row in rows["inputs"] if row["label"] == "STEL") == 32
    assert sum(row["quantity"] for row in rows["inputs"] if row["label"] == "GOOD") == 48
    assert mass["first_payload"] == mass["builder"] + mass["first_cargo"] <= mass["rocket_capacity"]
    assert mass["first_cargo"] <= mass["builder_capacity"] and mass["second_cargo"] <= min(mass["builder_capacity"], mass["rocket_capacity"])
    assert mass["fuel_hold"] >= 30 and len(rows["service"]) == 8 and all(row["remaining"] > 0 and row["orders"] == 2 for row in rows["service"])
    assert identity(source) == original
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert processes
    for reference in processes:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application),
                terminal=identity(log), rows=rows, initial=initial, final=final_snapshot, source_pause=source_pause,
                unchanged_families=121, native_processes=processes, retained_harness=identity(retained),
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_gateway.cpp"),
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
