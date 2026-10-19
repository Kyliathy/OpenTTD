"""Query actual current expansion and literal refill renewal/funding boundaries.

@file surface_entry_supply_admission_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CH.2.
Both producers remain pure. Quoted future carriers, buffers and credit cannot
become physical fuel, paid renewal or completed logistics in this receipt.
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
from surface_gateway_component_delivery_ledger import observe_retained as observe
from surface_gateway_native import execute as gateway, FIELDS, REVISION, PAUSE
from surface_regional_ledger import TABLE

DIRECTORY = Path(__file__).resolve().parent
CONSUMERS = runpy.run_path(str(DIRECTORY / "surface_entry_component_delivery_checks-1.py"))["FILES"]


def execute(configuration, boundary, receipt, recover_run=None):
    """@param configuration Actual independently linked native build. @param boundary Current modules or literal fuel-refill admission.
    @param receipt Exact selected original predecessor. @param recover_run Successful native invocation to inspect without repetition.
    @return Pure current actual quotes, unchanged original owners and explicit unfulfilled prerequisites.
    """
    files = [Path(__file__)] + [DIRECTORY / name for name in CONSUMERS]
    files += [ROOT / name for name in ("src/tests/surface_entry_resupply.cpp", "src/multilayer_space_content.cpp", "src/space_vehicle.cpp")]
    if boundary == "modules":
        prefixes = dict(layout="EXPANSION_LAYOUT", inputs="EXPANSION_INPUT", fuel="EXPANSION_FUEL", budget="EXPANSION_BUDGET", flight="EXPANSION_FLIGHT", invoices="INVOICE")
        current = gateway(configuration, "expansion-query", receipt, observe, prefixes, files, recover_run)
        rows = current["rows"]; budget, = rows["budget"]; fuel, = rows["fuel"]
        assert [(row["needed"], row["held"], row["missing"], row["load_admitted"]) for row in rows["inputs"]] == [(16, 32, 0, True), (28, 50, 0, True)]
        assert fuel["needed"] == 30 and fuel["held"] == 12 and fuel["missing"] == 18 and not fuel["fuel_load_admitted"]
        assert budget["base"] == 510000 and budget["cash"] == 143673 and budget["debt"] == budget["ceiling"] == 5000000
        assert budget["credit"] == 370000 and not budget["bank_admitted"] and not budget["funded"] and not budget["logistics_costed"]
    else:
        assert boundary == "refill"; current = execute_refill(configuration, receipt, files, recover_run)
        rows = current["rows"]; admission, = rows["admission"]
        assert admission["chemical_remaining"] == -30527 and admission["chemical_capacity"] == 21 and admission["minimum_fuel"] == 18
        assert admission["replacement_capacity"] >= 18 and admission["candidate_fuel"] >= 18
        assert admission["target"] == admission["rail_gross"] + admission["replacement_cost"] + admission["work_allowance"] + admission["reserve"] + admission["handling_allowance"]
        assert admission["work_allowance"] == 18000 and admission["reserve"] == 100000
        assert not admission["replacement_owned"] and not admission["full_logistics_bill_complete"]
        assert len(rows["fuel_carriers"]) == 2 and all(row["capacity"] == 27 and row["held"] == 0 and row["halted"] for row in rows["fuel_carriers"])
    assert chunks(ROOT / current["before"]["path"])[1] == chunks(ROOT / current["output"]["path"])[1] and current["initial"] == current["final"]
    return current


def execute_refill(configuration, receipt, files, recover_run):
    """@param configuration Actual focused build. @param receipt Original current module-query checkpoint.
    @param files Exact current consumer/native producer authorities. @param recover_run Successful invocation to decode without repeating its query.
    @return Original pure refill prices, current renewal/life/funding gaps and all unchanged native families.
    """
    value = json.loads(receipt.read_text(encoding="utf-8")); assert value["complete"] and value["configuration"] == configuration
    original = value["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    scenario = "surface-entry-resupply-admission"
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_resupply\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"; assert retained.is_file()
    application = ROOT / f"build/{configuration}/openttd_surface_supply_test.exe"; log = context.logs / "native.log"
    if recover_run is None:
        environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_RESUPPLY_PHASE="admission")
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-resupply]"], cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, timeout=110, env=environment)
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace"); assert "All tests passed" in terminal and "SURFACE_RESUPPLY_COMPLETE phase=admission" in terminal, log
    before, output = [context.saves / f"surface-resupply-{name}-440.sav" for name in ("before-admission", "admission")]
    saved, first, last = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == last.keys() and len(first) == 121 and first == last
    assert all(saved[key] == first[key] for key in saved if key not in (b"GLOG", b"DATE"))
    source_pause = None
    if saved[b"DATE"] != first[b"DATE"]:
        assert TABLE(source, b"DATE")[0]["pause_mode"] == 65 and TABLE(before, b"DATE")[0]["pause_mode"] == 1
        source_pause = PAUSE(before, source)
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert len(processes) == 1
    for reference in processes:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    prefixes = dict(admission="ADMISSION", fuel_carriers="FUEL_CARRIER", rail="RAIL", crossings="CROSSING", custody="CUSTODY")
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_RESUPPLY_" + prefix + " ")] for name, prefix in prefixes.items()}
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application), terminal=identity(log),
                rows=rows, initial=observe(before), final=observe(output), source_pause=source_pause, retained_harness=identity(retained), native_processes=processes,
                sources=[identity(ROOT / "src/tests/surface_entry_resupply.cpp"), identity(ROOT / "src/tests/surface_entry_gateway.cpp")] + [identity(path) for path in files])


def main():
    """Publish one immutable pure current admission from the selected original native world."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--boundary", choices=("modules", "refill"), required=True); parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    current = execute(args.configuration, args.boundary, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.boundary, operation=current), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
