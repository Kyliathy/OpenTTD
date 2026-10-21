"""Measure finite actual reserve recovery from the completed paid Goods feeder.

@file surface_entry_feeder_reserve_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZI.
Current cash funds every whole interval. Real growth after each four-window
burst alone permits another burst; the actual cash goal or any refusal ends work.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT,chunks,identity
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
PAID = runpy.run_path(str(DIRECTORY/"surface_entry_goods_rail_delivery_checks-1.py"))
observe,rail_stock,TRAIN = [PAID[k] for k in ("observe","rail_stock","TRAIN")]
PHASE = "staged-chemical-capital-feeder-reserve-work"
MAX_WINDOWS = 40


def execute(configuration,receipt,window,recover_run=None):
    """Earn exactly one finite cash-goal interval with fresh full funding and burst-growth gates.

    @param configuration Independently linked focused native application.
    @param receipt Exact paid-feeder endpoint or this scope's preceding original window.
    @param window Explicit ordinal in1..40, strictly consecutive and never replayed.
    @param recover_run Successful invocation to inspect without repeating earned time.
    @return Actual native costs/cargo/cash, immutable paid root and explicit finite-scope stop.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration and 1 <= window <= MAX_WINDOWS
    assert previous["operation"]["final"]["company"]["cash"] < 100000
    if window == 1:
        assert previous["phase"] in PAID["PHASES"].values() and previous["operation"]["rail_payment_posted"]
        anchor = receipt
        burst_start = previous["operation"]["final"]["company"]["cash"]
    else:
        assert previous["phase"] == PHASE
        scope = previous["operation"]["reserve_scope"]
        assert scope["window"] == window-1 and not scope["ended"]
        anchor = ROOT/previous["operation"]["paid_feeder_receipt"]["path"]
        assert identity(anchor) == previous["operation"]["paid_feeder_receipt"]
        # A completed four-window review must grow actual banked cash
        # before another burst begins. Outstanding cargo and predicted
        # fares cannot reopen a non-growing or refused previous scope.
        if (window-1) % 4 == 0:
            assert scope["burst_complete"] and scope["burst_cash_growth"] > 0
            burst_start = previous["operation"]["final"]["company"]["cash"]
        else:
            burst_start = scope["burst_start_cash"]
    root = json.loads(anchor.read_text(encoding="utf-8"))
    assert root["complete"] and root["configuration"] == configuration and root["operation"]["rail_payment_posted"]
    assert root["operation"]["cumulative_rail_accepted"] > 0
    assert json.loads(json.dumps(observe(ROOT/previous["operation"]["output"]["path"]))) == previous["operation"]["final"]
    files = [ROOT/r["path"] for r in previous["operation"]["sources"]]+[Path(__file__)]
    operation = native(configuration,PHASE,receipt,observe,
        dict(admission="GOODS_RAIL_FEEDER_WORK_ADMISSION",selection="GOODS_RAIL_DELIVERY_SELECTION",
             service="GOODS_RAIL_DELIVERY_SERVICE",earned="EARNED",expenses="EXPENSE"),
        files,recover_run=recover_run,chemical_limit=2000)
    operation.update(paid_feeder_receipt=identity(anchor),banked_sale=root["operation"]["banked_sale"])
    first,last,rows = operation["initial"],operation["final"],operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL":32,"GOOD":50,"PROP":38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before,output = [ROOT/operation[k]["path"] for k in ("before","output")]
    initial,final = [rail_stock(p) for p in (before,output)]
    elapsed = TABLE(output,b"DATE")[0]["tick_counter"]-TABLE(before,b"DATE")[0]["tick_counter"]
    selected, = rows["selection"]
    cases = rows["admission"]
    assert selected["ceiling"] == 2000 and not selected["settlement"] and selected["initial"] == initial["quantity"]
    assert all(r["ticks"] <= 2000 and r["unpaid"] == 0 and r["target"] == r["operating"]+40000 and r["future_income_counted"] == 0 for r in cases)
    if not selected["found"]:
        assert not any(r["funded"] for r in cases) and not rows["earned"] and not rows["service"]
        assert elapsed == 0 and chunks(before)[1] == chunks(output)[1]
        refused = True
        measured = dict(ticks=0,actual_fares=0,interest=0,other_operating_cost=0,cash_delta=0)
    else:
        assert cases[-1]["funded"] and not any(r["funded"] for r in cases[:-1])
        assert cases[-1]["ticks"] == selected["ticks"] and cases[-1]["target"] <= first["company"]["cash"]
        service, = rows["service"]
        earned, = rows["earned"]
        assert not service["stop_on_payment"] and not service["settlement"]
        assert 0 < elapsed == earned["earned"] == service["ticks"] <= selected["ticks"] <= 2000
        assert earned["reached"] == (last["company"]["cash"] >= 100000)
        assert earned["reached"] or elapsed == selected["ticks"]
        assert service["initial"] == initial["quantity"] and service["aboard"] == final["quantity"] and service["share"] == final["share"]
        assert initial["quantity"]+service["boarded"] == service["accepted"]+final["quantity"]
        assert not service["cohort_only_fare_claimed"] and not service["feeder_credit_is_cash"]
        pending = [r for r in TABLE(output,b"CAPY").values() if r["front"] == 50]
        assert service["payment_pending"] == bool(pending)
        a,b = [TABLE(p,b"VEHS")[49] for p in (before,output)]
        profit = b[TRAIN+"profit_this_year"]-a[TRAIN+"profit_this_year"]
        if a[TRAIN+"profit_last_year"] != b[TRAIN+"profit_last_year"]:
            profit += b[TRAIN+"profit_last_year"]
        assert service["profit_delta_256"] == profit
        expenses = {r["type"]:r["amount"] for r in rows["expenses"]}
        assert sum(expenses.values()) == first["company"]["cash"]-last["company"]["cash"]
        measured = dict(ticks=elapsed,selected_cap=selected["ticks"],actual_fares=-expenses[7]-expenses[8],interest=expenses[11],
            other_operating_cost=sum(v for k,v in expenses.items() if k not in (7,8,11)),
            cash_delta=last["company"]["cash"]-first["company"]["cash"],rail_initial=initial["quantity"],rail_boarded=service["boarded"],
            rail_accepted=service["accepted"],rail_aboard=final["quantity"],rail_share=final["share"],rail_pending_payment=pending,
            observed_whole_unload_fare=service["observed_whole_unload_fare"],cohort_only_fare_claimed=False,annual_projection=False)
        refused = False
    assert last["company"]["cash"] >= 40000
    growth = last["company"]["cash"]-burst_start
    complete_burst = window % 4 == 0
    # Actual goal/refusal takes precedence over a completed burst review.
    # This declaration closes the current scope; its next caller must reject
    # any attempted extension after non-growth or the explicit forty limit.
    reason = ("cash_goal" if last["company"]["cash"] >= 100000 else "funding_refused" if refused else
              "non_growing_burst" if complete_burst and growth <= 0 else "window_cap" if window == MAX_WINDOWS else None)
    operation.update(work_refused=refused,actual_rail_cargo=final,measured_cash_flow=measured,
        reserve_scope=dict(window=window,max_windows=MAX_WINDOWS,burst_start_cash=burst_start,burst_cash_growth=growth,
            burst_complete=complete_burst,ended=reason is not None,end_reason=reason))
    return operation


def main():
    """Publish one exact original finite reserve window or unchanged full-funding refusal."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"),required=True)
    parser.add_argument("--receipt",type=Path,required=True)
    parser.add_argument("--window",type=int,required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT/args.output
    assert output.resolve().is_relative_to(ROOT/"logs") and not output.exists()
    operation = execute(args.configuration,ROOT/args.receipt,args.window,args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__,complete=True,configuration=args.configuration,
        phase=PHASE,operation=operation),indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
