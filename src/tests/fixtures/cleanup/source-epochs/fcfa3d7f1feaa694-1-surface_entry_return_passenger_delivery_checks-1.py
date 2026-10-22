"""Follow actual return cargo through accepting Town0 delivery and native payment.

@file surface_entry_return_passenger_delivery_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZA.
The original paid bus, native packet owners and CargoPayment lifecycle alone
deliver and post fares. Each interval requires its whole current operating bill.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT,chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY/"surface_entry_return_passenger_work_checks-1.py"))
observe,return_stock,ROAD = [BASE[k] for k in ("observe","return_stock","ROAD")]
PHASE = "staged-chemical-capital-return-passenger-delivery"


def execute(configuration,receipt,recover_run=None):
    """Earn one whole affordable return-delivery interval or preserve an unchanged refusal.

    @param configuration Independently linked focused native application.
    @param receipt Exact first-pickup or preceding actual delivery-window receipt.
    @param recover_run Successful invocation to inspect without replaying earned work.
    @return Conserved return cargo, native posted/pending payment and measured cash.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] in (BASE["PHASES"]["work"],PHASE)
    assert not previous["operation"].get("return_payment_posted",False)
    files = [ROOT/r["path"] for r in previous["operation"]["sources"]]+[Path(__file__),
        ROOT/"src/economy.cpp",ROOT/"src/economy_base.h",ROOT/"src/tests/surface_entry_factory_goods_work.h"]
    operation = native(configuration,PHASE,receipt,observe,
        dict(candidates="FACTORY_GOODS_LIQUID_CANDIDATE",selection="FACTORY_GOODS_LIQUID_SELECTION",
             admission="FARM_ROAD_WORK_ADMISSION",returned="RETURN_PASSENGER_SERVICE",
             earned="EARNED",expenses="EXPENSE"),files,recover_run=recover_run,chemical_limit=2000)
    first,last,rows = operation["initial"],operation["final"],operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL":32,"GOOD":50,"PROP":38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before,output = [ROOT/operation[k]["path"] for k in ("before","output")]
    initial,final = [return_stock(p) for p in (before,output)]
    elapsed = TABLE(output,b"DATE")[0]["tick_counter"]-TABLE(before,b"DATE")[0]["tick_counter"]
    selected, = rows["selection"]
    candidates = rows["candidates"]
    assert selected["ceiling"] == 2000 and selected["cash"] == first["company"]["cash"]
    assert all(r["ticks"] <= 2000 and r["target"] == r["operating"]+40000 and r["future_income_counted"] == 0 for r in candidates)
    operation["actual_return_cargo"] = final
    if not selected["found"]:
        assert not any(r["funded"] for r in candidates) and not rows["earned"] and not rows["returned"]
        assert elapsed == 0 and chunks(before)[1] == chunks(output)[1]
        operation.update(work_refused=True,return_payment_posted=False)
        return operation
    assert candidates[-1]["funded"] and not any(r["funded"] for r in candidates[:-1])
    admission, = rows["admission"]
    assert admission["funded"] and admission["ticks"] == selected["ticks"] <= 2000
    assert admission["target"] == admission["operating"]+40000 <= first["company"]["cash"]
    returned, = rows["returned"]
    earned, = rows["earned"]
    assert 0 < elapsed == earned["earned"] == returned["ticks"] <= selected["ticks"]
    assert returned["initial"] == initial["quantity"] and returned["aboard"] == final["quantity"]
    assert returned["initial"]+returned["picked_up"] == returned["accepted"]+returned["aboard"]
    assert returned["payment_boundary"] and not returned["first_pickup_boundary"]
    assert earned["reached"] == (returned["return_payment_posted"] or last["company"]["cash"] >= 100000)
    assert earned["reached"] or elapsed == selected["ticks"]
    a,b = [TABLE(p,b"VEHS")[9] for p in (before,output)]
    profit = b[ROAD+"profit_this_year"]-a[ROAD+"profit_this_year"]
    if a[ROAD+"profit_last_year"] != b[ROAD+"profit_last_year"]:
        profit += b[ROAD+"profit_last_year"]
    assert profit == returned["profit_delta_256"]
    pending = [r for r in TABLE(output,b"CAPY").values() if r["front"] == 10]
    old_pending = [r for r in TABLE(before,b"CAPY").values() if r["front"] == 10]
    actual_initial_payment = bool(a[ROAD+"last_station_visited"] == 17 and old_pending and old_pending[0]["route_profit"] > 0)
    assert returned["initial_return_payment"] == actual_initial_payment
    assert returned["payment_pending"] == bool(pending)
    # First17acceptance and the real CargoPayment owner's completion must
    # agree before any return-payment claim; empty cargo alone is insufficient.
    if returned["return_payment_posted"]:
        assert returned["accepted"] > 0 or actual_initial_payment
        assert b[ROAD+"last_station_visited"] == 17 and final["quantity"] == 0 and not pending and profit > 0
    expenses = {r["type"]:r["amount"] for r in rows["expenses"]}
    assert sum(expenses.values()) == first["company"]["cash"]-last["company"]["cash"]
    assert last["company"]["cash"] >= 40000
    operation["measured_cash_flow"] = dict(ticks=elapsed,selected_cap=selected["ticks"],actual_fares=-expenses[7]-expenses[8],
        interest=expenses[11],other_operating_cost=sum(v for k,v in expenses.items() if k not in (7,8,11)),
        cash_delta=last["company"]["cash"]-first["company"]["cash"],return_initial=initial["quantity"],
        return_picked_up=returned["picked_up"],return_accepted=returned["accepted"],return_aboard=final["quantity"],
        return_profit_delta_256=profit,return_pending_payment=pending,annual_projection=False)
    operation.update(work_refused=False,return_payment_posted=returned["return_payment_posted"])
    return operation


def main():
    """Publish one exact original return-delivery/payment interval or unchanged refusal."""
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
