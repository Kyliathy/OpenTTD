"""Quote actual idle Goods assets, real production and lawful connected receivers.

@file surface_entry_goods_income_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSG.
This read-only admission never widens historical material observers or supplies
future cargo, passenger fares, bank credit or construction authority.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_income_capitalization_checks-1.py"))
observe = BASE["observe"]


def execute(configuration, receipt, recover_run=None):
    """Run one pure actual Goods source, existing-road and whole first-interval quote.

    @param configuration Independently linked focused native application.
    @param receipt Exact selected last actual coverage-recovery original.
    @param recover_run Successful invocation to inspect without repeating it.
    @return Unchanged current sole owners and actual admitted/refused proposal.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["operation"]["final"]["company"]["cash"] == 96397
    files = BASE["BASE"]["caller_files"]() + [Path(__file__), ROOT / "src/tests/surface_entry_goods_income.h",
        ROOT / "src/tests/surface_entry_income_capitalization.h", ROOT / "src/tests/surface_entry_passenger_coverage.h",
        ROOT / "src/economy.cpp", ROOT / "src/station_cmd.cpp", ROOT / "src/town_cmd.cpp", ROOT / "src/road_map.cpp"]
    operation = native(configuration, "staged-chemical-capital-goods-query", receipt, observe,
        dict(source="GOODS_SOURCE", production="GOODS_PRODUCTION", inputs="GOODS_INPUT", reserves="GOODS_RESERVE",
             receiver="GOODS_RECEIVER", fixed="INCOME_FIXED_PRICES", admission="GOODS_ADMISSION"), files, recover_run=recover_run)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    assert chunks(before)[1] == chunks(output)[1]
    rows = operation["rows"]
    assert len(rows["source"]) == len(rows["receiver"]) == len(rows["fixed"]) == 1
    assert {row["vehicle"] for row in rows["reserves"]} == {3, 25}
    assert all(row["asset_invoice"] == 0 and row["capacity"] == 18 and row["callback_free"] and row["remaining"] > 300 for row in rows["reserves"])
    admission = rows["admission"]
    assert [row["reserve"] for row in admission] == [100000, 40000] and not admission[0]["funded"]
    for row in admission:
        assert row["target"] == row["reserve"] + row["infrastructure"] + row["operating"]
        assert row["extra_upkeep"] == sum(item["future_upkeep"] for item in rows["reserves"])
        assert row["cash"] == 96397 and row["projected_fares"] == row["refunds_counted"] == row["credit"] == row["asset_invoice"] == 0
        assert row["funded"] == (row["cash_funded"] and row["receiver_admitted"])
    operation["passenger_state"] = BASE["passenger_state"](output)
    return operation


def main():
    """Publish one immutable native pure Goods-income admission under logs."""
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
                                     phase="staged-chemical-capital-goods-query", operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
