"""Price remaining paid first-delivery work from actual source offers and cash.

@file surface_entry_productive_capital_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGC.
Actual producer histories and physical offers remain observations. Neither
current loaded freight nor a possible future payment funds the cost-only bill.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
TRADE = runpy.run_path(str(DIRECTORY / "surface_entry_goods_trade_checks-1.py"))
FREIGHT = runpy.run_path(str(DIRECTORY / "surface_entry_capital_recovery_checks-1.py"))
observe = TRADE["observe"]


def current_producers(path):
    """Decode actual production, transport and age evidence before more capitalization.

    @param path Exact unchanged native original world.
    @return Short current native history, actual station offers, obsolete reserves and loaded original consists.
    """
    industries, stations, packets, vehicles = [TABLE(path, key) for key in (b"INDY", b"STNN", b"CAPA", b"VEHS")]
    producers = []
    # Keep actual recent production and transport together. A temporary full
    # freight packet cannot conceal declining raw supply or create a forecast.
    for industry in (0, 5, 7, 9, 10, 11):
        row = industries[industry]
        products = []
        for slot in range(row["produced.count"]):
            prefix = f"produced[{slot}]."
            history = [dict(production=row.get(prefix + f"history[{month}].production", 0),
                            transported=row.get(prefix + f"history[{month}].transported", 0)) for month in range(4)]
            products.append(dict(cargo=row[prefix + "cargo"], waiting=row[prefix + "waiting"], history=history))
        producers.append(dict(industry=industry, type=row["type"], tile=row["location.tile"], level=row["prod_level"], products=products))
    offers = []
    # One-based saved packet references name the only physical offered units;
    # historical peaks and production totals cannot become available cargo.
    for station, cargo in ((1, 9), (12, 5), (14, 1), (15, 4), (15, 6), (19, 5)):
        row = stations[station]
        prefix = f"normal[0].goods[{cargo}]."
        references = [reference for key, values in row.items() if key.startswith(prefix + "cargo[") and key.endswith(".second") for reference in values]
        offers.append(dict(station=station, cargo=cargo, waiting=sum(packets[ref - 1]["count"] for ref in references),
                           rating=row[prefix + "rating"], time_since_pickup=row[prefix + "time_since_pickup"]))
    old = []
    for vehicle in (4, 5):
        row = vehicles[vehicle]
        prefix = "roadveh[0].common[0]."
        old.append(dict(vehicle=vehicle, engine=row[prefix + "engine_type"], capacity=row[prefix + "cargo_cap"],
                        remaining=row[prefix + "max_age"] - row[prefix + "age"]))
    assert all(row["remaining"] < 0 for row in old)
    return dict(producers=producers, offers=offers, obsolete_steel_reserves=old,
                original_consists=[dict(vehicle=key[0], cargo=key[1], capacity=value[0], held=value[1]) for key, value in FREIGHT["saved_consists"](path).items()])


def execute(configuration, receipt, recover_run=None):
    """Run one pure full remaining-work capitalization without new purchases or cargo exceptions.

    @param configuration Independently linked focused native application.
    @param receipt Exact original partial first Goods trade measurement.
    @param recover_run Successful invocation to inspect without repeating it.
    @return Unchanged sole ownership, actual producer audit and complete6000tick funding cases.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["operation"]["final"]["company"]["cash"] == 84595
    files = FREIGHT["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_goods_trade_checks-1.py",
        ROOT / "src/tests/surface_entry_productive_capital.h", ROOT / "src/tests/surface_entry_goods_trade.h",
        ROOT / "src/tests/surface_entry_goods_income.h", ROOT / "src/tests/surface_entry_income_capitalization.h",
        ROOT / "src/tests/surface_entry_passenger_coverage.h", ROOT / "src/economy.cpp"]
    operation = native(configuration, "staged-chemical-capital-productive-query", receipt, observe,
        dict(source="PRODUCTIVE_SOURCE", admission="PRODUCTIVE_ADMISSION", fixed="INCOME_FIXED_PRICES"), files, recover_run=recover_run)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    assert chunks(before)[1] == chunks(output)[1]
    source, = operation["rows"]["source"]
    assert source["waiting"] == 9 and source["connected"] and source["purchases"] == source["changed_orders"] == 0
    fixed, = operation["rows"]["fixed"]
    assert fixed["callback_free"] and not fixed["inflation"] and fixed["priced_bodies"] == 16
    cases = operation["rows"]["admission"]
    assert [row["reserve"] for row in cases] == [100000, 40000] and not cases[0]["funded"] and cases[1]["funded"]
    for row in cases:
        assert row["ticks"] == 6000 and row["interval_limit"] == 3000 and row["intervals"] == 2
        assert row["target"] == row["reserve"] + row["operating"] and row["cash"] == 84595
        assert row["purchases"] == row["future_income_counted"] == row["refunds_counted"] == row["credit"] == 0
    operation["current_producers"] = current_producers(output)
    actual, = [row for row in operation["current_producers"]["offers"] if row["station"] == 19]
    assert actual["waiting"] == source["waiting"]
    return operation


def main():
    """Publish one immutable read-only productive freight capitalization receipt."""
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
                                     phase="staged-chemical-capital-productive-query", operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
