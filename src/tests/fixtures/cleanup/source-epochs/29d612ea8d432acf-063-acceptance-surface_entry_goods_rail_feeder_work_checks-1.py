"""Pay the admitted short Goods feeder, pass the real truck gate and earn first transfer work.

@file surface_entry_goods_rail_feeder_work_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZC.
Original native commands own the new joined12bay and3-only transfer policy.
Actual first13cargo remains separate from native first12production and cash.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT,chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY/"surface_entry_goods_rail_feeder_checks-1.py"))
observe = BASE["observe"]
PHASES = {s:"staged-chemical-capital-goods-rail-feeder-"+s for s in ("infrastructure","orders","work")}
ROAD = "roadveh[0].common[0]."


def feeder_stock(path):
    """Read actual transferred first13Goods on the original12station or49consist.

    @param path Complete actual native checkpoint.
    @return Sole-owned cohort quantity, virtual share and references, never posted cash.
    """
    packets = TABLE(path,b"CAPA"); station = TABLE(path,b"STNN")[12]
    claims = [("station",12,ref) for k,refs in station.items() if k.startswith("normal[0].goods[5].cargo[") and k.endswith(".second") for ref in refs]
    vehicles = TABLE(path,b"VEHS")
    # The original whole49consist may load a newly transferred packet in the
    # same native tick. Its actual owners distinguish that from natural
    # first12Factoryproduction or a virtual duplicate shipment receipt.
    for id in [49]+list(range(26,38)):
        claims += [("vehicle",id,r) for r in vehicles[id]["train[0].common[0].cargo.packets"]]
    cohort = [dict(owner=(kind,id),packet=ref-1,**packets[ref-1]) for kind,id,ref in claims if packets[ref-1]["source"] == 13]
    assert len({p["packet"] for p in cohort}) == len(cohort)
    assert all(p["source_type"] == 0 and p["source_id"] in (5,11) for p in cohort)
    return dict(quantity=sum(p["count"] for p in cohort),share=sum(p["feeder_share"] for p in cohort),packets=cohort)


def execute(configuration,stage,receipt,recover_run=None):
    """Run only one admitted paid infrastructure, zero-order or first operating boundary.

    @param configuration Independently linked focused native application.
    @param stage Explicit infrastructure/orders/work boundary, never an automatic replay.
    @param receipt Exact immediately preceding original native proof.
    @param recover_run Successful invocation to inspect without repeating payment or ticks.
    @return Original owners, exact invoices and literal physical transfer or partial result.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    expected = dict(infrastructure=BASE["PHASE"],orders=PHASES["infrastructure"],work=PHASES["orders"])[stage]
    assert previous["complete"] and previous["phase"] == expected
    if stage == "infrastructure":
        assert previous["operation"]["goods_rail_feeder_admitted"]
    files = [ROOT/r["path"] for r in previous["operation"]["sources"]]+[Path(__file__),ROOT/"src/tests/surface_entry_factory_goods_work.h"]
    operation = native(configuration,PHASES[stage],receipt,observe,
        dict(infrastructure="GOODS_RAIL_FEEDER_INFRASTRUCTURE",ordered="GOODS_RAIL_FEEDER_ORDERED",
             admission="GOODS_RAIL_FEEDER_WORK_ADMISSION",selection="GOODS_RAIL_FEEDER_WORK_SELECTION",
             service="GOODS_RAIL_FEEDER_SERVICE",invoices="INVOICE",earned="EARNED",expenses="EXPENSE"),
        files,recover_run=recover_run,chemical_limit=500)
    first,last,rows = operation["initial"],operation["final"],operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL":32,"GOOD":50,"PROP":38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before,output = [ROOT/operation[k]["path"] for k in ("before","output")]
    a,b = [TABLE(p,b"VEHS") for p in (before,output)]
    elapsed = TABLE(output,b"DATE")[0]["tick_counter"]-TABLE(before,b"DATE")[0]["tick_counter"]
    admission = rows["admission"]
    assert all(r["ceiling"] == 500 and r["ticks"] <= 500 and r["target"] == r["unpaid"]+r["operating"]+40000 and r["future_income_counted"] == 0 for r in admission)
    assert admission and admission[-1]["funded"] and admission[-1]["target"] <= first["company"]["cash"]
    if stage != "work":
        assert elapsed == 0 and TABLE(before,b"DATE") == TABLE(output,b"DATE")
        assert chunks(before)[1][b"CAPA"] == chunks(output)[1][b"CAPA"]
        assert len(rows["invoices"]) == 2
        if stage == "infrastructure":
            assert a == b and TABLE(before,b"ORDL") == TABLE(output,b"ORDL")
            assert [r["cost"] for r in rows["invoices"]] == [71,307]
            assert first["company"]["cash"]-last["company"]["cash"] == 378 and last["company"]["cash"] == 47455
            paid, = rows["infrastructure"]
            assert paid["invoice"] == 378 and paid["station"] == 12 and paid["bay"] == 2575 and not paid["new_station"]
        else:
            assert a.keys() == b.keys() and all(a[k] == b[k] for k in a if k != 3)
            assert a[3][ROAD+"tile"] == b[3][ROAD+"tile"] and a[3][ROAD+"cargo.packets"] == b[3][ROAD+"cargo.packets"] == []
            lists = [TABLE(p,b"ORDL") for p in (before,output)]
            changed = a[3][ROAD+"orders"]-1
            assert changed == b[3][ROAD+"orders"]-1
            assert {k:v for k,v in lists[0].items() if k != changed} == {k:v for k,v in lists[1].items() if k != changed}
            assert all(r["cost"] == 0 for r in rows["invoices"])
            assert first["company"]["cash"] == last["company"]["cash"] == 47455
            ordered, = rows["ordered"]
            assert ordered["actual_facility"] and ordered["transfer"] == 12 and ordered["original25_receiver"] == 20
    else:
        selected, = rows["selection"]
        service, = rows["service"]
        earned, = rows["earned"]
        assert selected["found"] and 0 < elapsed == service["ticks"] == earned["earned"] <= selected["ticks"] <= 500
        stock = feeder_stock(output)
        assert feeder_stock(before)["quantity"] == 0
        assert service["cohort"] == stock["quantity"] and service["feeder_share"] == stock["share"]
        held = sum(TABLE(output,b"CAPA")[r-1]["count"] for r in b[3][ROAD+"cargo.packets"])
        assert service["truck_held"] == held and not service["feeder_credit_is_cash"] and not service["rail_final_payment_claimed"]
        reached = bool(stock["quantity"] > 0 and held == 0 and b[3][ROAD+"last_station_visited"] == 12 or last["company"]["cash"] >= 100000)
        assert earned["reached"] == reached and (earned["reached"] or elapsed == selected["ticks"])
        expenses = {r["type"]:r["amount"] for r in rows["expenses"]}
        assert sum(expenses.values()) == first["company"]["cash"]-last["company"]["cash"]
        assert last["company"]["cash"] >= 40000
        operation["measured_cash_flow"] = dict(ticks=elapsed,actual_fares=-expenses[7]-expenses[8],interest=expenses[11],
            other_operating_cost=sum(v for k,v in expenses.items() if k not in (7,8,11)),
            cash_delta=last["company"]["cash"]-first["company"]["cash"],cohort=stock["quantity"],feeder_share=stock["share"],
            truck_held=held,first_transfer_complete=reached,annual_projection=False)
        operation["actual_feeder_cargo"] = stock
    if stage != "infrastructure":
        # Only3gains transfer12; native non-stop/FarEnd persistence and25's
        # original direct20policy remain explicit after both orders and work.
        for id,receiver,flags in ((3,12,66),(25,20,64)):
            order = TABLE(output,b"ORDL")[b[id][ROAD+"orders"]-1]
            assert order["orders.count"] == 2 and [order[f"orders[{i}].dest"] for i in (0,1)] == [13,receiver]
            assert [order[f"orders[{i}].flags"] for i in (0,1)] == [4,flags]
            assert all(order[f"orders[{i}].type"] == 97 for i in (0,1))
    return operation


def main():
    """Publish one exact original paid/order/first-work checkpoint."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"),required=True)
    parser.add_argument("--stage",choices=tuple(PHASES),required=True)
    parser.add_argument("--receipt",type=Path,required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT/args.output
    assert output.resolve().is_relative_to(ROOT/"logs") and not output.exists()
    operation = execute(args.configuration,args.stage,ROOT/args.receipt,args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__,complete=True,configuration=args.configuration,
        phase=PHASES[args.stage],operation=operation),indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
