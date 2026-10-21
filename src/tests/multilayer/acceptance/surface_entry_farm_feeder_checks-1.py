"""Inspect a complete actual short Farm7-to-rail15 feeder without paying or ticking.

@file surface_entry_farm_feeder_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGP.
Real tile authority, catchment and native complete road unions decide admission.
Neither future cargo nor virtual feeder credit supplies company spending power.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_farm_road_work_checks-1.py"))
observe = BASE["observe"]
PHASE = "staged-chemical-capital-farm-feeder-query"


def execute(configuration, receipt, recover_run=None):
    """Run pure whole feeder geometry and funding admission against the actual current world.

    @param configuration Independently linked focused native application.
    @param receipt Exact preceding successful first farm-road delivery receipt.
    @param recover_run Successful invocation to inspect without repeating it.
    @return Complete native proposal or exact refusal, with all original families unchanged.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["operation"]["measured_cash_flow"]["first_delivery"]
    assert receipt.name == f"surface-entry-farm-road-delivery-work-01-{configuration}.json"
    files = BASE["BASE"]["BASE"]["FREIGHT"]["caller_files"]() + [Path(__file__),
        DIRECTORY / "surface_entry_farm_road_work_checks-1.py", ROOT / "src/tests/surface_entry_farm_feeder.h",
        ROOT / "src/tests/surface_entry_farm_road_work.h", ROOT / "src/tests/surface_entry_farm_rebalance.h",
        ROOT / "src/tests/surface_entry_road_shortcut.h", ROOT / "src/tests/surface_entry_module_capital.h",
        ROOT / "src/tests/surface_entry_income_capitalization.h", ROOT / "src/station_cmd.cpp",
        ROOT / "src/road_cmd.cpp", ROOT / "src/order_cmd.cpp", ROOT / "src/economy.cpp"]
    operation = native(configuration, PHASE, receipt, observe,
        dict(source="FARM_FEEDER_SOURCE", search="FARM_FEEDER_SEARCH", proposal="FARM_FEEDER_PROPOSAL", admission="FARM_FEEDER_ADMISSION"),
        files, recover_run=recover_run, chemical_limit=2000)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    assert chunks(before)[1] == chunks(output)[1]
    source, = operation["rows"]["source"]
    search, = operation["rows"]["search"]
    proposal, = operation["rows"]["proposal"]
    assert source["vehicle"] == 62 and source["engine"] == 137 and source["capacity"] == 18 and source["held"] == 0
    assert source["industry"] == 7 and source["farm_tile"] == 1446 and source["farm_width"] == source["farm_height"] == 3
    assert source["transfer_station"] == 15 and source["rail"] == 52 and source["final_receiver"] == 13
    assert source["new_vehicle"] == source["paid"] == source["ticks"] == search["paid"] == search["ticks"] == 0
    assert not source["feeder_credit_is_cash"] and not proposal["paid"]
    assert search["found"] == proposal["found"] and proposal["strict_drag"] and proposal["new_station"]
    cases = operation["rows"]["admission"]
    if proposal["found"]:
        assert proposal["pickup"] != proposal["transfer"] and proposal["distance"] == 3
        assert 2 <= proposal["clearing"] <= search["quota"] and proposal["first"] <= proposal["anchor"] <= proposal["last"]
        assert [row["reserve"] for row in cases] == [100000, 40000] and not cases[0]["funded"]
        invoice = sum(proposal[key] for key in ("pickup_invoice", "transfer_invoice", "road_invoice"))
        # Both real floors use the identical complete gross invoice and whole
        # current company bill. Future final fares and virtual credits are zero.
        for row in cases:
            assert row["ticks"] == 2000 and row["invoice"] == invoice >= 0
            assert row["target"] == row["reserve"] + invoice + row["operating"]
            assert row["cash"] == 60955 and row["funded"] == (row["cash"] >= row["target"])
            assert row["future_income_counted"] == row["refunds_counted"] == row["credit"] == 0
        operation["farm_feeder_admitted"] = bool(cases[1]["funded"])
    else:
        assert not cases
        operation["farm_feeder_admitted"] = False
    return operation


def main():
    """Publish one immutable pure feeder quote or refusal from the exact current original world."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     phase=PHASE, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
