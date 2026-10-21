"""Quote current-compatible farm locomotives and honest unpriced-route bounds.

@file surface_entry_farm_fleet_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGG.
Native availability and engine masks reject imaginary refits/retired purchases.
No known-cost lower bound admits a train before its entire safe route is priced.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_farm_upgrade_checks-1.py"))
observe = BASE["observe"]


def execute(configuration, receipt, recover_run=None):
    """Run one pure actual further-farm head/wagon compatibility and price characterization.

    @param configuration Independently linked focused native application.
    @param receipt Exact original first useful one-wagon service checkpoint.
    @param recover_run Successful invocation to inspect without repeating it.
    @return Unchanged current original and actual native engine/refit/cost boundaries.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["operation"]["final"]["company"]["cash"] == 81662
    files = BASE["BASE"]["BASE"]["FREIGHT"]["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_farm_upgrade_checks-1.py",
        ROOT / "src/tests/surface_entry_farm_fleet.h", ROOT / "src/tests/surface_entry_farm_upgrade.h",
        ROOT / "src/tests/surface_entry_farm_capacity.h", ROOT / "src/tests/surface_entry_productive_capital.h",
        ROOT / "src/tests/surface_entry_goods_trade.h", ROOT / "src/tests/surface_entry_goods_income.h",
        ROOT / "src/tests/surface_entry_income_capitalization.h", ROOT / "src/tests/surface_entry_passenger_coverage.h",
        ROOT / "src/engine.cpp", ROOT / "src/train_cmd.cpp", ROOT / "src/economy.cpp"]
    operation = native(configuration, "staged-chemical-capital-farm-fleet-query", receipt, observe,
        dict(source="FARM_FLEET_SOURCE", heads="FARM_FLEET_HEAD", refusals="FARM_FLEET_REFUSAL", bounds="FARM_FLEET_LOWER_BOUND",
             fixed="INCOME_FIXED_PRICES", summary="FARM_FLEET_SUMMARY"), files, recover_run=recover_run)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    assert chunks(before)[1] == chunks(output)[1]
    source, = operation["rows"]["source"]
    assert source["head"] == 52 and source["length"] == source["max_tiles"] * 16 == 112
    assert source["original_wagons"] == 32 and source["added_wagons"] == 1 and source["wagon"] == 50
    assert source["purchases"] == source["changed_orders"] == 0
    fixed = operation["rows"]["fixed"]
    assert [row["ticks"] for row in fixed] == [2000, 3000]
    assert all(row["callback_free"] and not row["inflation"] and row["priced_bodies"] == 16 for row in fixed)
    summary, = operation["rows"]["summary"]
    assert summary["accepted"] == len(operation["rows"]["heads"]) and summary["refused"] == len(operation["rows"]["refusals"])
    assert summary["attempts"] == summary["accepted"] + summary["refused"] and summary["wagon_unit"] == 1125
    assert len(operation["rows"]["bounds"]) == summary["freight_heads"] * 8
    # Funding a known-cost sum cannot supply the missing passing/terminal
    # producer. Every preliminary whole-route admission therefore stays false.
    for row in operation["rows"]["bounds"]:
        assert row["known_target"] == row["reserve"] + row["assets"] + row["operating"]
        assert row["cash"] == 81662 and row["known_funded"] == (row["known_target"] <= 81662)
        assert not row["infrastructure_priced"] and not row["route_admitted"] and not row["whole_funded"]
        assert row["capacity"] == row["wagons"] * 25 and row["length"] <= 112
        assert row["projected_fares"] == row["refunds_counted"] == row["credit"] == 0
    stations, packets = [TABLE(output, key) for key in (b"STNN", b"CAPA")]
    farm = stations[15]
    for cargo, name in ((4, "waiting_livestock"), (6, "waiting_grain")):
        references = [reference for key, values in farm.items() if key.startswith(f"normal[0].goods[{cargo}].cargo[") and key.endswith(".second") for reference in values]
        assert source[name] == sum(packets[reference - 1]["count"] for reference in references)
    return operation


def main():
    """Publish one immutable pure further-farm fleet characterization receipt."""
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
                                     phase="staged-chemical-capital-farm-fleet-query", operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
