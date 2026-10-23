"""Query actual launch savings and repay only real cash above the native reserve.

@file surface_entry_launch_savings_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIED.
Original commands and company books own every payment and town-rating change.
Read-only recovery can finalize a successful native save without repurchasing it.
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
from surface_propellant_ledger import TABLE, observe, reserve_identity

FIELDS = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-2.py")))["fields"]
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))["known_revision"]
FUEL_IDENTITY = runpy.run_path(str(Path(__file__).with_name("surface_entry_propellant_checks-8.py")))["fuel_identity"]


def execute(configuration, phase, receipt=None, recover_run=None):
    """@param configuration Actual focused native build. @param phase Pure quote or admitted original civic work.
    @param receipt Immediate selected predecessor, or canonical completed fuel input.
    @param recover_run Existing successful native invocation to inspect without executing again.
    @return Immutable actual payment, physical ownership and native-process evidence.
    """
    foundation_path = ROOT / "logs/surface-entry-launch-site-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"]
    if receipt is None:
        entry, = [row for row in foundation["configurations"] if row["configuration"] == configuration]
        original = entry["output"]
    else:
        predecessor = json.loads(receipt.read_text(encoding="utf-8"))
        assert predecessor["complete"] and predecessor["configuration"] == configuration
        original = predecessor["operation"]["output"]
    source = ROOT / original["path"]; assert identity(source) == original
    scenario = "surface-entry-launch-savings-" + phase
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_launch\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"
        assert retained.is_file()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    log = context.logs / "native.log"
    if recover_run is None:
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-launch]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                                OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_LAUNCH_PHASE=phase))
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and f"SURFACE_LAUNCH_COMPLETE phase={phase}" in terminal, log
    assert identity(source) == original
    before, output = [context.saves / f"surface-launch-{name}-440.sav" for name in ("before-" + phase, phase)]
    saved, first, final = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == final.keys() and len(first) == 121
    assert all(saved[key] == first[key] for key in saved if key != b"GLOG")
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    assert first[b"GLOG"] == final[b"GLOG"]
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith(prefix + " ")]
            for name, prefix in dict(plan="SURFACE_SAVINGS_PLAN", models="SURFACE_SAVINGS_MODEL", repayment="SURFACE_SAVINGS_REPAY", invoices="SURFACE_PASSENGER_INVOICE").items()}
    initial, observed = observe(before), observe(output)
    assert {k: v for k, v in initial.items() if k != "company"} == {k: v for k, v in observed.items() if k != "company"}
    differences = [key.decode("ascii") for key in first if first[key] != final[key]]
    if phase == "savings-query":
        assert first == final and initial == observed
        plan, = rows["plan"]; assert plan["reserve"] == 150000 and plan["milestone"] == 325000 and not plan["complete_launch"]
        assert plan["cash"] == initial["company"]["cash"] and plan["debt"] == initial["company"]["debt"] and plan["ceiling"] == 5000000
        assert plan["spendable"] == plan["cash"] + plan["ceiling"] - plan["debt"]
        assert len(rows["models"]) == 3 and all(row["model"] == 26 and row["maximum"] == 9150 and row["safe_periods"] > 0 for row in rows["models"])
        assert sum(row["cost"] for row in rows["models"]) == plan["gross_renewal"]
        assert plan["milestone"] >= plan["gross_renewal"] + plan["reserve"]
    else:
        paid, = rows["repayment"]; invoice, = rows["invoices"]
        assert differences == ["PLYR"] and invoice["command"] == "CmdDecreaseLoan" and invoice["cost"] == 0
        assert paid["reserve"] == 150000 and paid["ceiling"] == 5000000
        assert 0 < paid["amount"] <= initial["company"]["debt"] and paid["amount"] % 10000 == 0
        assert initial["company"]["cash"] - observed["company"]["cash"] == initial["company"]["debt"] - observed["company"]["debt"] == paid["amount"]
        assert observed["company"]["cash"] >= 150000
        old_company, new_company = TABLE(before, b"PLYR")[0], TABLE(output, b"PLYR")[0]
        assert {k: v for k, v in old_company.items() if k not in ("money", "current_loan")} == {k: v for k, v in new_company.items() if k not in ("money", "current_loan")}
        assert paid["spendable"] == observed["company"]["cash"] + 5000000 - observed["company"]["debt"] == initial["company"]["cash"] + 5000000 - initial["company"]["debt"]
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert processes
    for reference in processes:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    return dict(foundation=identity(foundation_path), receipt=identity(receipt) if receipt else None, source=original,
                before=identity(before), output=identity(output), application=identity(application), terminal=identity(log), rows=rows,
                initial=initial, final=observed, changed_families=differences, retained_harness=identity(retained),
                native_processes=processes, sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_launch.cpp"),
                identity(ROOT / "src/tests/surface_passenger_layout.h"), identity(ROOT / "src/tests/multilayer/acceptance/surface_propellant_ledger.py")])


def main():
    """Run or recover one explicitly selected boundary and retain its exact original output."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("savings-query", "repay"), required=True)
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, (ROOT / args.receipt).resolve() if args.receipt else None, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase,
                                     operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
