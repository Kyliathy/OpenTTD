"""Settle the real joint rail cohort under its separately priced short startup reserve.

@file surface_entry_joint_settlement_work_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZO.
Native packets can merge both real truck deliveries. A whole train fare or
accepted quantity is never attributed solely to25or counted before settlement.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT,chunks,identity
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BUDGET = runpy.run_path(str(DIRECTORY/"surface_entry_joint_settlement_budget_checks-1.py"))
RAIL = runpy.run_path(str(DIRECTORY/"surface_entry_goods_rail_delivery_checks-1.py"))
observe,rail_stock,TRAIN = [RAIL[k] for k in ("observe","rail_stock","TRAIN")]
PHASES = dict(work="staged-chemical-capital-joint-settlement-work",payment="staged-chemical-capital-joint-settlement-payment")
MAX_WINDOWS = 4


def execute(configuration,receipt,window,recover_run=None):
    """Earn one finite joint-feeder transport or qualified pending-payment interval.

    @param configuration Independently linked focused native application.
    @param receipt Exact bound GZNstartup comparison or this scope's preceding original.
    @param window Explicit strictly consecutive1..4ordinal, never a saving-scope extension.
    @param recover_run Successful native invocation to inspect without repeating ticks.
    @return Real rail custody/acceptance, whole unloading settlement, actual books and finite stop.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration and 1 <= window <= MAX_WINDOWS
    accepted_before = 0
    if window == 1:
        assert previous["phase"] == BUDGET["PHASE"] and previous["operation"]["candidate_settlement_admitted"]
        assert previous["operation"]["candidate_reserve"] == 30000 and previous["operation"]["complete_startup_bill"]["candidate_funded"]
        anchor = receipt
    else:
        assert previous["phase"] in PHASES.values()
        scope = previous["operation"]["rail_scope"]
        assert scope["window"] == window-1 and not scope["ended"]
        anchor = ROOT/previous["operation"]["transfer_receipt"]["path"]
        assert identity(anchor) == previous["operation"]["transfer_receipt"]
        accepted_before = previous["operation"]["cumulative_rail_accepted"]
    root = json.loads(anchor.read_text(encoding="utf-8"))
    assert root["complete"] and root["configuration"] == configuration and root["operation"]["candidate_settlement_admitted"]
    assert previous["operation"]["final"]["company"]["cash"] < 100000
    source = ROOT/previous["operation"]["output"]["path"]
    assert json.loads(json.dumps(observe(source))) == previous["operation"]["final"]
    initial = rail_stock(source)
    pending_before = [r for r in TABLE(source,b"CAPY").values() if r["front"] == 50]
    # Only actual earlier acceptance can qualify this positive payment-only
    # continuation. An unrelated old train unload cannot fund or complete
    # the newly supplied first13cohort's transport boundary.
    settlement = bool(accepted_before > 0 and initial["quantity"] == 0 and pending_before and pending_before[0]["route_profit"] > 0)
    phase = PHASES["payment"] if settlement else PHASES["work"]
    files = [ROOT/r["path"] for r in previous["operation"]["sources"]]+[Path(__file__)]
    operation = native(configuration,phase,receipt,observe,
        dict(admission="GOODS_RAIL_FEEDER_WORK_ADMISSION",selection="GOODS_RAIL_DELIVERY_SELECTION",service="GOODS_RAIL_DELIVERY_SERVICE",
             earned="EARNED",expenses="EXPENSE"),files,recover_run=recover_run,chemical_limit=2000)
    operation["transfer_receipt"] = identity(anchor)
    operation["startup_reserve"] = 30000
    first,last,rows = operation["initial"],operation["final"],operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL":32,"GOOD":50,"PROP":38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before,output = [ROOT/operation[k]["path"] for k in ("before","output")]
    initial,final = [rail_stock(p) for p in (before,output)]
    elapsed = TABLE(output,b"DATE")[0]["tick_counter"]-TABLE(before,b"DATE")[0]["tick_counter"]
    selected, = rows["selection"]
    cases = rows["admission"]
    assert selected["ceiling"] == 2000 and selected["settlement"] == settlement and selected["initial"] == initial["quantity"]
    assert all(r["reserve"] == 30000 and r["ticks"] <= 2000 and r["unpaid"] == 0 and r["target"] == r["operating"]+30000 and r["future_income_counted"] == 0 for r in cases)
    refused = not selected["found"]
    if refused:
        assert not any(r["funded"] for r in cases) and not rows["service"] and not rows["earned"]
        assert elapsed == 0 and chunks(before)[1] == chunks(output)[1]
        posted,cumulative = False,accepted_before
        measured = dict(ticks=0,actual_fares=0,interest=0,other_operating_cost=0,cash_delta=0)
    else:
        assert cases[-1]["funded"] and not any(r["funded"] for r in cases[:-1])
        assert cases[-1]["ticks"] == selected["ticks"] and cases[-1]["target"] <= first["company"]["cash"]
        service, = rows["service"]
        earned, = rows["earned"]
        assert 0 < elapsed == service["ticks"] == earned["earned"] <= selected["ticks"] <= 2000
        assert service["initial"] == initial["quantity"] and service["aboard"] == final["quantity"] and service["share"] == final["share"]
        assert initial["quantity"]+service["boarded"] == service["accepted"]+final["quantity"]
        assert service["settlement"] == settlement and service["stop_on_payment"]
        assert not service["cohort_only_fare_claimed"] and not service["feeder_credit_is_cash"]
        pending = [r for r in TABLE(output,b"CAPY").values() if r["front"] == 50]
        assert service["payment_pending"] == bool(pending)
        cumulative,posted = accepted_before+service["accepted"],service["payment_posted"]
        if posted:
            assert cumulative > 0 and service["observed_whole_unload_fare"] > 0 and not pending and final["quantity"] == 0
            assert TABLE(output,b"VEHS")[49][TRAIN+"last_station_visited"] == 11
        assert earned["reached"] == (posted or last["company"]["cash"] >= 100000)
        assert earned["reached"] or elapsed == selected["ticks"]
        a,b = [TABLE(p,b"VEHS")[49] for p in (before,output)]
        profit = b[TRAIN+"profit_this_year"]-a[TRAIN+"profit_this_year"]
        if a[TRAIN+"profit_last_year"] != b[TRAIN+"profit_last_year"]:
            profit += b[TRAIN+"profit_last_year"]
        assert service["profit_delta_256"] == profit
        expenses = {r["type"]:r["amount"] for r in rows["expenses"]}
        assert sum(expenses.values()) == first["company"]["cash"]-last["company"]["cash"]
        measured = dict(ticks=elapsed,selected_cap=selected["ticks"],actual_fares=-expenses[7]-expenses[8],interest=expenses[11],
            other_operating_cost=sum(v for k,v in expenses.items() if k not in (7,8,11)),cash_delta=last["company"]["cash"]-first["company"]["cash"],
            rail_boarded=service["boarded"],rail_accepted=service["accepted"],rail_aboard=final["quantity"],rail_share=final["share"],
            observed_whole_unload_fare=service["observed_whole_unload_fare"],cohort_only_fare_claimed=False,annual_projection=False)
    assert last["company"]["cash"] >= 30000
    # A posted unload, actual cash goal, funding refusal or hard cap ends this
    # transport scope. No forecast can admit a later fifth interval.
    reason = ("posted_payment" if posted else "cash_goal" if last["company"]["cash"] >= 100000 else
              "funding_refused" if refused else "window_cap" if window == MAX_WINDOWS else None)
    operation.update(work_refused=refused,rail_payment_posted=posted,cumulative_rail_accepted=cumulative,measured_cash_flow=measured,
        rail_scope=dict(window=window,max_windows=MAX_WINDOWS,ended=reason is not None,end_reason=reason))
    return phase,operation


def main():
    """Publish one actual finite joint rail transport or qualified native settlement interval."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"),required=True)
    parser.add_argument("--receipt",type=Path,required=True)
    parser.add_argument("--window",type=int,required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT/args.output
    assert output.resolve().is_relative_to(ROOT/"logs") and not output.exists()
    phase,operation = execute(args.configuration,ROOT/args.receipt,args.window,args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__,complete=True,configuration=args.configuration,
        phase=phase,operation=operation),indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
