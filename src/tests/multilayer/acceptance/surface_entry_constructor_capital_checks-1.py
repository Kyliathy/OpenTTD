"""Inspect the exhausted original constructor's legal lifecycle and continuing costs.

@file surface_entry_constructor_capital_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZJ.
Sale and Visit estimates preserve actual position, paid inputs and every owner.
Neither unbanked resale value nor prospective upkeep savings funds any action.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT,chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY/"surface_entry_feeder_reserve_checks-1.py"))
observe = BASE["observe"]
PHASE = "staged-chemical-capital-constructor-capital-query"
SPACE = "space[0].common[0]."


def execute(configuration,receipt,recover_run=None):
    """Quote actual sale refusal, legal home Visit and full existing operating bills.

    @param configuration Independently linked focused native application.
    @param receipt Actual closed non-growing reserve endpoint, never a new saving interval.
    @param recover_run Successful pure invocation to inspect without repeating it.
    @return Unchanged owners, real command estimates and separately disclosed replacement gaps.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration and previous["phase"] == BASE["PHASE"]
    scope = previous["operation"]["reserve_scope"]
    assert scope["ended"] and scope["end_reason"] == "non_growing_burst" and scope["window"] == 4
    assert previous["operation"]["final"]["company"]["cash"] == 49250
    files = [ROOT/r["path"] for r in previous["operation"]["sources"]]+[Path(__file__),
        ROOT/"src/tests/surface_entry_constructor_capital.h",ROOT/"src/vehicle_cmd.cpp",
        ROOT/"src/space_orders.cpp",ROOT/"src/space_vehicle.cpp",ROOT/"src/space_manifest.cpp"]
    operation = native(configuration,PHASE,receipt,observe,
        dict(source="CONSTRUCTOR_CAPITAL_SOURCE",admission="CONSTRUCTOR_CAPITAL_ADMISSION"),files,
        recover_run=recover_run,chemical_limit=500)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before,output = [ROOT/operation[k]["path"] for k in ("before","output")]
    assert chunks(before)[1] == chunks(output)[1]
    row = TABLE(output,b"VEHS")[38]
    source, = operation["rows"]["source"]
    assert row["type"] == 6 and row[SPACE+"owner"] == 0 and row[SPACE+"engine_type"] == 267
    assert not row[SPACE+"vehstatus"] & 2 and row[SPACE+"cur_speed"] == 0 and not row[SPACE+"cargo.packets"]
    assert source["vehicle"] == 38 and source["generation"] == 2 and source["model"] == 20 and source["placement"] == 0
    assert source["capital_paid"] == source["replacement_invoice"] == 100000
    assert source["value"] == row[SPACE+"value"] == 32606 and source["idle_per_day"] == 100
    assert not source["stopped"] and source["motionless"] and source["exhausted"] and not source["manifest_assets"] and source["held"] == 0
    # The public sale keeps its stopped-at-home restriction. A valid Visit estimate
    # resolves the same real home but grants no position, running intent,
    # tick, replacement waiver, resale proceeds or avoided daily charge.
    for field,expected in (("position_x",655360),("position_y",3342336),("home_x",655360),("home_y",2949120)):
        assert source[field] == row["space[0]."+field.replace("_",".") ] == expected
    assert not source["sale_available"] and source["refund_banked"] == source["ticks"] == 0
    assert source["visit_available"] and source["visit_invoice"] == 0 and source["destination_is_home"]
    assert source["home_center"] == 21 and source["home_generation"] == 1 and not source["work_permission"]
    assert (source["replacement_steel"],source["replacement_goods"],source["replacement_work"]) == (4,8,148)
    assert not source["replacement_launch_priced"]
    cases = operation["rows"]["admission"]
    assert [r["ticks"] for r in cases] == [2000,1000,500,250,64]
    # Existing fleet bills retain this still-present constructor's idle
    # charges and all real monthly debits. A legal return would need its
    # own bound ownership consumer; affordable estimates alone permit no work.
    for case in cases:
        assert case["cash"] == 49250 and case["target"] == case["operating"]+40000
        assert case["funded"] == (case["cash"] >= case["target"])
        assert case["unbanked_value"] == 32606 and case["refund_counted"] == case["avoided_cost_counted"] == 0
        assert not case["work_permission"]
    operation.update(actual_sale_available=False,actual_sale_complete=False,home_visit_quoted=True,
        constructor_return_complete=False,retirement_admitted=False,
        replacement_prerequisites=dict(capital=100000,steel=4,goods=8,work_ticks=148,
            additional_launch_and_service_unpriced=True,current_capital_and_floor_gap=90750),
        actual_reserve_restored=False,module_capital_complete=False)
    return operation


def main():
    """Publish one pure original constructor lifecycle quote with no time or owner change."""
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
