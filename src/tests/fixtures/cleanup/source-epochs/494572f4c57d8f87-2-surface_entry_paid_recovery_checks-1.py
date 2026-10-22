"""Recover genuine working cash through at most three conditionally funded windows.

@file surface_entry_paid_recovery_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGJ.
Each whole6000cost bill must fit then-current cash before its separately quoted
3000tick intervals. Actual100000cash stops recovery immediately.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
CYCLE_PATH = DIRECTORY / "surface_entry_farm_cycle_checks-1.py"
CYCLE = runpy.run_path(str(CYCLE_PATH))
observe = CYCLE["observe"]
QUERY = "staged-chemical-capital-paid-recovery-query"
WORK = "staged-chemical-capital-paid-recovery-work"


def execute(configuration, window, stage, receipt, recover_run=None):
    """Run a new whole-window admission or its next independently priced interval.

    @param configuration Independently linked focused native application.
    @param window Explicit conditional window1..3; no fourth window is admitted.
    @param stage Query, first work or second/final work within that window.
    @param receipt Exact immediately preceding actual native original receipt.
    @param recover_run Successful invocation to inspect without repeating work.
    @return Actual producers, all owned/material/Space ledgers and funding/cash outcome.
    """
    assert 1 <= window <= 3
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["operation"]["final"]["company"]["cash"] < 100000
    prior = previous["operation"]
    if stage == "query":
        if window == 1:
            assert receipt.name == f"surface-entry-farm-cycle-work-02-{configuration}.json"
            assert previous["phase"] == CYCLE["WORK"] and prior["cycle_ticks"] == 6000
        else:
            assert previous["phase"] == WORK and prior["recovery_window"] == window - 1
            assert prior["recovery_stage"] == "work-02" and prior["window_ticks"] == 6000
    else:
        assert prior["recovery_window"] == window
        assert prior["recovery_stage"] == ("query" if stage == "work-01" else "work-01")
        assert previous["phase"] == (QUERY if stage == "work-01" else WORK)
    root = receipt if stage == "work-01" else ROOT / prior["window_admission"]["path"] if stage == "work-02" else None
    if stage == "work-02":
        assert identity(root) == prior["window_admission"]
    if root is not None:
        admission = json.loads(root.read_text(encoding="utf-8"))["operation"]
        whole, = [row for row in admission["rows"]["whole"] if row["reserve"] == 40000]
        assert whole["funded"] and whole["target"] <= admission["initial"]["company"]["cash"] == whole["cash"]
    phase = QUERY if stage == "query" else WORK
    files = CYCLE["BASE"]["BASE"]["FREIGHT"]["caller_files"]() + [Path(__file__), CYCLE_PATH,
        DIRECTORY / "surface_entry_farm_rebalance_checks-1.py", ROOT / "src/tests/surface_entry_paid_recovery.h",
        ROOT / "src/tests/surface_entry_farm_cycle.h", ROOT / "src/tests/surface_entry_farm_rebalance.h",
        ROOT / "src/tests/surface_entry_farm_upgrade.h", ROOT / "src/tests/surface_entry_module_capital.h",
        ROOT / "src/tests/surface_entry_income_capitalization.h", ROOT / "src/economy.cpp"]
    operation = native(configuration, phase, receipt, observe,
        dict(whole="RECOVERY_ADMISSION", progress="RECOVERY_PROGRESS", rail="CYCLE_RAIL", offers="CYCLE_OFFER",
             admission="FARM_UPGRADE_ADMISSION", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"),
        files, recover_run=recover_run, chemical_limit=3000)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == prior["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    operation["producers_before"], operation["producers_after"] = CYCLE["producers"](before), CYCLE["producers"](output)
    assert CYCLE["BASE"]["strict_default_rejects"](output)
    if stage == "query":
        assert chunks(before)[1] == chunks(output)[1] and first == last
        assert [row["reserve"] for row in rows["whole"]] == [100000, 40000]
        assert not rows["whole"][0]["funded"]
        for row in rows["whole"]:
            assert row["cash"] == first["company"]["cash"] and row["ticks"] == 6000
            assert row["target"] == row["reserve"] + row["operating"]
            assert row["operating"] == row["upkeep"] + row["interest"] + row["station_charge"]
            assert row["purchases"] == row["future_income_counted"] == row["refunds_counted"] == row["credit"] == 0
        elapsed = 0
    else:
        elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
        earned, = rows["earned"]
        progress, = rows["progress"]
        assert 0 < elapsed == earned["earned"] == progress["ticks"] <= 3000
        assert earned["reached"] == progress["restored"] == (last["company"]["cash"] >= 100000)
        assert earned["reached"] or elapsed == 3000
        assert all(row["funded"] and row["target"] <= first["company"]["cash"] for row in rows["admission"])
        assert last["company"]["cash"] >= 40000
        expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
        assert sum(expenses.values()) == first["company"]["cash"] - last["company"]["cash"]
        operation["measured_cash_flow"] = dict(ticks=elapsed, actual_fares=-expenses[7] - expenses[8], interest=expenses[11],
            other_operating_cost=sum(v for k, v in expenses.items() if k not in (7, 8, 11)),
            cash_delta=last["company"]["cash"] - first["company"]["cash"], annual_projection=False)
        operation["window_admission"] = identity(root)
    operation.update(recovery_window=window, recovery_stage=stage,
        window_ticks=elapsed + (prior["window_ticks"] if stage == "work-02" else 0))
    assert operation["window_ticks"] <= 6000
    return operation


def main():
    """Publish an immutable actual recovery boundary; successful native work is never replayed."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--window", type=int, choices=(1, 2, 3), required=True)
    parser.add_argument("--stage", choices=("query", "work-01", "work-02"), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.window, args.stage, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     phase=QUERY if args.stage == "query" else WORK, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
