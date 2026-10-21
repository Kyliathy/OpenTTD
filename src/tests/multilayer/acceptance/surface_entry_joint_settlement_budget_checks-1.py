"""Price complete short joint-feeder settlement work under explicit startup reserves.

@file surface_entry_joint_settlement_budget_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZN.
The candidate30000floor is a pure planning comparison. Actual cash, service
lives, monthly debits and unpaid rail cargo grant neither income nor work.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT,chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY/"surface_entry_joint_goods_rail_checks-1.py"))
observe = BASE["observe"]
PHASE = "staged-chemical-capital-joint-settlement-budget-query"


def execute(configuration,receipt,recover_run=None):
    """Compare current whole bills and actual near-term renewal obligations without mutation.

    @param configuration Independently linked focused native application.
    @param receipt Exact ended GZMpartial with179actual Goods aboard and no final payment.
    @param recover_run Successful pure invocation to inspect without a new process.
    @return Unchanged owners, whole current costs and explicitly unspent candidate admission.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration and previous["phase"] in BASE["PHASES"].values()
    assert previous["operation"]["rail_scope"] == dict(window=8,max_windows=8,ended=True,end_reason="window_cap")
    assert not previous["operation"]["rail_payment_posted"] and previous["operation"]["final"]["company"]["cash"] == 44172
    files = [ROOT/r["path"] for r in previous["operation"]["sources"]]+[Path(__file__)]
    operation = native(configuration,PHASE,receipt,observe,
        dict(source="JOINT_SETTLEMENT_BUDGET_SOURCE",admission="JOINT_SETTLEMENT_BUDGET_ADMISSION",services="CAPITAL_SERVICE"),
        files,recover_run=recover_run,chemical_limit=2000)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before,output = [ROOT/operation[k]["path"] for k in ("before","output")]
    assert chunks(before)[1] == chunks(output)[1]
    source, = operation["rows"]["source"]
    assert source["rail"] == 49 and source["first13"] == 179 and source["virtual_share"] == 294
    assert source["last_station"] == 12 and source["receiver"] == 11 and not source["payment_pending"]
    assert source["cash"] == 44172 and source["working_goal"] == 100000 and source["module_reserve"] == 150000
    assert source["incomplete_module_base"] == 510000 and source["paid"] == source["elapsed"] == 0 and not source["work_permission"]
    vehicles = TABLE(output,b"VEHS")
    services = {r["vehicle"]:r for r in operation["rows"]["services"] if vehicles[r["vehicle"]]["type"] in (0,1)}
    assert len(services) == source["active_ground"] == 15
    remaining = []
    # The native quote names only real active primary services. Cross-check
    # their current saved lives; parked reserves and wagon bodies cannot
    # hide a renewal bill or supply another service's remaining lifetime.
    for id,row in services.items():
        prefix = "train[0].common[0]." if vehicles[id]["type"] == 0 else "roadveh[0].common[0]."
        value = vehicles[id][prefix+"max_age"]-vehicles[id][prefix+"age"]
        assert value == row["remaining"] and not vehicles[id][prefix+"vehstatus"] & 2
        remaining.append(value)
    assert min(remaining) == source["minimum_remaining"] > 365+source["renewal_lead"]
    assert source["near_due_next_year"] == 0
    cases = operation["rows"]["admission"]
    assert [r["ticks"] for r in cases] == [2000,1000,500,250,64]
    # Both floors cover the identical complete operating bill. Changing a
    # planning reserve neither changes native interest/settings nor borrows
    # against the already boarded but still unpaid Goods.
    for row in cases:
        assert row["historical_reserve"] == 40000 and row["candidate_reserve"] == 30000
        assert row["historical_target"] == row["operating"]+40000 and row["candidate_target"] == row["operating"]+30000
        assert row["cash"] == 44172 and row["historical_funded"] == (row["cash"] >= row["historical_target"])
        assert row["candidate_funded"] == (row["cash"] >= row["candidate_target"])
        assert row["future_income_counted"] == row["resale_counted"] == row["credit"] == 0 and not row["work_permission"]
    assert cases[0]["candidate_funded"] and not cases[0]["historical_funded"]
    operation.update(candidate_settlement_admitted=True,candidate_reserve=30000,complete_startup_bill=cases[0],
        actual_work_complete=False,actual_reserve_restored=False,module_capital_complete=False)
    return operation


def main():
    """Publish a pure complete startup-reserve comparison with no earned work or changed setting."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"),required=True)
    parser.add_argument("--receipt",type=Path,required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT/args.output
    assert output.resolve().is_relative_to(ROOT/"logs") and not output.exists()
    operation = execute(args.configuration,ROOT/args.receipt,args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__,complete=True,configuration=args.configuration,
        phase=PHASE,operation=operation),indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
