"""Quote redundant empty refill capital while keeping actual current funding refused.

@file surface_entry_refill_capital_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZE.
Original39remains owned until a separately admitted actual sale. Its native
refund estimate and conditional balance are never counted as current money.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT,chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY/"surface_entry_goods_rail_feeder_work_checks-1.py"))
observe,ROAD = [BASE[k] for k in ("observe","ROAD")]
PHASE = "staged-chemical-capital-refill-capital-query"


def execute(configuration,receipt,recover_run=None):
    """Purely quote native empty39sale and conditional post-banked-sale operating admission.

    @param configuration Independently linked focused native application.
    @param receipt Exact original full-funding refusal endpoint of the finite Goods followthrough.
    @param recover_run Successful pure invocation to inspect without repeating it.
    @return Unchanged original owners, actual legal quote and explicitly unbanked conditional funding.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] == BASE["PHASES"]["work"]
    assert previous["operation"]["work_refused"] and previous["operation"]["final"]["company"]["cash"] == 47627
    files = [ROOT/r["path"] for r in previous["operation"]["sources"]]+[Path(__file__),
        ROOT/"src/tests/surface_entry_refill_capital.h",ROOT/"src/vehicle_cmd.cpp"]
    operation = native(configuration,PHASE,receipt,observe,
        dict(source="REFILL_CAPITAL_SOURCE",admission="REFILL_CAPITAL_ADMISSION"),files,recover_run=recover_run,chemical_limit=500)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before,output = [ROOT/operation[k]["path"] for k in ("before","output")]
    assert chunks(before)[1] == chunks(output)[1]
    source, = operation["rows"]["source"]
    assert source["vehicle"] == 39 and source["retained"] == 2 and source["chemical_retained"] == 55
    assert source["engine"] == 134 and source["capacity"] == 27 and source["same_role_retained"]
    assert source["held"] == source["paid"] == source["ticks"] == 0 and not source["work_permission"]
    vehicles = TABLE(output,b"VEHS")
    for id,name in ((39,"Surface Propellant refill"),(2,"Surface Propellant reserve")):
        row = vehicles[id]
        assert row[ROAD+"owner"] == 0 and row[ROAD+"engine_type"] == 134 and row[ROAD+"cargo_cap"] == 27
        assert row[ROAD+"cargo_type"] == 26 and row[ROAD+"cargo.packets"] == [] and row[ROAD+"tile"] == 2443
        assert row[ROAD+"vehstatus"] & 2 and row["roadveh[0].state"] == 254
        assert bytes.fromhex(row[ROAD+"name"]).decode("utf-8") == name
    remaining = vehicles[2][ROAD+"max_age"]-vehicles[2][ROAD+"age"]
    assert remaining > 500 and source["value"] == vehicles[39][ROAD+"value"] == 2120
    if source["sale_available"]:
        assert source["sale_invoice"] == -source["value"] and source["refund"] == source["value"]
    else:
        assert source["refund"] == 0
    cases = operation["rows"]["admission"]
    assert [r["ticks"] for r in cases] == [2000,1000,500,250,64]
    for row in cases:
        assert row["target"] == row["operating"]+40000 and row["cash"] == 47627
        assert row["current_funded"] == (row["cash"] >= row["target"]) and not row["current_funded"]
        assert row["refund"] == source["refund"] and row["conditional_after_actual_sale"] == row["cash"]+source["refund"]
        assert row["funded_only_if_banked"] == (row["conditional_after_actual_sale"] >= row["target"])
        assert row["future_refund_counted_as_cash"] == row["credit"] == 0 and not row["work_permission"]
    operation["refill_retirement_admitted"] = bool(source["sale_available"] and any(r["funded_only_if_banked"] for r in cases))
    operation["retained_fuel_carrier"] = dict(vehicle=2,engine=134,capacity=27,remaining_days=remaining)
    operation["actual_sale_complete"] = False
    return operation


def main():
    """Publish one unchanged actual redundant-refill sale quote or legal refusal."""
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
