"""Pay lawful native passenger coverage and measure its actual funded interval.

@file surface_entry_passenger_coverage_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSDE.
The sole150invoice and actual fares change cash; all original materials, useful
services, rolling stock and Space owners remain independently decoded.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_income_capitalization_checks-1.py"))
observe = BASE["observe"]
PHASES = {name: "staged-chemical-capital-coverage-" + name for name in ("buy", "work")}


def execute(configuration, phase, receipt, recover_run=None, limit=3000):
    """Pay only the admitted station extension or advance freshly funded native service.

    @param configuration Independently linked focused native application.
    @param phase Separate actual native buy or bounded work boundary.
    @param receipt Exact immediately preceding original, never an ordinary copy.
    @param recover_run Successful invocation to inspect without repeating it.
    @param limit Explicit native work cap in1..3000, freshly quoted before execution.
    @return Actual cash, physical custody, clock and measured financial result.
    """
    assert 0 < limit <= 3000
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    expected = ("staged-chemical-capital-income-query",) if phase == "buy" else tuple(PHASES.values())
    assert previous["complete"] and previous["phase"] in expected
    files = BASE["BASE"]["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_income_capitalization_checks-1.py",
        ROOT / "src/tests/surface_entry_income_capitalization.h", ROOT / "src/tests/surface_entry_passenger_coverage.h",
        ROOT / "src/economy.cpp", ROOT / "src/town_cmd.cpp", ROOT / "src/station_cmd.cpp"]
    operation = native(configuration, PHASES[phase], receipt, observe,
        dict(paid="COVERAGE_PAID", admission="COVERAGE_ADMISSION", progress="COVERAGE_PROGRESS", fixed="INCOME_FIXED_PRICES",
             invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"),
        files, recover_run=recover_run, chemical_limit=limit)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert last["new_fuel_at_center"] and not last["handling"]["jobs"]
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    if phase == "buy":
        paid, = rows["paid"]
        invoice, = rows["invoices"]
        assert paid["invoice"] == invoice["cost"] == 150 and paid["tile"] == 1055 and paid["station"] == 20
        assert paid["changed_orders"] == paid["added_buses"] == paid["credit"] == 0
        assert first["company"]["cash"] == 95487 and last["company"]["cash"] == 95337 and elapsed == 0
        # Building on a real road may consume normal native random bits, but
        # cannot advance a calendar or alter any existing vehicle/packet/order.
        before_chunks, output_chunks = [chunks(path)[1] for path in (before, output)]
        for key in (b"VEHS", b"CAPA", b"ORDL"):
            assert before_chunks[key] == output_chunks[key]
        dates = [TABLE(path, b"DATE")[0] for path in (before, output)]
        assert {key: value for key, value in dates[0].items() if not key.startswith("random_state")} == {key: value for key, value in dates[1].items() if not key.startswith("random_state")}
    else:
        admission, = rows["admission"]
        progress, = rows["progress"]
        earned, = rows["earned"]
        assert admission["funded"] and admission["target"] <= first["company"]["cash"]
        assert admission["ticks"] == limit and admission["reserve"] == 40000
        assert admission["purchases"] == admission["future_income_counted"] == admission["refunds_counted"] == admission["credit"] == 0
        assert 0 < elapsed == earned["earned"] == progress["ticks"] <= limit
        assert progress["restored"] == (last["company"]["cash"] >= 100000)
        assert progress["restored"] or elapsed == limit
        assert last["company"]["cash"] >= 40000 and not progress["module_capital_complete"]
        expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
        assert sum(expenses.values()) == first["company"]["cash"] - last["company"]["cash"]
        operation["measured_cash_flow"] = dict(ticks=elapsed, actual_fares=-expenses[7] - expenses[8],
            interest=expenses[11], other_operating_cost=sum(value for key, value in expenses.items() if key not in (7, 8, 11)),
            cash_delta=last["company"]["cash"] - first["company"]["cash"], annual_projection=False)
    operation["passenger_state"] = BASE["passenger_state"](output)
    return operation


def main():
    """Publish one immutable paid coverage or actual bounded measurement receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=tuple(PHASES), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--limit", type=int, default=3000)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, ROOT / args.receipt, args.recover_run, args.limit)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     phase=PHASES[args.phase], operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
