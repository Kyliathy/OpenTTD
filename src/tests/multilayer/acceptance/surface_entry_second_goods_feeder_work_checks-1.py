"""Route the actual second Goods carrier and follow its first physical transfer.

@file surface_entry_second_goods_feeder_work_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZL.
Original public commands preserve loaded stock and pose. Each whole native
work interval is funded from actual money; virtual transfer share is not cash.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT,chunks,identity
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY/"surface_entry_second_goods_feeder_checks-1.py"))
observe,ROAD = BASE["observe"],BASE["ROAD"]
PHASES = {name:"staged-chemical-capital-second-goods-feeder-"+name for name in ("orders","work")}
MAX_WINDOWS = 12


def execute(configuration,stage,receipt,window=0,recover_run=None):
    """Run one original zero-price order boundary or separately funded first-transfer interval.

    @param configuration Independently linked focused native application.
    @param stage Explicit orders/work phase; successful commands and ticks are never replayed.
    @param receipt Exact bound proposal or the immediately preceding actual original boundary.
    @param window Zero for orders; strict consecutive1..12for actual work.
    @param recover_run Successful native invocation to inspect after a parser failure.
    @return Actual preserved owners, transferred25units, posted company books and finite stop.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration
    transferred_before = 0
    if stage == "orders":
        assert window == 0 and previous["phase"] == BASE["PHASE"] and previous["operation"]["second_feeder_admitted"]
    elif window == 1:
        assert previous["phase"] == PHASES["orders"] and previous["operation"]["actual_order_change_complete"]
    else:
        assert previous["phase"] == PHASES["work"] and 2 <= window <= MAX_WINDOWS
        scope = previous["operation"]["work_scope"]
        assert scope["window"] == window-1 and not scope["ended"]
        transferred_before = previous["operation"]["cumulative_25_transferred"]
    files = [ROOT/r["path"] for r in previous["operation"]["sources"]]+[Path(__file__)]
    operation = native(configuration,PHASES[stage],receipt,observe,
        dict(ordered="SECOND_GOODS_FEEDER_ORDERED",selection="SECOND_GOODS_FEEDER_WORK_SELECTION",
            admission="GOODS_RAIL_FEEDER_WORK_ADMISSION",service="SECOND_GOODS_FEEDER_SERVICE",
            invoices="INVOICE",earned="EARNED",expenses="EXPENSE"),files,recover_run=recover_run,chemical_limit=2000)
    first,last,rows = operation["initial"],operation["final"],operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL":32,"GOOD":50,"PROP":38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before,output = [ROOT/operation[k]["path"] for k in ("before","output")]
    a,b = [TABLE(p,b"VEHS") for p in (before,output)]
    elapsed = TABLE(output,b"DATE")[0]["tick_counter"]-TABLE(before,b"DATE")[0]["tick_counter"]
    if stage == "orders":
        assert elapsed == 0 and TABLE(before,b"DATE") == TABLE(output,b"DATE")
        changed = {k for k in chunks(before)[1] if chunks(before)[1][k] != chunks(output)[1][k]}
        assert changed <= {b"VEHS",b"ORDL"}
        assert a.keys() == b.keys() and all(a[k] == b[k] for k in a if k != 25)
        assert a[25][ROAD+"tile"] == b[25][ROAD+"tile"] and a[25][ROAD+"cargo.packets"] == b[25][ROAD+"cargo.packets"]
        lists = [TABLE(p,b"ORDL") for p in (before,output)]
        own = a[25][ROAD+"orders"]-1
        assert own == b[25][ROAD+"orders"]-1
        assert {k:v for k,v in lists[0].items() if k != own} == {k:v for k,v in lists[1].items() if k != own}
        assert lists[1][own]["orders[1].dest"] == 12 and lists[1][own]["orders[1].flags"] == 66
        assert first["company"]["cash"] == last["company"]["cash"] == 49250
        assert 3 <= len(rows["invoices"]) <= 4 and all(r["cost"] == 0 for r in rows["invoices"])
        ordered, = rows["ordered"]
        assert ordered["vehicle"] == 25 and ordered["held"] == 18 and ordered["receiver"] == 12 and ordered["invoice"] == ordered["elapsed"] == 0
        operation.update(actual_order_change_complete=True,route_admission=identity(receipt))
        return operation
    assert 1 <= window <= MAX_WINDOWS and not rows["invoices"] and elapsed >= 0
    anchor = receipt if window == 1 else ROOT/previous["operation"]["ordered_receipt"]["path"]
    if window > 1:
        assert identity(anchor) == previous["operation"]["ordered_receipt"]
    operation["ordered_receipt"] = identity(anchor)
    selected, = rows["selection"]
    cases = rows["admission"]
    assert selected["ceiling"] == 2000
    assert all(r["target"] == r["operating"]+40000 and r["unpaid"] == r["future_income_counted"] == 0 for r in cases)
    refused = not selected["found"]
    if refused:
        assert not any(r["funded"] for r in cases) and not rows["service"] and not rows["earned"]
        assert elapsed == 0 and chunks(before)[1] == chunks(output)[1]
        transferred,complete = 0,False
        measured = dict(ticks=0,actual_fares=0,interest=0,other_operating_cost=0,cash_delta=0)
    else:
        assert cases[-1]["funded"] and not any(r["funded"] for r in cases[:-1])
        assert cases[-1]["ticks"] == selected["ticks"] and cases[-1]["target"] <= first["company"]["cash"]
        service, = rows["service"]
        earned, = rows["earned"]
        assert 0 < elapsed == service["ticks"] == earned["earned"] <= selected["ticks"] <= 2000
        transferred = service["transferred"]
        assert service["initial"] == selected["initial"] and service["initial"] == service["held"]+transferred
        assert not service["feeder_credit_is_cash"] and service["cash"] == last["company"]["cash"]
        complete = service["first_transfer_complete"]
        assert complete == (service["held"] == 0 and service["last_station"] == 12)
        assert earned["reached"] == (complete or last["company"]["cash"] >= 100000)
        assert earned["reached"] or elapsed == selected["ticks"]
        expenses = {r["type"]:r["amount"] for r in rows["expenses"]}
        assert sum(expenses.values()) == first["company"]["cash"]-last["company"]["cash"]
        measured = dict(ticks=elapsed,selected_cap=selected["ticks"],actual_fares=-expenses[7]-expenses[8],interest=expenses[11],
            other_operating_cost=sum(v for k,v in expenses.items() if k not in (7,8,11)),cash_delta=last["company"]["cash"]-first["company"]["cash"])
    cumulative = transferred_before+transferred
    assert 0 <= cumulative <= 18 and (not complete or cumulative == 18) and last["company"]["cash"] >= 40000
    # Completion is this existing25load's physical transfer, independent of
    # company fares from other useful services. A refusal or hard12cap ends
    # the scope; neither waiting Goods nor future rail revenue extends it.
    reason = ("first_transfer" if complete else "cash_goal" if last["company"]["cash"] >= 100000 else
              "funding_refused" if refused else "window_cap" if window == MAX_WINDOWS else None)
    operation.update(actual_order_change_complete=True,first_25_transfer_complete=complete,cumulative_25_transferred=cumulative,
        work_refused=refused,measured_cash_flow=measured,work_scope=dict(window=window,max_windows=MAX_WINDOWS,ended=reason is not None,end_reason=reason))
    return operation


def main():
    """Publish one exact second-feeder order or bounded actual first-transfer interval."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"),required=True)
    parser.add_argument("--stage",choices=tuple(PHASES),required=True)
    parser.add_argument("--receipt",type=Path,required=True)
    parser.add_argument("--window",type=int,default=0)
    parser.add_argument("--recover-run")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT/args.output
    assert output.resolve().is_relative_to(ROOT/"logs") and not output.exists()
    operation = execute(args.configuration,args.stage,ROOT/args.receipt,args.window,args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__,complete=True,configuration=args.configuration,
        phase=PHASES[args.stage],operation=operation),indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
