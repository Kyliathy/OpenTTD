"""Quote a balanced existing train retaining all original wagons as owned assets.

@file surface_entry_farm_balance_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGG.
Only three surplus Grain bodies may later become an empty coupled depot
reserve. They supply no refund, money or active capacity in this proposal.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_farm_fleet_checks-1.py"))
FREIGHT = runpy.run_path(str(DIRECTORY / "surface_entry_capital_recovery_checks-1.py"))
observe = BASE["observe"]


def execute(configuration, receipt, recover_run=None):
    """Run one pure full existing-train balance and remaining-work funding admission.

    @param configuration Independently linked focused native application.
    @param receipt Exact immediately preceding native head/refit/funding refusal.
    @param recover_run Successful invocation to inspect without repeating it.
    @return Unchanged original owners and actual parked-versus-active capacity/funding proposal.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] == "staged-chemical-capital-farm-fleet-query"
    assert not previous["operation"]["rows"]["source"][0]["grain_to_livestock_refittable"]
    assert not any(row["known_funded"] for row in previous["operation"]["rows"]["bounds"])
    files = FREIGHT["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_farm_fleet_checks-1.py",
        DIRECTORY / "surface_entry_farm_upgrade_checks-1.py", ROOT / "src/tests/surface_entry_farm_balance.h",
        ROOT / "src/tests/surface_entry_farm_fleet.h", ROOT / "src/tests/surface_entry_farm_upgrade.h",
        ROOT / "src/tests/surface_entry_farm_capacity.h", ROOT / "src/tests/surface_entry_productive_capital.h",
        ROOT / "src/tests/surface_entry_goods_trade.h", ROOT / "src/tests/surface_entry_goods_income.h",
        ROOT / "src/tests/surface_entry_income_capitalization.h", ROOT / "src/tests/surface_entry_passenger_coverage.h",
        ROOT / "src/engine.cpp", ROOT / "src/train_cmd.cpp", ROOT / "src/vehicle_cmd.cpp", ROOT / "src/economy.cpp"]
    operation = native(configuration, "staged-chemical-capital-farm-balance-query", receipt, observe,
        dict(source="FARM_BALANCE_SOURCE", admission="FARM_BALANCE_ADMISSION", fixed="INCOME_FIXED_PRICES"), files, recover_run=recover_run)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    assert chunks(before)[1] == chunks(output)[1]
    source, = operation["rows"]["source"]
    assert source["waiting_livestock"] == 269 and source["waiting_grain"] == 49
    assert source["proposed_livestock"] == 250 and source["proposed_grain"] == source["parked_grain_capacity"] == 90
    assert source["current_grain"] == source["proposed_grain"] + source["parked_grain_capacity"] == 180
    assert source["original_wagons_owned"] == 32 and source["parked_original"] == source["added_wagons_to_buy"] == 3
    assert source["length"] == source["max_tiles"] * 16 == 112 and source["existing_extra"] == 50
    assert source["new_head"] == source["infrastructure"] == source["recall"] == source["purchases"] == source["changed_orders"] == 0
    cases = operation["rows"]["admission"]
    assert [row["reserve"] for row in cases] == [100000, 40000] and not cases[0]["funded"] and cases[1]["funded"]
    for row in cases:
        assert row["invoice"] == 3375 and row["extra_running"] == 0 and row["ticks"] == 6000
        assert row["target"] == row["reserve"] + row["invoice"] + row["operating"] and row["cash"] == 81662
        assert row["parked_original"] == 3 and row["original_wagons_owned"] == 32
        assert row["projected_fares"] == row["refunds_counted"] == row["credit"] == 0
    return operation


def main():
    """Publish one immutable full existing-train balance proposal without paid mutation."""
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
                                     phase="staged-chemical-capital-farm-balance-query", operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
