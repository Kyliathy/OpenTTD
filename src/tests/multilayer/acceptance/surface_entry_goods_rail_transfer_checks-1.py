"""Follow the actual Goods feeder only after its real capital-recovery receipt.

@file surface_entry_goods_rail_transfer_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZG.
Actual banked cash pays every complete interval. The earlier refusal remains
closed; no further sale, purchase or route change belongs to this consumer.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT,chunks,identity
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
SALE = runpy.run_path(str(DIRECTORY/"surface_entry_refill_capital_sale_checks-1.py"))
FEEDER = runpy.run_path(str(DIRECTORY/"surface_entry_goods_rail_feeder_work_checks-1.py"))
observe = SALE["observe"]
feeder_stock,ROAD = [FEEDER[k] for k in ("feeder_stock","ROAD")]
PHASE = FEEDER["PHASES"]["work"]


def execute(configuration,receipt,recover_run=None):
    """Earn one whole freshly funded interval or preserve a complete unchanged refusal.

    @param configuration Independently linked focused native application.
    @param receipt Actual native banked-sale receipt or this consumer's preceding work receipt.
    @param recover_run Successful invocation to inspect without repeating actual earned work.
    @return Exact sale anchor, original owners, physical transfer and measured cash facts.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] in (SALE["PHASE"],PHASE)
    anchor = receipt if previous["phase"] == SALE["PHASE"] else ROOT/previous["operation"]["banked_sale"]["path"]
    sale = json.loads(anchor.read_text(encoding="utf-8"))
    assert sale["complete"] and sale["phase"] == SALE["PHASE"] and sale["configuration"] == configuration
    assert sale["operation"]["actual_sale_complete"] and sale["operation"]["actual_refund_banked"] == 2120 and sale["operation"]["earned_ticks"] == 0
    # Later windows may consume only this consumer's exact real banked-sale
    # identity. A quote or an older unfunded feeder endpoint cannot supply
    # either the retirement owner contract or cash for another interval.
    if previous["phase"] == PHASE:
        assert identity(anchor) == previous["operation"]["banked_sale"]
        assert not previous["operation"]["work_refused"] and not previous["operation"]["measured_cash_flow"]["first_transfer_complete"]
    assert json.loads(json.dumps(observe(ROOT/previous["operation"]["output"]["path"]))) == previous["operation"]["final"]
    files = [ROOT/r["path"] for r in previous["operation"]["sources"]]+[Path(__file__),Path(SALE["__file__"]),Path(FEEDER["__file__"])]
    operation = native(configuration,PHASE,receipt,observe,
        dict(admission="GOODS_RAIL_FEEDER_WORK_ADMISSION",selection="GOODS_RAIL_FEEDER_WORK_SELECTION",
             service="GOODS_RAIL_FEEDER_SERVICE",earned="EARNED",expenses="EXPENSE"),
        files,recover_run=recover_run,chemical_limit=2000)
    operation["banked_sale"] = identity(anchor)
    first,last,rows = operation["initial"],operation["final"],operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL":32,"GOOD":50,"PROP":38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before,output = [ROOT/operation[k]["path"] for k in ("before","output")]
    initial,final = [feeder_stock(p) for p in (before,output)]
    assert initial["quantity"] == 0
    elapsed = TABLE(output,b"DATE")[0]["tick_counter"]-TABLE(before,b"DATE")[0]["tick_counter"]
    candidates = rows["admission"]
    selected, = rows["selection"]
    assert selected["ceiling"] == 2000
    assert all(r["ceiling"] == 2000 and r["ticks"] <= 2000 and r["unpaid"] == 0 and r["target"] == r["operating"]+40000 and r["future_income_counted"] == 0 for r in candidates)
    operation["actual_feeder_cargo"] = final
    if not selected["found"]:
        assert not any(r["funded"] for r in candidates) and not rows["earned"] and not rows["service"]
        assert elapsed == 0 and chunks(before)[1] == chunks(output)[1]
        operation["work_refused"] = True
        return operation
    assert candidates[-1]["funded"] and not any(r["funded"] for r in candidates[:-1])
    assert candidates[-1]["target"] <= first["company"]["cash"] and candidates[-1]["ticks"] == selected["ticks"]
    earned, = rows["earned"]
    service, = rows["service"]
    assert 0 < elapsed == earned["earned"] == service["ticks"] <= selected["ticks"] <= 2000
    assert service["cohort"] == final["quantity"] and service["feeder_share"] == final["share"]
    vehicle = TABLE(output,b"VEHS")[3]
    held = sum(TABLE(output,b"CAPA")[r-1]["count"] for r in vehicle[ROAD+"cargo.packets"])
    assert service["truck_held"] == held and not service["feeder_credit_is_cash"] and not service["rail_final_payment_claimed"]
    transferred = bool(final["quantity"] > 0 and held == 0 and vehicle[ROAD+"last_station_visited"] == 12)
    assert earned["reached"] == (transferred or last["company"]["cash"] >= 100000)
    assert earned["reached"] or elapsed == selected["ticks"]
    expenses = {r["type"]:r["amount"] for r in rows["expenses"]}
    assert sum(expenses.values()) == first["company"]["cash"]-last["company"]["cash"] and last["company"]["cash"] >= 40000
    operation["measured_cash_flow"] = dict(ticks=elapsed,selected_cap=selected["ticks"],actual_fares=-expenses[7]-expenses[8],
        interest=expenses[11],other_operating_cost=sum(v for k,v in expenses.items() if k not in (7,8,11)),
        cash_delta=last["company"]["cash"]-first["company"]["cash"],cohort=final["quantity"],feeder_share=final["share"],
        truck_held=held,first_transfer_complete=transferred,annual_projection=False)
    operation["work_refused"] = False
    return operation


def main():
    """Publish one immutable actual banked-capital feeder interval or unchanged refusal."""
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
