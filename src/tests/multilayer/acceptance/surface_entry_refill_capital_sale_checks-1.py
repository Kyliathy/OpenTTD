"""Retire only the admitted empty duplicate and verify its actual banked native refund.

@file surface_entry_refill_capital_sale_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZF.
Explicit later ownership mode admits only absent vehicle39after the legal
native sale. Current cash alone then funds the whole quoted operating bill.
"""
import argparse
import copy
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT,chunks
from surface_regional_ledger import TABLE
from surface_gateway_component_delivery_ledger import observe_propellant_delivery
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY/"surface_entry_refill_capital_checks-1.py"))
PHASE = "staged-chemical-capital-refill-capital-sell"


def observe(path):
    """Observe actual post-delivery owners with only the explicitly retired empty duplicate omitted.

    @param path Genuine original native checkpoint, never edited vehicle or cargo data.
    @return Conserved original materials, retained supply capacity and useful service owners.
    """
    result = observe_propellant_delivery(path,allow_capital_service=True,allow_goods_income=True,
        allow_farm_capacity=True,allow_farm_balance=True,allow_farm_road=True,
        allow_factory_goods=True,allow_capital_return=True,allow_retired_refill=True)
    assert result["components_at_center"] and result["totals"] == {"STEL":32,"GOOD":50,"PROP":38}
    factory = TABLE(path,b"INDY")[5]
    assert factory["type"] == 6 and factory["location.tile"] == 3344 and factory["produced[0].cargo"] == 5
    return result


def execute(configuration,receipt,recover_run=None):
    """Execute exactly one admitted empty-body retirement with no earned work.

    @param configuration Independently linked focused native application.
    @param receipt Exact original bound legal sale quote with refund still unbanked.
    @param recover_run Successful native sale invocation to inspect without repeating it.
    @return Actual banked refund, preserved owners and freshly funded whole operating quote.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] == BASE["PHASE"]
    assert previous["operation"]["refill_retirement_admitted"] and not previous["operation"]["actual_sale_complete"]
    source = ROOT/previous["operation"]["output"]["path"]
    # Verify the explicit mode's identical pre-sale observation before the
    # paid command, so a forwarding or default-contract error cannot require
    # repeating any actual retirement while inspecting its saved result.
    assert json.loads(json.dumps(observe(source))) == previous["operation"]["final"]
    files = [ROOT/r["path"] for r in previous["operation"]["sources"]]+[Path(__file__)]
    operation = native(configuration,PHASE,receipt,observe,
        dict(sold="REFILL_CAPITAL_SOLD",admission="GOODS_RAIL_FEEDER_WORK_ADMISSION",invoices="INVOICE",earned="EARNED"),
        files,recover_run=recover_run,chemical_limit=500)
    first,last,rows = operation["initial"],operation["final"],operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    before,output = [ROOT/operation[k]["path"] for k in ("before","output")]
    assert TABLE(before,b"DATE") == TABLE(output,b"DATE") and not rows["earned"]
    assert chunks(before)[1][b"CAPA"] == chunks(output)[1][b"CAPA"]
    a,b = [TABLE(p,b"VEHS") for p in (before,output)]
    assert a.keys()-b.keys() == {39} and not b.keys()-a.keys()
    assert all(a[k] == b[k] for k in b)
    # Native retirement removes the sold body's sole order list together
    # with that owner. Every other order list and vehicle row must remain
    # identical; retaining this orphan would contradict the real lifecycle.
    own = a[39]["roadveh[0].common[0].orders"]
    assert own > 0
    references = [(id,k) for id,row in a.items() for k,value in row.items() if k.endswith(".common[0].orders") and value == own]
    assert references == [(39,"roadveh[0].common[0].orders")]
    assert TABLE(output,b"ORDL") == {k:v for k,v in TABLE(before,b"ORDL").items() if k != own-1}
    expected = copy.deepcopy(first)
    expected["company"]["cash"] += 2120
    expected["shipments"].pop("Surface Propellant refill")
    assert last == expected and last["company"]["cash"] == 49747 and last["company"]["debt"] == 5000000
    sold, = rows["sold"]
    invoice, = rows["invoices"]
    assert sold["vehicle"] == 39 and sold["invoice"] == invoice["cost"] == -2120 and sold["refund"] == 2120
    assert sold["elapsed"] == sold["earned"] == sold["credit"] == sold["future_refund_counted_as_cash"] == 0
    assert sold["retained"] == 2 and sold["retained_capacity"] == 27 and sold["chemical_retained"] == 55
    admission, = rows["admission"]
    assert admission["funded"] and admission["ceiling"] == admission["ticks"] == sold["funded_cap"] == 500
    assert admission["unpaid"] == admission["future_income_counted"] == 0
    assert admission["cash"] == last["company"]["cash"] and admission["target"] == admission["operating"]+40000 <= last["company"]["cash"]
    # The same genuine post-sale checkpoint must fail the historical
    # observer that still requires the redundant body. This proves the new
    # explicit retirement mode has not silently widened an old default.
    try:
        BASE["observe"](output)
    except AssertionError:
        operation["historical_refill_required_observer_rejected"] = True
    else:
        raise AssertionError("Historical refill ownership default was widened")
    operation.update(actual_sale_complete=True,actual_refund_banked=2120,earned_ticks=0,retired_own_orderlist=own-1)
    return operation


def main():
    """Publish one immutable actual native retirement and banked-funding receipt."""
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
