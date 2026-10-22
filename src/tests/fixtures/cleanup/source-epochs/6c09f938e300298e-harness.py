"""Buy actual electric Salt transport and retain its strictly capped native work.

@file surface_entry_salt_window_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CIC.2/.3.
Actual commands own the coupled bodies; normal movement/loading owns all Salt.
One500tick work interval follows one paid purchase. No unearned sale is funded.
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
from surface_gateway_component_delivery_ledger import observe_salt_window as observe
from surface_gateway_native import FIELDS, REVISION, PAUSE
from surface_regional_ledger import TABLE

DIRECTORY = Path(__file__).resolve().parent
FILES = runpy.run_path(str(DIRECTORY / "surface_entry_component_delivery_checks-1.py"))["FILES"]


def execute(configuration, phase, receipt, recover_run=None):
    """Execute or recover one real coupled-train purchase or bounded finite-Salt interval.

    @param configuration Independently rebuilt actual focused application.
    @param phase Native staged-buy or staged-work boundary.
    @param receipt Exact original immediately preceding conversion or purchase.
    @param recover_run Successful invocation to recover without repeating payment or work.
    @return Exact native invoices, literal Source14packets and original owner conservation.
    """
    value = json.loads(receipt.read_text(encoding="utf-8"))
    assert value["complete"] and value["configuration"] == configuration
    assert value["phase"] == ("staged-convert" if phase == "staged-buy" else "staged-buy")
    original = value["operation"]["output"]
    source = ROOT / original["path"]
    assert identity(source) == original
    scenario = "surface-entry-resupply-" + phase
    if recover_run is None:
        context = create_run_context("space", scenario)
        context.prepare_runtime()
        retained = context.directory / "harness.py"
        retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_resupply\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run))
        retained = context.directory / "harness.py"
        assert retained.is_file()
    application = ROOT / f"build/{configuration}/openttd_surface_supply_test.exe"
    log = context.logs / "native.log"
    if recover_run is None:
        environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source),
                           OTTD_SURFACE_RESUPPLY_PHASE=phase)
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-resupply]"], cwd=ROOT / "build",
                                stdout=sink, stderr=subprocess.STDOUT, timeout=110, env=environment)
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and "SURFACE_RESUPPLY_COMPLETE phase=" + phase in terminal, log
    before, output = [context.saves / f"surface-resupply-{name}-440.sav" for name in ("before-" + phase, phase)]
    saved, first, last = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == last.keys() and len(first) == 121
    assert all(saved[key] == first[key] for key in saved if key not in (b"GLOG", b"DATE"))
    pause = None
    if saved[b"DATE"] != first[b"DATE"]:
        assert TABLE(source, b"DATE")[0]["pause_mode"] == 65 and TABLE(before, b"DATE")[0]["pause_mode"] == 1
        pause = PAUSE(before, source)
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    prefixes = dict(peak="PEAK", purchase="ELECTRIC_PURCHASE", progress="ELECTRIC_PROGRESS",
                    invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED")
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith("SURFACE_RESUPPLY_" + prefix + " ")]
            for name, prefix in prefixes.items()}
    initial, final = observe(before), observe(output)
    assert json.loads(json.dumps(initial)) == value["operation"]["final"]
    assert initial["totals"] == final["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 12}
    assert initial["flight"] == final["flight"] and initial["orbital_consumed"] == final["orbital_consumed"]
    assert initial["company"]["debt"] == final["company"]["debt"] == 5000000
    assert final["company"]["cash"] >= 100000 and len(final["service_lives"]) == 11
    salt = final["salt_consist"]
    if phase == "staged-buy":
        purchase, = rows["purchase"]
        assert "salt_consist" not in initial and salt["held"] == 0
        assert purchase["head"] == salt["head"] and purchase["wagon"] == salt["wagon"]
        assert purchase["gross"] == sum(row["cost"] for row in rows["invoices"]) == 35653
        assert final["company"]["cash"] == initial["company"]["cash"] - purchase["gross"] == 101351
        assert first[b"DATE"] == last[b"DATE"] and purchase["earned"] == 0
    else:
        earned, = rows["earned"]
        progress, = rows["progress"]
        peak, = rows["peak"]
        assert not rows["invoices"] and 0 < earned["earned"] <= 500
        assert progress["earned"] == earned["earned"] and progress["held"] == salt["held"]
        assert progress["collected"] == (salt["held"] == 60 and salt["home"])
        assert TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"] == earned["earned"]
        assert earned["net_expense"] == sum(row["amount"] for row in rows["expenses"])
        assert final["company"]["cash"] == initial["company"]["cash"] - earned["net_expense"]
        # Gross original operating costs must fit the exact quoted short
        # window even if useful original services also earn unrelated income.
        # Those fares are actual native receipts, never advance purchase cash.
        costs = sum(row["amount"] for row in rows["expenses"] if row["amount"] > 0)
        assert costs <= peak["work_bound"] and peak["funded"] and peak["refund_counted"] == 0
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]
    assert len(processes) == 1
    report = json.loads((ROOT / processes[0]["path"]).read_text(encoding="utf-8"))
    assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    assert identity(source) == original
    authorities = [ROOT / name for name in ("src/tests/surface_entry_resupply.cpp", "src/train_cmd.cpp", "src/layer_portal_cmd.cpp", "src/space_vehicle.cpp")]
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application),
                terminal=identity(log), rows=rows, initial=initial, final=final, source_pause=pause, retained_harness=identity(retained),
                native_processes=processes, sources=[identity(Path(__file__))] + [identity(DIRECTORY / name) for name in FILES] + [identity(path) for path in authorities])


def main():
    """Publish one paid original Salt stage, preserving successful invocations for exact recovery."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("staged-buy", "staged-work"), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                      phase=args.phase, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
