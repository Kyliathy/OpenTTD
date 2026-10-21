"""Earn only the longest complete funded native interval within an explicit ceiling.

@file surface_entry_factory_goods_liquidity_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGVC.
Current cash and the shared whole operating bill decide admission. Existing
loaded cargo and possible payments provide no spending credit.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT,chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_factory_goods_settlement_checks-1.py"))
observe = BASE["observe"]
PHASE = "staged-chemical-capital-factory-goods-liquid-work"


def rail_stock(path):
    """Read actual first22Industry7cargo on the original farm consist across native split/merge.

    @param path Genuine native original checkpoint.
    @return Actual cohort quantity, feeder share and sole per-wagon packet references.
    """
    packets = TABLE(path,b"CAPA"); owned = []
    allowed = {52,12,13,14,15,16,17,21,22,23,50,57,58,59}
    # Global material ownership is checked separately. These original native
    # vehicle references identify the transported cohort without manufacturing
    # packet IDs or treating cargo waiting at22as already aboard the train.
    for id,row in TABLE(path,b"VEHS").items():
        if row["type"] != 0:
            continue
        for ref in row["train[0].common[0].cargo.packets"]:
            packet = packets[ref-1]
            if packet["source"] != 22:
                continue
            assert id in allowed and packet["source_type"] == 0 and packet["source_id"] == 7
            owned.append(dict(vehicle=id,packet=ref-1,**packet))
    return dict(quantity=sum(p["count"] for p in owned),share=sum(p["feeder_share"] for p in owned),packets=owned)


def execute(configuration,receipt,ceiling=1000,recover_run=None):
    """Select and earn one whole affordable cap or retain an unchanged native refusal.

    @param configuration Independently linked focused native application.
    @param receipt Exact prior settlement or independently accepted liquidity-work receipt.
    @param ceiling Explicit group-owned maximum; current GVCscope supplies1000.
    @param recover_run Successful invocation to inspect without repeating native work.
    @return Complete admission/refusal, actual cargo/payment owners and measured company cash.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] in (BASE["PHASE"],PHASE)
    assert ceiling in (64,250,500,1000,2000,3000)
    files = [ROOT / r["path"] for r in previous["operation"]["sources"]] + [Path(__file__)]
    operation = native(configuration,PHASE,receipt,observe,
        dict(candidates="FACTORY_GOODS_LIQUID_CANDIDATE",selection="FACTORY_GOODS_LIQUID_SELECTION",admission="FARM_ROAD_WORK_ADMISSION",
             service="FACTORY_GOODS_SERVICE",vehicles="FACTORY_GOODS_SERVICE_VEHICLE",earned="EARNED",expenses="EXPENSE",delivered="DELIVERED"),
        files,recover_run=recover_run,chemical_limit=ceiling)
    first,last,rows = operation["initial"],operation["final"],operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL":32,"GOOD":50,"PROP":38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before,output = [ROOT / operation[k]["path"] for k in ("before","output")]
    initial_rail,final_rail = rail_stock(before),rail_stock(output)
    selected, = rows["selection"]
    candidates = rows["candidates"]
    assert selected["ceiling"] == ceiling and selected["cash"] == first["company"]["cash"]
    assert all(r["ticks"] <= ceiling and r["target"] == r["operating"]+40000 and r["future_income_counted"] == 0 for r in candidates)
    elapsed = TABLE(output,b"DATE")[0]["tick_counter"]-TABLE(before,b"DATE")[0]["tick_counter"]
    if not selected["found"]:
        assert not any(r["funded"] for r in candidates) and not rows["admission"] and not rows["earned"]
        assert elapsed == 0 and chunks(before)[1] == chunks(output)[1]
        operation["work_refused"] = True
        return operation
    assert candidates[-1]["funded"] and not any(r["funded"] for r in candidates[:-1])
    assert selected["ticks"] == candidates[-1]["ticks"] <= ceiling
    admission, = rows["admission"]
    assert admission["funded"] and admission["ticks"] == selected["ticks"] and admission["operating"] == selected["operating"]
    assert admission["target"] == admission["operating"]+40000 <= first["company"]["cash"]
    earned, = rows["earned"]
    service, = rows["service"]
    assert 0 < elapsed == earned["earned"] == service["ticks"] <= selected["ticks"]
    assert earned["reached"] or elapsed == selected["ticks"]
    assert initial_rail["quantity"]+service["rail_boarded"] == service["rail_delivered"]+final_rail["quantity"]
    assert service["rail_aboard"] == final_rail["quantity"]
    assert service["restored"] == (last["company"]["cash"] >= 100000) and last["company"]["cash"] >= 40000
    expenses = {r["type"]:r["amount"] for r in rows["expenses"]}
    assert sum(expenses.values()) == first["company"]["cash"]-last["company"]["cash"]
    pending = [r for r in TABLE(output,b"CAPY").values() if r["front"] == 53]
    a,b = [TABLE(p,b"VEHS")[52] for p in (before,output)]
    prefix = "train[0].common[0]."
    profit = b[prefix+"profit_this_year"]-a[prefix+"profit_this_year"]
    if a[prefix+"profit_last_year"] != b[prefix+"profit_last_year"]:
        profit += b[prefix+"profit_last_year"]
    operation["measured_cash_flow"] = dict(ticks=elapsed,selected_cap=selected["ticks"],ceiling=ceiling,
        actual_fares=-expenses[7]-expenses[8],interest=expenses[11],other_operating_cost=sum(v for k,v in expenses.items() if k not in (7,8,11)),
        cash_delta=last["company"]["cash"]-first["company"]["cash"],rail_initial=initial_rail["quantity"],rail_boarded=service["rail_boarded"],
        rail_delivered=service["rail_delivered"],rail_aboard=final_rail["quantity"],rail_pending_payment=pending,rail_profit_delta_256=profit,
        actual_rail_payment_posted=bool(service["rail_delivered"] > 0 and not pending and profit > 0),annual_projection=False)
    operation["work_refused"] = False
    return operation


def main():
    """Publish one explicitly capped actual funded interval or unchanged funding refusal."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"),required=True)
    parser.add_argument("--receipt",type=Path,required=True)
    parser.add_argument("--ceiling",type=int,choices=(64,250,500,1000,2000,3000),default=1000)
    parser.add_argument("--recover-run")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration,ROOT / args.receipt,args.ceiling,args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__,complete=True,configuration=args.configuration,phase=PHASE,operation=operation),indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
