"""Observe actual native settlement of the existing Goods payment within a funded64tick cap.

@file surface_entry_factory_goods_settlement_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGVB.
CAPYowns572before work. Only its native lifecycle may post company cash and
vehicle profit; pending money provides no credit for operating admission.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_factory_goods_work_checks-1.py"))
observe = BASE["observe"]
PHASE = "staged-chemical-capital-factory-goods-settlement"
ROAD = "roadveh[0].common[0]."


def execute(configuration, receipt, recover_run=None):
    """Earn at most64ordinary ticks until real native payment settlement or honest late partial.

    @param configuration Independently linked focused native application.
    @param receipt Exact preceding actual Goods-acceptance receipt with CAPYfront4owning572.
    @param recover_run Successful invocation to inspect without repeating native work.
    @return Sole payment owner, actual posted fare/profit/cash and preserved original owners.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] == BASE["PHASES"]["work"]
    assert receipt.name == f"surface-entry-factory-goods-follow-01-{configuration}.json"
    source = ROOT / previous["operation"]["output"]["path"]
    payment, = [row for row in TABLE(source,b"CAPY").values() if row["front"] == 4]
    assert payment["route_profit"] == payment["visual_profit"] == 572 and payment["visual_transfer"] == 0
    files = [ROOT / row["path"] for row in previous["operation"]["sources"]] + [Path(__file__),
        ROOT / "src/tests/surface_entry_factory_goods_settlement.h", ROOT / "src/economy_base.h"]
    operation = native(configuration,PHASE,receipt,observe,
        dict(admission="FACTORY_GOODS_SETTLEMENT_ADMISSION",settlement="FACTORY_GOODS_SETTLED",earned="EARNED",expenses="EXPENSE"),
        files,recover_run=recover_run,chemical_limit=64)
    first,last,rows = operation["initial"],operation["final"],operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL":32,"GOOD":50,"PROP":38}
    assert first["company"]["cash"] == 49932 and first["company"]["debt"] == last["company"]["debt"] == 5000000
    admission, = rows["admission"]
    assert admission["funded"] and admission["ticks"] == 64 and admission["pending"] == 572
    assert admission["target"] == admission["operating"]+40000 <= 49932
    assert admission["pending_credit"] == admission["future_income_counted"] == 0
    before,output = [ROOT / operation[key]["path"] for key in ("before","output")]
    elapsed = TABLE(output,b"DATE")[0]["tick_counter"]-TABLE(before,b"DATE")[0]["tick_counter"]
    settlement, = rows["settlement"]
    earned, = rows["earned"]
    assert 0 < elapsed == settlement["ticks"] == earned["earned"] <= 64
    assert earned["reached"] == settlement["settled"] and (earned["reached"] or elapsed == 64)
    assert settlement["pending"] == 572 and settlement["vehicle"] == 3 and not settlement["direct_payment_granted"]
    remaining = [row for row in TABLE(output,b"CAPY").values() if row["front"] == 4]
    assert bool(remaining) == (not settlement["settled"])
    a,b = [TABLE(path,b"VEHS")[3] for path in (before,output)]
    profit = b[ROAD+"profit_this_year"]-a[ROAD+"profit_this_year"]
    if a[ROAD+"profit_last_year"] != b[ROAD+"profit_last_year"]:
        profit += b[ROAD+"profit_last_year"]
    assert settlement["profit_delta_256"] == profit
    expenses = {row["type"]:row["amount"] for row in rows["expenses"]}
    assert sum(expenses.values()) == first["company"]["cash"]-last["company"]["cash"]
    if settlement["settled"]:
        assert -expenses[8] >= 572 and profit > 0
    operation["measured_cash_flow"] = dict(ticks=elapsed,actual_fares=-expenses[7]-expenses[8],interest=expenses[11],
        other_operating_cost=sum(v for k,v in expenses.items() if k not in (7,8,11)),cash_delta=last["company"]["cash"]-first["company"]["cash"],
        actual_first_direct_payment=settlement["settled"],vehicle3_profit_delta_256=profit,payment_granted=False,annual_projection=False)
    return operation


def main():
    """Publish one immutable actual payment-settlement boundary without a manual payment or replay."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"),required=True)
    parser.add_argument("--receipt",type=Path,required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration,ROOT / args.receipt,args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__,complete=True,configuration=args.configuration,phase=PHASE,operation=operation),indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
