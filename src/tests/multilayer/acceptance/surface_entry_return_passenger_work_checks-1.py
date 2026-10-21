"""Apply the admitted extra-bus return policy and observe the first actual Town1 pickup.

@file surface_entry_return_passenger_work_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15CSGY/15CSGZ.
Only the explicit later contract admits Town1 cargo on extra9. Original local
and regional services, conserved launch materials and native books stay owned.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_regional_ledger import TABLE
from surface_gateway_component_delivery_ledger import observe_propellant_delivery
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_return_passenger_checks-1.py"))
PHASES = {s: "staged-chemical-capital-return-passenger-" + s for s in ("orders", "work")}
ROAD = "roadveh[0].common[0]."


def observe(path):
    """Read actual owners with the narrowly admitted extra9 Town1 return contract.

    @param path Genuine native original checkpoint.
    @return Original conserved materials, services and sole physical packet owners.
    """
    result = observe_propellant_delivery(path, allow_capital_service=True, allow_goods_income=True,
        allow_farm_capacity=True, allow_farm_balance=True, allow_farm_road=True,
        allow_factory_goods=True, allow_capital_return=True)
    assert result["components_at_center"] and result["totals"] == {"STEL":32,"GOOD":50,"PROP":38}
    factory = TABLE(path,b"INDY")[5]
    assert factory["type"] == 6 and factory["location.tile"] == 3344 and factory["produced[0].cargo"] == 5
    return result


def return_stock(path):
    """Read the real Town1 cohort aboard the actual paid extra bus.

    @param path Genuine original native checkpoint, never edited packet data.
    @return Sole packet references and conserved quantity, without forecast fare credit.
    """
    vehicle,packets = TABLE(path,b"VEHS")[9],TABLE(path,b"CAPA")
    assert vehicle[ROAD+"owner"] == 0 and vehicle[ROAD+"engine_type"] == 119 and vehicle[ROAD+"cargo_cap"] == 40
    cargo = [dict(packet=r-1,**packets[r-1]) for r in vehicle[ROAD+"cargo.packets"]]
    assert all(p["source_type"] == 1 and p["source_id"] in (0,1) for p in cargo)
    returned = [p for p in cargo if p["source_id"] == 1]
    assert all(p["source"] == 20 for p in returned)
    return dict(quantity=sum(p["count"] for p in returned),packets=returned)


def execute(configuration,stage,receipt,recover_run=None,follow=False):
    """Run exactly one zero-price order boundary or first funded pickup interval.

    @param configuration Independently linked focused native application.
    @param stage Explicit orders/work boundary, never a replay or automatic extension.
    @param receipt Exact preceding original pure query or order receipt.
    @param recover_run Successful native invocation to inspect without repeating it.
    @param follow Explicit later first-pickup continuation; false preserves the historical order receipt requirement.
    @return Actual owner conservation, order policy and measured cargo/payment/cash facts.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert not follow or stage == "work"
    expected = BASE["PHASE"] if stage == "orders" else (PHASES["work"] if follow else PHASES["orders"])
    assert previous["complete"] and previous["phase"] == expected
    if stage == "orders":
        assert previous["operation"]["return_passenger_admitted"]
    files = [ROOT/r["path"] for r in previous["operation"]["sources"]] + [Path(__file__),
        ROOT/"src/tests/surface_entry_intertown.h",ROOT/"src/tests/surface_entry_factory_goods_work.h",
        ROOT/"src/tests/surface_entry_return_passenger.h"]
    operation = native(configuration,PHASES[stage],receipt,observe,
        dict(ordered="RETURN_PASSENGER_ORDERED",invoices="INVOICE",candidates="FACTORY_GOODS_LIQUID_CANDIDATE",
             selection="FACTORY_GOODS_LIQUID_SELECTION",admission="FARM_ROAD_WORK_ADMISSION",returned="RETURN_PASSENGER_SERVICE",
             service="FACTORY_GOODS_SERVICE",earned="EARNED",expenses="EXPENSE"),
        files,recover_run=recover_run,chemical_limit=2000)
    first,last,rows = operation["initial"],operation["final"],operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL":32,"GOOD":50,"PROP":38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before,output = [ROOT/operation[k]["path"] for k in ("before","output")]
    a,b = [TABLE(p,b"VEHS") for p in (before,output)]
    elapsed = TABLE(output,b"DATE")[0]["tick_counter"]-TABLE(before,b"DATE")[0]["tick_counter"]
    order = TABLE(output,b"ORDL")[b[9][ROAD+"orders"]-1]
    assert order["orders.count"] == 2 and [order[f"orders[{i}].dest"] for i in (0,1)] == [17,20]
    assert [order[f"orders[{i}].flags"] for i in (0,1)] == [0,0]
    initial,final = return_stock(before),return_stock(output)
    if stage == "orders":
        assert elapsed == 0 and TABLE(before,b"DATE") == TABLE(output,b"DATE")
        old,new = [chunks(p)[1] for p in (before,output)]
        assert all(old[k] == new[k] for k in old if k not in (b"ORDL",b"VEHS"))
        assert a.keys() == b.keys() and all(a[k] == b[k] for k in a if k != 9)
        assert a[9][ROAD+"tile"] == b[9][ROAD+"tile"] and a[9][ROAD+"cargo.packets"] == b[9][ROAD+"cargo.packets"] == []
        lists = [TABLE(p,b"ORDL") for p in (before,output)]
        changed = a[9][ROAD+"orders"]-1
        assert changed == b[9][ROAD+"orders"]-1
        assert {k:v for k,v in lists[0].items() if k != changed} == {k:v for k,v in lists[1].items() if k != changed}
        assert len(rows["invoices"]) == 2 and all(r["cost"] == 0 for r in rows["invoices"])
        ordered, = rows["ordered"]
        assert ordered["invoice"] == ordered["elapsed"] == ordered["original_services_changed"] == 0
        assert ordered["target"] == ordered["operating"]+40000 <= first["company"]["cash"]
        assert first["company"]["cash"] == last["company"]["cash"] == 54619
    else:
        selected, = rows["selection"]
        assert selected["found"] and selected["ceiling"] == 2000
        admission, = rows["admission"]
        assert admission["funded"] and admission["ticks"] == selected["ticks"] <= 2000
        assert admission["target"] == admission["operating"]+40000 <= first["company"]["cash"]
        returned, = rows["returned"]
        earned, = rows["earned"]
        assert 0 < elapsed == returned["ticks"] == earned["earned"] <= selected["ticks"]
        assert returned["first_pickup_boundary"] and returned["initial"] == initial["quantity"] == 0
        assert returned["initial"]+returned["picked_up"] == returned["accepted"]+returned["aboard"]
        assert returned["aboard"] == final["quantity"] and last["company"]["cash"] >= 40000
        # A first-pickup boundary must retain physical Source1cargo aboard.
        # This same real checkpoint must still fail the historical Source0
        # observer, proving that the default contract was never widened.
        if final["quantity"] > 0:
            try:
                BASE["observe"](output)
            except AssertionError:
                operation["historical_source0_observer_rejected"] = True
            else:
                raise AssertionError("Historical extra-bus source default was widened")
        assert earned["reached"] == (returned["picked_up"] > 0 or last["company"]["cash"] >= 100000)
        expenses = {r["type"]:r["amount"] for r in rows["expenses"]}
        assert sum(expenses.values()) == first["company"]["cash"]-last["company"]["cash"]
        pending = [r for r in TABLE(output,b"CAPY").values() if r["front"] == 10]
        assert returned["payment_pending"] == bool(pending)
        profit = b[9][ROAD+"profit_this_year"]-a[9][ROAD+"profit_this_year"]
        if a[9][ROAD+"profit_last_year"] != b[9][ROAD+"profit_last_year"]:
            profit += b[9][ROAD+"profit_last_year"]
        assert returned["profit_delta_256"] == profit
        operation["measured_cash_flow"] = dict(ticks=elapsed,actual_fares=-expenses[7]-expenses[8],interest=expenses[11],
            other_operating_cost=sum(v for k,v in expenses.items() if k not in (7,8,11)),
            cash_delta=last["company"]["cash"]-first["company"]["cash"],return_initial=initial["quantity"],
            return_picked_up=returned["picked_up"],return_accepted=returned["accepted"],return_aboard=final["quantity"],
            return_profit_delta_256=profit,return_pending_payment=pending,annual_projection=False)
    operation["actual_return_cargo"] = final
    return operation


def main():
    """Publish one immutable actual order or first-pickup receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"),required=True)
    parser.add_argument("--stage",choices=tuple(PHASES),required=True)
    parser.add_argument("--receipt",type=Path,required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--follow",action="store_true")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT/args.output
    assert output.resolve().is_relative_to(ROOT/"logs") and not output.exists()
    operation = execute(args.configuration,args.stage,ROOT/args.receipt,args.recover_run,args.follow)
    output.write_text(json.dumps(dict(_documentation=__doc__,complete=True,configuration=args.configuration,
        phase=PHASES[args.stage],operation=operation),indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
