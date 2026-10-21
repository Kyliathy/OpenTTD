"""Measure one freshly funded followthrough of the actual loaded Farm7shipment.

@file surface_entry_farm_road_delivery_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGO.
The existing immutable native worker owns all further movement, payment and
costs. Loaded packets and possible delivery income supply no funding credit.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_farm_road_work_checks-1.py"))
observe = BASE["observe"]
PHASE = BASE["PHASES"]["work"]


def execute(configuration, receipt, recover_run=None):
    """Earn only one newly quoted native2000tick followthrough of the real loaded batch.

    @param configuration Independently linked focused native application.
    @param receipt Exact immediately preceding first native farm-road work receipt.
    @param recover_run Successful invocation to inspect without repeating work.
    @return Actual custody, native first delivery or late partial and measured whole-company cash.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] == PHASE
    assert receipt.name == f"surface-entry-farm-road-work-01-{configuration}.json"
    before_body = previous["operation"]["final"]["farm_road"]
    assert before_body["vehicle"] == 62 and before_body["held"] == 15 and before_body["engine"] == 137 and before_body["capacity"] == 18
    prior, = previous["operation"]["rows"]["service"]
    assert prior["picked_up"] and not prior["delivered"] and prior["ticks"] == 2000
    files = BASE["BASE"]["BASE"]["FREIGHT"]["caller_files"]() + [Path(__file__),
        DIRECTORY / "surface_entry_farm_road_work_checks-1.py", DIRECTORY / "surface_entry_farm_road_checks-1.py",
        ROOT / "src/tests/surface_entry_farm_road_work.h", ROOT / "src/tests/surface_entry_farm_road.h",
        ROOT / "src/tests/surface_entry_module_capital.h", ROOT / "src/tests/surface_entry_income_capitalization.h",
        ROOT / "src/roadveh_cmd.cpp", ROOT / "src/economy.cpp"]
    operation = native(configuration, PHASE, receipt, observe,
        dict(service="FARM_ROAD_SERVICE", admission="FARM_ROAD_WORK_ADMISSION", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"),
        files, recover_run=recover_run, chemical_limit=2000)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert all(row["funded"] and row["target"] <= first["company"]["cash"] for row in rows["admission"])
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    earned, = rows["earned"]
    service, = rows["service"]
    assert 0 < elapsed == earned["earned"] == service["ticks"] <= 2000
    assert earned["reached"] or elapsed == 2000
    assert service["picked_up"] and service["vehicle"] == last["farm_road"]["vehicle"] == 62
    assert service["restored"] == (last["company"]["cash"] >= 100000) and last["company"]["cash"] >= 40000
    assert BASE["strict_default_rejects"](output)
    a, b = first["farm_road"], last["farm_road"]
    delta = b["profit256"] - a["profit256"]
    if a["previous_profit256"] != b["previous_profit256"]:
        delta += b["previous_profit256"]
    assert service["profit_delta_256"] == delta
    expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
    assert sum(expenses.values()) == first["company"]["cash"] - last["company"]["cash"]
    operation["measured_cash_flow"] = dict(ticks=elapsed, actual_fares=-expenses[7] - expenses[8], interest=expenses[11],
        other_operating_cost=sum(v for k, v in expenses.items() if k not in (7, 8, 11)),
        cash_delta=last["company"]["cash"] - first["company"]["cash"], new_truck_profit_256=delta,
        first_delivery=service["delivered"], annual_projection=False, startup_recouped=b["profit256"] >= 6042 * 256)
    return operation


def main():
    """Publish one immutable genuine loaded-delivery followthrough without replaying prior work."""
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
