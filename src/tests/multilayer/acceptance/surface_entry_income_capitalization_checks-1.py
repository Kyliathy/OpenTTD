"""Admit actual civic coverage and unchanged fixed-price recovery without mutation.

@file surface_entry_income_capitalization_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSD.
Historical Salt profit crosses a fiscal rollover. Native current quotes retain
every money, packet, authority, clock, useful role and original asset owner.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_capital_recovery_checks-1.py"))
observe = BASE["observe"]


def salt_profit(configuration):
    """Reconcile the qualified actual Salt head across its saved profit rollover.

    @param configuration Independently qualified earlier original world.
    @return Exact historical identities and measured head profit, excluding other trains.
    """
    proof_path = ROOT / "logs/surface-entry-salt-delivery-verification.json"
    proof = json.loads(proof_path.read_text(encoding="utf-8"))
    assert proof["complete"] and proof["salt_delivered"]
    origin, = [row for row in proof["configurations"] if row["configuration"] == configuration]
    reference = origin["receipt"]
    target = f"logs/surface-entry-salt-delivery-work-01-{configuration}.json"
    # Follow only the accepted original's short receipt chain. Historical
    # applications are never replayed to manufacture an income opportunity.
    for _ in range(8):
        path = ROOT / reference["path"]
        assert identity(path) == reference
        receipt = json.loads(path.read_text(encoding="utf-8"))
        if reference["path"] == target:
            break
        reference = receipt["operation"]["receipt"]
    else:
        raise AssertionError("qualified Salt work absent from accepted lineage")
    operation = receipt["operation"]
    before, after = [ROOT / operation[key]["path"] for key in ("before", "output")]
    assert identity(before) == operation["before"] and identity(after) == operation["output"]
    a, b = [TABLE(path, b"VEHS")[55] for path in (before, after)]
    prefix = "train[0].common[0]."
    assert a[prefix + "engine_type"] == b[prefix + "engine_type"] == 24
    assert a[prefix + "profit_this_year"] == -29824 and a[prefix + "profit_last_year"] == 0
    assert b[prefix + "profit_this_year"] == 33927 and b[prefix + "profit_last_year"] == -39929
    leg = b[prefix + "profit_this_year"] + b[prefix + "profit_last_year"] - a[prefix + "profit_this_year"]
    cycle = b[prefix + "profit_this_year"] + b[prefix + "profit_last_year"]
    assert leg == 23822 and cycle == -6002
    expenses = {row["type"]: row["amount"] for row in operation["rows"]["expenses"]}
    return dict(proof=identity(proof_path), receipt=reference, before=operation["before"], output=operation["output"],
                delivery_profit256=leg, complete_cycle_profit256=cycle, aggregate_train_revenue=-expenses[7],
                profitable_before_purchase=False, new_salt_train_admitted=False)


def passenger_state(path):
    """Decode current sole passenger queues, bus positions and unchanged native rates.

    @param path Current original native saved world.
    @return Actual civic ratings, waiting packets, bus occupancy and rate settings.
    """
    stations, packets, vehicles = [TABLE(path, key) for key in (b"STNN", b"CAPA", b"VEHS")]
    stops = []
    # Saved packet references are one-based native references. Count actual
    # queued units once; historical maximums and town population are not demand.
    for station in (17, 18, 20):
        row = stations[station]
        prefix = "normal[0].goods[0]."
        references = [reference for key, values in row.items() if key.startswith(prefix + "cargo[") and key.endswith(".second") for reference in values]
        assert len(references) == len(set(references))
        stops.append(dict(station=station, waiting=sum(packets[ref - 1]["count"] for ref in references),
                          reserved=row[prefix + "cargo.reserved_count"], rating=row[prefix + "rating"]))
    buses = []
    # Preserve the measured fleet's actual occupancy and position. An empty
    # bus in transit is not proof that a stop can accept more vehicles.
    for vehicle, row in vehicles.items():
        if row["type"] != 1:
            continue
        prefix = "roadveh[0].common[0]."
        if row[prefix + "cargo_type"] != 0:
            continue
        buses.append(dict(vehicle=vehicle, tile=row[prefix + "tile"], speed=row[prefix + "cur_speed"],
                          capacity=row[prefix + "cargo_cap"], held=sum(packets[ref - 1]["count"] for ref in row[prefix + "cargo.packets"])))
    settings = TABLE(path, b"PATS")[0]
    assert settings["economy.inflation"] == 0 and settings["difficulty.initial_interest"] == 2
    towns = [dict(town=town, rating=row["ratings"][0]) for town, row in TABLE(path, b"CITY").items()]
    return dict(stops=stops, buses=buses, towns=towns, inflation=False, initial_interest=2)


def execute(configuration, receipt, recover_run=None):
    """Run a pure current civic and complete fixed-price operating admission.

    @param configuration Independently linked focused native application.
    @param receipt Exact selected original completed freight-trial receipt.
    @param recover_run Successful invocation to inspect without repeating it.
    @return Unchanged current physical world and fully costed next proposal.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["operation"]["final"]["company"]["cash"] == 95487
    files = BASE["caller_files"]() + [Path(__file__), ROOT / "src/tests/surface_entry_income_capitalization.h",
        ROOT / "src/tests/surface_entry_freight_efficiency.h", ROOT / "src/economy.cpp", ROOT / "src/town_cmd.cpp",
        ROOT / "src/station_cmd.cpp", ROOT / "src/space_vehicle.cpp"]
    operation = native(configuration, "staged-chemical-capital-income-query", receipt, observe,
        dict(authority="INCOME_AUTHORITY", coverage="INCOME_COVERAGE", fixed="INCOME_FIXED_PRICES", admission="INCOME_ADMISSION"),
        files, recover_run=recover_run)
    assert operation["initial"] == operation["final"]
    assert json.loads(json.dumps(operation["initial"])) == previous["operation"]["final"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    assert chunks(before)[1] == chunks(output)[1]
    fixed, = operation["rows"]["fixed"]
    assert not fixed["inflation"] and fixed["callback_free"] and fixed["year_boundary"] and fixed["ticks"] == 3000
    assert operation["rows"]["authority"] == [dict(town=0, rating=-996, center=2606), dict(town=1, rating=1000, center=860)]
    admission = operation["rows"]["admission"]
    assert [row["reserve"] for row in admission] == [100000, 40000]
    assert not admission[0]["funded"] and admission[1]["funded"]
    for row in admission:
        assert row["projected_income"] == row["refunds_counted"] == row["credit"] == 0
        assert row["cash"] == 95487 and row["working_shortfall"] == 4513 and row["module_base_shortfall"] == 414513
        assert row["target"] == row["reserve"] + row["infrastructure"] + row["operating"]
    operation["historical_salt_profit"] = salt_profit(configuration)
    operation["passenger_state"] = passenger_state(output)
    return operation


def main():
    """Publish one immutable focused income-capitalization admission under logs."""
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
                                     phase="staged-chemical-capital-income-query", operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
