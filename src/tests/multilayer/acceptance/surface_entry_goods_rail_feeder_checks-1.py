"""Quote actual Factory13 Goods transfer into original49's spare existing rail capacity.

@file surface_entry_goods_rail_feeder_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZB.
Only a new joined12 road bay/branch is proposed. Original rail lines, all
owners and source defaults stay unchanged; future final fares supply no funding.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT,chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY/"surface_entry_return_passenger_delivery_checks-1.py"))
observe = BASE["observe"]
PHASE = "staged-chemical-capital-goods-rail-feeder-query"


def execute(configuration,receipt,recover_run=None):
    """Purely admit or refuse one complete actual Goods feeder and its whole operating bill.

    @param configuration Independently linked focused native application.
    @param receipt Exact completed original return-delivery/payment endpoint.
    @param recover_run Successful pure invocation to inspect without rerunning it.
    @return Unchanged original owners and literal complete geometry/command/funding proposal.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] == BASE["PHASE"]
    assert previous["operation"]["return_payment_posted"]
    files = [ROOT/r["path"] for r in previous["operation"]["sources"]]+[Path(__file__),
        ROOT/"src/tests/surface_entry_goods_rail_feeder.h",ROOT/"src/station_cmd.cpp",ROOT/"src/road_cmd.cpp"]
    operation = native(configuration,PHASE,receipt,observe,
        dict(source="GOODS_RAIL_FEEDER_SOURCE",search="GOODS_RAIL_FEEDER_SEARCH",proposal="GOODS_RAIL_FEEDER_PROPOSAL",orders="GOODS_RAIL_FEEDER_ORDER",
             admission="GOODS_RAIL_FEEDER_ADMISSION"),files,recover_run=recover_run,chemical_limit=2000)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before,output = [ROOT/operation[k]["path"] for k in ("before","output")]
    assert chunks(before)[1] == chunks(output)[1]
    source, = operation["rows"]["source"]
    assert source["truck"] == 3 and source["engine"] == 140 and source["capacity"] == 18
    assert source["pickup"] == 13 and source["transfer"] == 12 and source["rail"] == 49 and source["final_receiver"] == 11
    station,packets = TABLE(output,b"STNN")[13],TABLE(output,b"CAPA")
    cargo = [packets[ref-1] for k,refs in station.items() if k.startswith("normal[0].goods[5].cargo[") and k.endswith(".second") for ref in refs]
    assert all(p["source_type"] == 0 and p["source_id"] in (5,11) and p["source"] == 13 for p in cargo)
    assert source["physical"] == sum(p["count"] for p in cargo) == 508
    assert source["source5"] == sum(p["count"] for p in cargo if p["source_id"] == 5) == 493
    assert source["source11"] == sum(p["count"] for p in cargo if p["source_id"] == 11) == 15
    vehicles = TABLE(output,b"VEHS"); prefix = "train[0].common[0]."
    chain = [49]+list(range(26,38))
    assert source["rail_capacity"] == sum(vehicles[id][prefix+"cargo_cap"] for id in chain) == 300
    assert source["rail_held"] == sum(packets[r-1]["count"] for id in chain for r in vehicles[id][prefix+"cargo.packets"]) == 78
    order, = operation["rows"]["orders"]
    assert order["destination"] == 12 and not order["available"] and order["prerequisite"] == "paid_truck_bay"
    assert order["control_destination"] == 13 and order["control_available"] and order["control_invoice"] == order["paid"] == 0
    proposal, = operation["rows"]["proposal"]
    search, = operation["rows"]["search"]
    assert proposal["found"] == search["found"] and not proposal["new_station"]
    assert proposal["rail_changes"] == proposal["paid"] == proposal["ticks"] == 0
    if proposal["found"]:
        assert 0 < proposal["distance"] <= 12 and proposal["bay_invoice"] >= 0 and proposal["road_invoice"] >= 0
    cases = operation["rows"]["admission"]
    assert [r["ticks"] for r in cases] == [2000,1000,500,250,64]
    for row in cases:
        assert row["invoice"] == proposal["bay_invoice"]+proposal["road_invoice"]
        assert row["target"] == row["invoice"]+row["operating"]+40000 and row["cash"] == 47833
        assert row["funded"] == (row["cash"] >= row["target"]) and row["complete"] == proposal["found"]
        assert row["future_income_counted"] == row["refunds_counted"] == row["credit"] == 0
    operation["goods_rail_feeder_admitted"] = bool(proposal["found"] and any(r["funded"] for r in cases))
    operation["actual_goods_offer"] = cargo
    return operation


def main():
    """Publish one immutable actual pure Goods rail-feeder admission or refusal."""
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
