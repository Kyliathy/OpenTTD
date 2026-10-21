"""Follow only the actual five loaded Farm7units to their first native rail transfer.

@file surface_entry_farm_feeder_follow_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGR.
The entire fresh operating bill is admitted without future fares or virtual
transfer credit. Native packets, clocks and company books retain ownership.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_farm_feeder_work_checks-1.py"))
observe = BASE["observe"]
PHASE = "staged-chemical-capital-farm-feeder-follow"


def execute(configuration, receipt, recover_run=None):
    """Earn one separately funded actual loaded-five first-transfer followthrough.

    @param configuration Independently linked focused native application.
    @param receipt Exact preceding actual first feeder-work receipt with five Source7units aboard62.
    @param recover_run Successful invocation to inspect without repeating native work.
    @return Literal native transfer or late partial, original custody and measured company cash.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] == BASE["PHASES"]["work"]
    assert receipt.name == f"surface-entry-farm-feeder-work-01-{configuration}.json"
    body = previous["operation"]["final"]["farm_road"]
    assert body["vehicle"] == 62 and body["held"] == 5
    assert all(p["source_type"] == 0 and p["source_id"] == 7 and p["source"] == 22 for p in body["packets"])
    assert not previous["operation"]["measured_cash_flow"]["first_transfer"]
    # Reuse the preceding explicit producer's source membership, resolving
    # current identities after its exact changed authorities were retained.
    files = [ROOT / row["path"] for row in previous["operation"]["sources"]] + [Path(__file__)]
    operation = native(configuration, PHASE, receipt, observe,
        dict(service="FARM_FEEDER_SERVICE", admission="FARM_ROAD_WORK_ADMISSION", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"),
        files, recover_run=recover_run, chemical_limit=2000)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["company"]["cash"] == 58572 and first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    admission, = rows["admission"]
    assert admission["funded"] and admission["ticks"] == 2000 and admission["unpaid"] == admission["extra_running"] == 0
    assert admission["target"] == admission["operating"] + 40000 <= first["company"]["cash"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    earned, = rows["earned"]
    service, = rows["service"]
    assert 0 < elapsed == earned["earned"] == service["ticks"] <= 2000
    assert earned["reached"] or elapsed == 2000
    assert service["vehicle"] == 62 and service["pickup"] == BASE["pickup_identity"](output) == 22 and service["transfer"] == 15
    assert service["picked_up"] and service["maximum_loaded"] >= 5
    assert not service["transferred"] or service["held"] == 0
    assert service["held"] == last["farm_road"]["held"] and service["partial"] == (not service["transferred"])
    assert not service["feeder_credit_is_cash"] and not service["final_payment_proven"]
    assert service["restored"] == (last["company"]["cash"] >= 100000) and last["company"]["cash"] >= 40000
    delta = last["farm_road"]["profit256"] - first["farm_road"]["profit256"]
    if first["farm_road"]["previous_profit256"] != last["farm_road"]["previous_profit256"]:
        delta += last["farm_road"]["previous_profit256"]
    assert service["profit_delta_256"] == delta
    expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
    assert sum(expenses.values()) == first["company"]["cash"] - last["company"]["cash"]
    operation["measured_cash_flow"] = dict(ticks=elapsed, actual_fares=-expenses[7]-expenses[8], interest=expenses[11],
        other_operating_cost=sum(v for k,v in expenses.items() if k not in (7,8,11)), cash_delta=last["company"]["cash"]-first["company"]["cash"],
        virtual_truck_profit_256=delta, first_transfer=service["transferred"], feeder_credit_is_cash=False, final_payment_proven=False, annual_projection=False)
    return operation


def main():
    """Publish one immutable actual loaded-five transfer followthrough without any replay."""
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
