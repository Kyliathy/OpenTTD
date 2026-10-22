"""Earn remaining launch capital with whole fuel custody and actual service-life guards.

@file surface_entry_capital_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEK.
Existing native annual work and matching repayment commands retain all owners;
current42unit fuel custody never weakens the closed original20unit contract.
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
from surface_fuel_refill_ledger import TABLE, observe, reserve_identity

DIRECTORY = Path(__file__).resolve().parent
FIELDS = runpy.run_path(str(DIRECTORY / "surface_entry_chemical_checks-2.py"))["fields"]
REVISION = runpy.run_path(str(DIRECTORY / "surface_entry_checks-7.py"))["known_revision"]
RESTORE_PAUSE = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_checks-12.py"))["restore_pause"]
OLD_FUEL = runpy.run_path(str(DIRECTORY / "surface_entry_propellant_checks-8.py"))["fuel_identity"]
NEW_FUEL = runpy.run_path(str(DIRECTORY / "surface_entry_fuel_refill_checks-3.py"))["fuel_identity"]
GOODS = runpy.run_path(str(DIRECTORY / "surface_entry_goods_checks-1.py"))["goods_identity"]


def lives(path):
    """@param path Complete original save. @return Current useful eight-service identities and actual remaining days."""
    result = []
    for key, row in TABLE(path, b"VEHS").items():
        if row["type"] not in (0, 1):
            continue
        prefix = "train[0].common[0]." if row["type"] == 0 else "roadveh[0].common[0]."
        name = bytes.fromhex(row[prefix + "name"]).decode("utf-8")
        active = name.startswith("Surface Income ") or name.startswith("Surface Launch passenger ") or name in ("Surface ore delivery", "Surface Steel delivery")
        if not active:
            continue
        result.append(dict(vehicle=key, name=name, engine=row[prefix + "engine_type"], age=row[prefix + "age"],
                           remaining=row[prefix + "max_age"] - row[prefix + "age"]))
    assert len(result) == 8
    return result


def plan():
    """@return Actual current funds, available native matching repayment and all service-life guards, without game mutation."""
    foundation_path = ROOT / "logs/surface-entry-bus-reload-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"] and foundation["actual_bus_renewal_complete"]
    summaries, worlds = [], []
    for entry in foundation["configurations"]:
        source = ROOT / entry["output"]["path"]; assert identity(source) == entry["output"]
        snapshot = observe(source); service = lives(source); assert all(row["remaining"] > 366 for row in service)
        company = snapshot["company"]; amount = min(company["debt"], (company["cash"] - 150000) // 10000 * 10000)
        assert amount > 0 and amount % 10000 == 0 and company["cash"] - amount >= 150000
        spendable = company["cash"] + 5000000 - company["debt"]; assert spendable == 327464
        summaries.append(dict(configuration=entry["configuration"], receipt=entry["receipt"], output=entry["output"], company=company,
                              spendable=spendable, target=1870000, reserve=150000, matching_repayment=amount, service_lives=service))
        worlds.append(chunks(source)[1])
    assert worlds[0].keys() == worlds[1].keys() and all(worlds[0][key] == worlds[1][key] for key in worlds[0] if key != b"GLOG")
    return dict(_documentation=__doc__, complete=True, actual_capital_complete=False, foundation=identity(foundation_path),
                cross_build_gameplay_comparisons=120, native_commands_queried=False, configurations=summaries,
                native_contracts=[identity(ROOT / "src/tests/surface_entry_launch.cpp"), identity(ROOT / "src/misc_cmd.cpp")], source=identity(Path(__file__)))


def execute(configuration, phase, receipt, recover_run=None, ticks=27000):
    """@param configuration Actual independently linked focused build.
    @param phase Original bounded earnings or native matching repayment.
    @param receipt Exact immediate selected predecessor. @param recover_run Existing successful native work to inspect only.
    @param ticks Explicit9000or27000tick native interval, never installed simulation time.
    @return Complete actual financial, material, vehicle-life and native-process evidence.
    """
    assert phase in ("earn", "repay") and ticks in (9000, 27000)
    predecessor = json.loads(receipt.read_text(encoding="utf-8")); assert predecessor["complete"] and predecessor["configuration"] == configuration
    original = predecessor["operation"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    service = lives(source)
    if phase == "earn":
        assert all(row["remaining"] > 366 for row in service), service
    scenario = "surface-entry-capital-" + phase
    if recover_run is None:
        context = create_run_context("space", scenario); context.prepare_runtime()
        retained = context.directory / "harness.py"; retained.write_bytes(Path(__file__).read_bytes())
        (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_launch\n", encoding="utf-8")
    else:
        context = RunContext(ROOT, "space", scenario, identifier(recover_run)); retained = context.directory / "harness.py"; assert retained.is_file()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"; log = context.logs / "native.log"
    if recover_run is None:
        with log.open("wb") as sink:
            result = run_native(context, [str(application), "[.surface-entry-launch]"], cwd=ROOT / "build", stdout=sink,
                                stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration,
                                OTTD_SURFACE_ENTRY_SOURCE=str(source), OTTD_SURFACE_LAUNCH_PHASE=phase, OTTD_SURFACE_SAVINGS_TICKS=str(ticks)))
        assert result.returncode == 0, log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and f"SURFACE_LAUNCH_COMPLETE phase={phase}" in terminal, log
    before, output = [context.saves / f"surface-launch-{name}-440.sav" for name in ("before-" + phase, phase)]
    saved, first, last = [chunks(path)[1] for path in (source, before, output)]
    assert saved.keys() == first.keys() == last.keys() and len(first) == 121
    assert all(saved[key] == first[key] for key in saved if key not in (b"GLOG", b"DATE"))
    source_pause = None
    if saved[b"DATE"] != first[b"DATE"]:
        # Only the original restored due-worker transition65to1is qualified;
        # no RNG, tick, date or other owner receives a blanket exception.
        assert TABLE(source, b"DATE")[0]["pause_mode"] == 65 and TABLE(before, b"DATE")[0]["pause_mode"] == 1
        source_pause = RESTORE_PAUSE(before, source)
    if saved[b"GLOG"] != first[b"GLOG"]:
        REVISION(source, before)
    assert first[b"GLOG"] == last[b"GLOG"]
    rows = {name: [FIELDS(line) for line in terminal.splitlines() if line.startswith(prefix + " ")]
            for name, prefix in dict(earned="SURFACE_PASSENGER_EARNED", expenses="SURFACE_PASSENGER_EXPENSE", delivered="SURFACE_PASSENGER_DELIVERED",
                                     profit="SURFACE_PASSENGER_PROFIT", repayment="SURFACE_SAVINGS_REPAY", invoices="SURFACE_PASSENGER_INVOICE").items()}
    initial, final = observe(before), observe(output)
    assert reserve_identity(initial) == reserve_identity(final) and GOODS(initial) == GOODS(final)
    assert OLD_FUEL(initial) == OLD_FUEL(final) and NEW_FUEL(initial) == NEW_FUEL(final) and initial["plant"] == final["plant"]
    assert [(row["head"], row["chain"]) for row in initial["trains"]] == [(row["head"], row["chain"]) for row in final["trains"]]
    if phase == "earn":
        earned, = rows["earned"]; assert earned["earned"] == ticks
        assert TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"] == ticks
        assert sum(row["amount"] for row in rows["expenses"]) == earned["net_expense"] == initial["company"]["cash"] - final["company"]["cash"]
        assert initial["company"]["debt"] == final["company"]["debt"] <= 5000000 and not rows["invoices"]
        profit, = rows["profit"]; assert profit["buses"] == 3 and profit["fraction"] == 256
        assert all(row["remaining"] > 0 for row in lives(output))
    else:
        assert [key for key in first if first[key] != last[key]] == [b"PLYR"]
        paid, = rows["repayment"]; invoice, = rows["invoices"]; assert invoice["command"] == "CmdDecreaseLoan" and invoice["cost"] == 0
        assert paid["amount"] == min(initial["company"]["debt"], (initial["company"]["cash"] - 150000) // 10000 * 10000) > 0
        assert initial["company"]["cash"] - final["company"]["cash"] == initial["company"]["debt"] - final["company"]["debt"] == paid["amount"]
        assert final["company"]["cash"] >= 150000 and paid["ceiling"] == 5000000
        old_company, new_company = TABLE(before, b"PLYR")[0], TABLE(output, b"PLYR")[0]
        assert {k: v for k, v in old_company.items() if k not in ("money", "current_loan")} == {k: v for k, v in new_company.items() if k not in ("money", "current_loan")}
    assert final["company"]["cash"] > 0 and identity(source) == original
    processes = [identity(path) for path in sorted(context.reports.glob("native-*.json"))]; assert processes
    for reference in processes:
        report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
        assert report["exit_status"] == 0 and report["inputs"] == [original] and report["executable"] == identity(application)
    return dict(receipt=identity(receipt), source=original, before=identity(before), output=identity(output), application=identity(application), terminal=identity(log),
                rows=rows, initial=initial, final=final, service_lives=lives(output), source_pause=source_pause, retained_harness=identity(retained), native_processes=processes,
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_launch.cpp"), identity(ROOT / "src/tests/surface_entry_work.h"),
                         identity(DIRECTORY / "surface_fuel_refill_ledger.py"), identity(DIRECTORY / "surface_propellant_ledger.py")])


def main():
    """Retain current eligibility or one actual original savings boundary without repeating successful work."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--phase", choices=("plan", "earn", "repay"), required=True); parser.add_argument("--receipt", type=Path)
    parser.add_argument("--ticks", type=int, choices=(9000, 27000), default=27000); parser.add_argument("--recover-run"); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    if args.phase == "plan":
        value = plan()
    else:
        operation = execute(args.configuration, args.phase, (ROOT / args.receipt).resolve(), args.recover_run, args.ticks)
        value = dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase, operation=operation)
    output.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
