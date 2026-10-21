"""Quote a second existing Goods carrier on the real paid short feeder.

@file surface_entry_second_goods_feeder_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZK.
Loaded commercial stock, waiting demand and rail capacity are observed without
moving cargo, changing orders, earning time or crediting any prospective fare.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT,chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY/"surface_entry_constructor_capital_checks-1.py"))
observe = BASE["observe"]
PHASE = "staged-chemical-capital-second-goods-feeder-query"
ROAD = "roadveh[0].common[0]."


def waiting(path,station):
    """Read actual waiting Goods through the station's sole native packet references.

    @param path Genuine original native checkpoint.
    @param station Existing owned station12or13, never a new stock owner.
    @return Physical waiting quantity, without virtual-share or future-fare credit.
    """
    row,packets = TABLE(path,b"STNN")[station],TABLE(path,b"CAPA")
    refs = [ref for key,values in row.items() if key.startswith("normal[0].goods[5].cargo[") and key.endswith(".second") for ref in values]
    return sum(packets[ref-1]["count"] for ref in refs)


def execute(configuration,receipt,recover_run=None):
    """Quote only loaded25receiver/skip policy and current whole operating funding.

    @param configuration Independently linked focused native application.
    @param receipt Exact GZJ unchanged original endpoint, preserving its closed saving scopes.
    @param recover_run Successful pure invocation to inspect without another process.
    @return Unchanged actual owners and a separately admitted later route proposal.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] == BASE["PHASE"] and previous["configuration"] == configuration
    assert not previous["operation"]["actual_sale_complete"] and previous["operation"]["final"]["company"]["cash"] == 49250
    files = [ROOT/r["path"] for r in previous["operation"]["sources"]]+[Path(__file__),ROOT/"src/tests/surface_entry_second_goods_feeder.h"]
    operation = native(configuration,PHASE,receipt,observe,
        dict(source="SECOND_GOODS_FEEDER_SOURCE",admission="GOODS_RAIL_FEEDER_WORK_ADMISSION",selection="SECOND_GOODS_FEEDER_SELECTION"),
        files,recover_run=recover_run,chemical_limit=2000)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before,output = [ROOT/operation[k]["path"] for k in ("before","output")]
    assert chunks(before)[1] == chunks(output)[1]
    source, = operation["rows"]["source"]
    row,packets = TABLE(output,b"VEHS")[25],TABLE(output,b"CAPA")
    assert row[ROAD+"owner"] == 0 and row[ROAD+"engine_type"] == 140 and row[ROAD+"cargo_cap"] == 18
    assert sum(packets[ref-1]["count"] for ref in row[ROAD+"cargo.packets"]) == source["held"] == 18
    assert row[ROAD+"current_order.dest"] == source["old_receiver"] == 20 and source["proposed_receiver"] == 12
    assert source["vehicle"] == 25 and source["pickup"] == 13 and source["transfer"] and source["actual_facility"]
    assert source["delete_available"] and source["insert_available"] and source["invoice"] == source["paid"] == source["elapsed"] == 0
    assert source["current_index"] == 1 and source["skip_pickup_available"] and not source["skip_receiver_available"]
    assert source["rail_capacity"] == 300 and source["rail_first13"] == 86 and source["rail_share"] == 141
    assert source["future_income_counted"] == 0 and source["cash"] == 49250
    selected, = operation["rows"]["selection"]
    cases = operation["rows"]["admission"]
    assert [r["ticks"] for r in cases] == [2000,1000,500,250]
    assert all(r["target"] == r["operating"]+40000 and r["unpaid"] == r["future_income_counted"] == 0 for r in cases)
    assert not any(r["funded"] for r in cases[:-1]) and cases[-1]["funded"] and cases[-1]["target"] == 48938
    assert selected["funded"] and selected["ticks"] == 250 and selected["ceiling"] == 2000
    assert selected["route_quoted"] and not selected["order_change_complete"] and not selected["work_permission"]
    assert waiting(output,13) == 495 and waiting(output,12) == 18
    operation.update(second_feeder_admitted=True,actual_order_change_complete=False,actual_reserve_restored=False,
        module_capital_complete=False,actual_waiting_goods=dict(factory13=495,rail_pickup12=18),funded_cap=250)
    return operation


def main():
    """Publish one exact unchanged second-feeder quote with no physical time or order grant."""
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
