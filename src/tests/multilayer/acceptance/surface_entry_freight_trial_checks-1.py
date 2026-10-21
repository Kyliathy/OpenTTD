"""Apply only free native freight pickup changes and measure actual bounded service.

@file surface_entry_freight_trial_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSC.
Whole operating admission excludes future fares. Original capacities, physical
cargo, useful roles and financial owners remain strict throughout the trial.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
PREVIOUS = runpy.run_path(str(DIRECTORY / "surface_entry_freight_efficiency_checks-1.py"))
observe = PREVIOUS["observe"]
PHASES = {name: "staged-chemical-capital-throughput-" + name for name in ("change", "work")}


def execute(configuration, phase, receipt, recover_run=None, limit=3000):
    """Apply one actual pickup-policy stage or earn one newly funded native interval.

    @param configuration Independently linked focused native application.
    @param phase Separate free order changes or actual bounded work.
    @param receipt Immediately preceding original proposal/change/partial trial.
    @param recover_run Successful invocation to inspect without repeating native commands/work.
    @param limit Explicit actual cap in1..3000, freshly quoted before advancing; smaller windows may preserve year-price safety.
    @return Actual commands, fares, operating costs, lumpy monthly interest and sole native ownership.
    """
    assert 0 < limit <= 3000
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["configuration"] == configuration
    expected = ("staged-chemical-capital-throughput-query",) if phase == "change" else (PHASES["change"], PHASES["work"])
    assert predecessor["phase"] in expected
    if predecessor["phase"] == PHASES["work"]:
        previous, = predecessor["operation"]["rows"]["progress"]
        assert previous["partial"] and not previous["restored"]
    files = PREVIOUS["PREVIOUS"]["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_freight_efficiency_checks-1.py",
        ROOT / "src/tests/surface_entry_freight_efficiency.h", ROOT / "src/economy.cpp", ROOT / "src/ground_vehicle.hpp"]
    operation = native(configuration, PHASES[phase], receipt, observe,
        dict(admission="RECOVERY_ADMISSION", changed="THROUGHPUT_CHANGED", invoices="INVOICE", progress="RECOVERY_PROGRESS",
             earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"), files, recover_run=recover_run, chemical_limit=limit)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == predecessor["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert last["new_fuel_at_center"] and not last["handling"]["jobs"]
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    admission, = rows["admission"]
    assert admission["funded"] and admission["target"] <= first["company"]["cash"]
    assert admission["ticks"] == (3000 if phase == "change" else limit)
    assert admission["future_income_counted"] == admission["refunds_counted"] == admission["credit"] == admission["purchases"] == 0
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    if phase == "change":
        assert elapsed == 0 and first == last and len(rows["changed"]) == len(rows["invoices"]) == 3
        assert all(row["cost"] == 0 for row in rows["invoices"])
        assert all(row["load"] == 0 and row["original_load"] in (2, 3) for row in rows["changed"])
        assert TABLE(before, b"DATE") == TABLE(output, b"DATE")
        assert TABLE(before, b"CAPA") == TABLE(output, b"CAPA")
    else:
        progress, = rows["progress"]
        earned, = rows["earned"]
        assert 0 < elapsed == earned["earned"] == progress["ticks"] <= limit
        assert progress["restored"] == (last["company"]["cash"] >= 100000)
        assert progress["restored"] or elapsed == limit
        assert earned["cash_before"] - earned["cash"] == sum(row["amount"] for row in rows["expenses"])
        assert progress["cash"] == last["company"]["cash"] >= 40000 and not progress["module_capital_complete"]
        classes = {row["type"]: row["amount"] for row in rows["expenses"]}
        # Interest falls on calendar boundaries, so retain it separately from
        # the observed fare/operating margin rather than extrapolating a year
        # from unequal month counts or a shorter successful cash-stop interval.
        operation["measured_cash_flow"] = dict(ticks=elapsed, actual_fares=-classes[7] - classes[8],
            interest=classes[11], other_operating_cost=sum(value for key, value in classes.items() if key not in (7, 8, 11)),
            surplus_before_interest=-sum(value for key, value in classes.items() if key != 11),
            cash_delta=last["company"]["cash"] - first["company"]["cash"], annual_projection=False)
    return operation


def main():
    """Publish one immutable native pickup-policy or actual freight-trial boundary."""
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
