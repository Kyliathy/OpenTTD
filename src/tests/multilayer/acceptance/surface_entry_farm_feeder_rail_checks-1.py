"""Observe real first22Farm7cargo boarding original52and finally unloading at13.

@file surface_entry_farm_feeder_rail_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGS.
One complete freshly funded native interval owns all movement and payment.
Older rail cargo fares cannot stand in for this actual new feeder cohort.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_farm_feeder_follow_checks-1.py"))
observe = BASE["observe"]
PHASE = "staged-chemical-capital-farm-feeder-rail"


def execute(configuration, receipt, recover_run=None):
    """Earn one admitted actual rail pickup/final-delivery followthrough or honest late partial.

    @param configuration Independently linked focused native application.
    @param receipt Exact preceding native first-transfer receipt.
    @param recover_run Successful invocation to inspect without repeating work.
    @return Literal cohort boarding/delivery, conserved owners and actual whole-company cash.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] == BASE["PHASE"]
    assert receipt.name == f"surface-entry-farm-feeder-follow-01-{configuration}.json"
    assert previous["operation"]["measured_cash_flow"]["first_transfer"]
    files = [ROOT / row["path"] for row in previous["operation"]["sources"]] + [Path(__file__), ROOT / "src/tests/surface_entry_farm_feeder_rail.h"]
    operation = native(configuration, PHASE, receipt, observe,
        dict(source="FARM_FEEDER_RAIL_SOURCE", service="FARM_FEEDER_RAIL_SERVICE", admission="FARM_ROAD_WORK_ADMISSION",
             earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"), files, recover_run=recover_run, chemical_limit=2000)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["company"]["cash"] == 58044 and first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    source, = rows["source"]
    admission, = rows["admission"]
    assert source["vehicle"] == 52 and source["first_station"] == 22 and source["industry"] == 7 and source["receiver"] == 13
    assert source["waiting"] == 9 and source["feeder_share"] == 3 and source["initial_aboard"] == source["paid"] == source["changed_orders"] == 0
    assert admission["funded"] and admission["ticks"] == 2000 and admission["unpaid"] == admission["extra_running"] == 0
    assert admission["target"] == admission["operating"] + 40000 <= first["company"]["cash"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    earned, = rows["earned"]
    service, = rows["service"]
    assert 0 < elapsed == earned["earned"] == service["ticks"] <= 2000
    assert earned["reached"] == (service["delivered"] > 0) and (earned["reached"] or elapsed == 2000)
    assert service["vehicle"] == 52 and service["boarded"] >= service["maximum_aboard"] >= service["aboard"]
    assert service["boarded"] == service["delivered"] + service["aboard"]
    assert not service["feeder_credit_is_cash"] and service["partial"] == (service["delivered"] == 0)
    assert service["restored"] == (last["company"]["cash"] >= 100000) and last["company"]["cash"] >= 40000
    expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
    assert sum(expenses.values()) == first["company"]["cash"] - last["company"]["cash"]
    paid_livestock = sum(row["amount"] for row in rows["delivered"] if row["label"] == "LVST")
    if service["delivered"] > 0:
        assert paid_livestock >= service["delivered"] and earned["income"] > 0
    operation["measured_cash_flow"] = dict(ticks=elapsed, actual_fares=-expenses[7]-expenses[8], interest=expenses[11],
        other_operating_cost=sum(v for k,v in expenses.items() if k not in (7,8,11)), cash_delta=last["company"]["cash"]-first["company"]["cash"],
        feeder_cohort_boarded=service["boarded"], feeder_cohort_delivered=service["delivered"], feeder_cohort_aboard=service["aboard"],
        all_paid_livestock=paid_livestock, feeder_credit_is_cash=False, annual_projection=False)
    return operation


def main():
    """Publish one immutable original feeder-to-rail delivery observation without replay."""
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
