"""Measure only the completely funded remaining paid-producer observation.

@file surface_entry_paid_continuation_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGD.
The initial6000tick bill is covered before either step. Every actual step also
rechecks native costs and stops on genuinely restored working cash.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_productive_capital_checks-1.py"))
observe = BASE["observe"]
PHASE = "staged-chemical-capital-coverage-work"


def execute(configuration, receipt, recover_run=None):
    """Earn one actual interval inside the initial covered6000tick total.

    @param configuration Independently linked focused native application.
    @param receipt Exact pure complete admission or first actual continuation.
    @param recover_run Successful invocation to inspect without repeating earned work.
    @return Actual financial books, sole Goods custody, bounded time and current reserve outcome.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration
    if previous["phase"] == "staged-chemical-capital-productive-query":
        root, used, window = receipt, 0, 1
    else:
        assert previous["phase"] == PHASE and previous["operation"]["funded_window"] == 1
        reference = previous["operation"]["complete_admission"]
        root = ROOT / reference["path"]
        assert identity(root) == reference
        used, window = previous["operation"]["whole_work_ticks_used"], 2
        assert previous["operation"]["final"]["company"]["cash"] < 100000
    proposal = json.loads(root.read_text(encoding="utf-8"))["operation"]
    whole, = [row for row in proposal["rows"]["admission"] if row["funded"]]
    assert whole["ticks"] == 6000 and whole["target"] == 69409 <= whole["cash"] == 84595
    files = BASE["FREIGHT"]["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_goods_trade_checks-1.py",
        DIRECTORY / "surface_entry_productive_capital_checks-1.py", ROOT / "src/tests/surface_entry_productive_capital.h",
        ROOT / "src/tests/surface_entry_goods_trade.h", ROOT / "src/tests/surface_entry_goods_income.h",
        ROOT / "src/tests/surface_entry_income_capitalization.h", ROOT / "src/tests/surface_entry_passenger_coverage.h",
        ROOT / "src/economy.cpp"]
    operation = native(configuration, PHASE, receipt, observe,
        dict(admission="COVERAGE_ADMISSION", progress="COVERAGE_PROGRESS", fixed="INCOME_FIXED_PRICES",
             earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"), files, recover_run=recover_run, chemical_limit=3000)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    admission, = rows["admission"]
    progress, = rows["progress"]
    earned, = rows["earned"]
    fixed, = rows["fixed"]
    assert admission["funded"] and admission["target"] <= first["company"]["cash"] and admission["reserve"] == 40000
    assert admission["purchases"] == admission["future_income_counted"] == admission["refunds_counted"] == admission["credit"] == 0
    assert 0 < elapsed == earned["earned"] == progress["ticks"] <= 3000 and used + elapsed <= 6000
    assert progress["restored"] == (last["company"]["cash"] >= 100000)
    assert progress["restored"] or elapsed == 3000
    assert last["company"]["cash"] >= 40000 and not progress["module_capital_complete"]
    expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
    assert sum(expenses.values()) == first["company"]["cash"] - last["company"]["cash"]
    # The admitted complete interval stays in its actual native profit year.
    # Body profit deltas demonstrate only those two carriers' own operation,
    # never aggregate road/trains or a predicted quantity of future Goods.
    assert not fixed["year_boundary"]
    profits = {name: value["profit256"] - first["goods_income"][name]["profit256"] for name, value in last["goods_income"].items()}
    operation.update(complete_admission=identity(root), funded_window=window, whole_work_ticks_used=used + elapsed,
        goods_body_operating_profit_delta256=profits, new_goods_payment_observed=any(value > 0 for value in profits.values()),
        measured_cash_flow=dict(ticks=elapsed, actual_fares=-expenses[7] - expenses[8], interest=expenses[11],
            other_operating_cost=sum(value for key, value in expenses.items() if key not in (7, 8, 11)),
            cash_delta=last["company"]["cash"] - first["company"]["cash"], annual_projection=False))
    return operation


def main():
    """Publish one immutable funded continuation under the complete initial bill."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     phase=PHASE, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
