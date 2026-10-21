"""Quote one actual farm wagon within unchanged train and platform limits.

@file surface_entry_farm_capacity_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGE.
The259queuedLivestock motivate capacity, but contribute no startup money.
Every original coupled body, Grain role and launch material owner remains.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_productive_capital_checks-1.py"))
observe = BASE["observe"]


def execute(configuration, receipt, recover_run=None):
    """Run one pure complete native compatible-wagon and empty-return admission.

    @param configuration Independently linked focused native application.
    @param receipt Exact selected completed paid-producer continuation original.
    @param recover_run Successful invocation to inspect without repeating it.
    @return Unchanged source queues, original bodies and complete fresh funding cases.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["operation"]["final"]["company"]["cash"] == 85710
    files = BASE["FREIGHT"]["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_goods_trade_checks-1.py",
        DIRECTORY / "surface_entry_productive_capital_checks-1.py", ROOT / "src/tests/surface_entry_farm_capacity.h",
        ROOT / "src/tests/surface_entry_productive_capital.h", ROOT / "src/tests/surface_entry_goods_trade.h",
        ROOT / "src/tests/surface_entry_goods_income.h", ROOT / "src/tests/surface_entry_income_capitalization.h",
        ROOT / "src/tests/surface_entry_passenger_coverage.h", ROOT / "src/engine.cpp", ROOT / "src/train_cmd.cpp", ROOT / "src/vehicle_cmd.cpp"]
    operation = native(configuration, "staged-chemical-capital-farm-query", receipt, observe,
        dict(capacity="FARM_CAPACITY", platforms="FARM_PLATFORM", fixed="INCOME_FIXED_PRICES", admission="FARM_ADMISSION"), files, recover_run=recover_run)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    assert chunks(before)[1] == chunks(output)[1]
    capacity, = operation["rows"]["capacity"]
    assert capacity["head"] == 52 and capacity["wagon_engine"] == 31 and capacity["native_capacity"] == 25
    assert capacity["original_livestock"] == 150 and capacity["proposed_livestock"] == 175 and capacity["grain"] == 180
    assert capacity["queued_livestock"] == 259 and capacity["proposed_length"] == capacity["max_tiles"] * 16 == 112
    assert capacity["recall"] == capacity["extra_running"] == capacity["changed_orders"] == capacity["purchases"] == 0
    assert capacity["connected"] and capacity["callback_free"] and capacity["invoice"] > 0
    assert len(operation["rows"]["platforms"]) == 2 and all(row["overhang"] == 0 for row in operation["rows"]["platforms"])
    settings = TABLE(output, b"PATS")[0]
    assert settings["vehicle.max_train_length"] == settings["station.station_spread"] - 5 == 7
    cases = operation["rows"]["admission"]
    assert [row["reserve"] for row in cases] == [100000, 40000] and not cases[0]["funded"] and cases[1]["funded"]
    for row in cases:
        assert row["target"] == row["reserve"] + row["invoice"] + row["operating"] and row["cash"] == 85710
        assert row["invoice"] == capacity["invoice"] and row["ticks"] == 6000 and row["interval_limit"] == 3000
        assert row["projected_fares"] == row["refunds_counted"] == row["credit"] == 0
    operation["current_producers"] = BASE["current_producers"](output)
    return operation


def main():
    """Publish one immutable read-only native farm-capacity admission."""
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
                                     phase="staged-chemical-capital-farm-query", operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
