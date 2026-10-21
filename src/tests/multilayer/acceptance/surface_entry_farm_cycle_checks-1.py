"""Measure one fully admitted retained farm-to-factory producer cycle.

@file surface_entry_farm_cycle_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGI.
Physical offers and current production are observations; only present cash
funds at most two separately quoted intervals within one6000tick ceiling.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_farm_rebalance_checks-1.py"))
observe = BASE["observe"]
QUERY = "staged-chemical-capital-farm-cycle-query"
WORK = "staged-chemical-capital-farm-cycle-work"


def producers(path):
    """@param path Exact current native original. @return Sole offers, recent producer histories and one-based native town/statue facts."""
    industries, stations, packets = [TABLE(path, key) for key in (b"INDY", b"STNN", b"CAPA")]
    offers, histories = [], []
    # Only physical packet references define waiting cargo. Recent production
    # and peaks cannot be spent as a second copy of these same offers.
    for station, cargo in ((12, 5), (15, 4), (15, 6), (19, 5)):
        row = stations[station]
        prefix = f"normal[0].goods[{cargo}]."
        refs = [ref for key, values in row.items() if key.startswith(prefix + "cargo[") and key.endswith(".second") for ref in values]
        offers.append(dict(station=station, cargo=cargo, waiting=sum(packets[ref - 1]["count"] for ref in refs), rating=row[prefix + "rating"]))
    # Preserve incomplete current-month history separately from the last full
    # month. A partial month cannot diagnose decline or promise later output.
    for industry in (5, 7, 11):
        row = industries[industry]
        for slot in range(row["produced.count"]):
            prefix = f"produced[{slot}]."
            histories.append(dict(industry=industry, cargo=row[prefix + "cargo"], buffered=row[prefix + "waiting"],
                partial_production=row[prefix + "history[0].production"], full_production=row[prefix + "history[1].production"],
                full_transported=row[prefix + "history[1].transported"]))
    towns = TABLE(path, b"CITY")
    primary_towns = {stations[key]["normal[0].base[0].town"] - 1 for key in (12, 14, 15)}
    assert primary_towns == {0} and towns[0]["statues"] & 1
    return dict(offers=offers, histories=histories, primary_source_town=0, primary_source_statue_already_owned=True)


def execute(configuration, stage, receipt, recover_run=None):
    """Run a pure whole-cycle quote or one of its two explicitly bounded paid intervals.

    @param configuration Independently linked focused application.
    @param stage Query, first work or second/final work within this one cycle.
    @param receipt Exact immediately preceding actual native original receipt.
    @param recover_run Successful invocation to inspect without repeating work.
    @return Actual owners, production/load endpoints and cost-only admission or earned cash.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    expected = {"query": BASE["PHASES"]["service"], "work-01": QUERY, "work-02": WORK}
    assert previous["complete"] and previous["phase"] == expected[stage]
    if stage == "work-02":
        assert previous["operation"]["cycle_interval"] == 1
        assert previous["operation"]["final"]["company"]["cash"] < 100000
    phase = QUERY if stage == "query" else WORK
    root = receipt if stage == "work-01" else ROOT / previous["operation"]["cycle_admission"]["path"] if stage == "work-02" else None
    if stage == "work-02":
        assert identity(root) == previous["operation"]["cycle_admission"]
    if root is not None:
        whole, = [row for row in json.loads(root.read_text(encoding="utf-8"))["operation"]["rows"]["whole"] if row["reserve"] == 40000]
        assert whole["funded"] and whole["target"] <= whole["cash"] == 72259 and whole["ticks"] == 6000
    files = BASE["BASE"]["FREIGHT"]["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_farm_rebalance_checks-1.py",
        ROOT / "src/tests/surface_entry_farm_cycle.h", ROOT / "src/tests/surface_entry_farm_rebalance.h",
        ROOT / "src/tests/surface_entry_farm_upgrade.h", ROOT / "src/tests/surface_entry_module_capital.h",
        ROOT / "src/tests/surface_entry_income_capitalization.h", ROOT / "src/station_cmd.cpp", ROOT / "src/economy.cpp"]
    operation = native(configuration, phase, receipt, observe,
        dict(whole="CYCLE_ADMISSION", rail="CYCLE_RAIL", offers="CYCLE_OFFER", use="BALANCE_USE", service="BALANCE_SERVICE",
             admission="FARM_UPGRADE_ADMISSION", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"),
        files, recover_run=recover_run, chemical_limit=3000)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    operation["producers_before"], operation["producers_after"] = producers(before), producers(output)
    assert BASE["strict_default_rejects"](before) and BASE["strict_default_rejects"](output)
    if stage == "query":
        assert chunks(before)[1] == chunks(output)[1] and first == last
        assert [row["reserve"] for row in rows["whole"]] == [100000, 40000]
        assert not rows["whole"][0]["funded"]
        for row in rows["whole"]:
            assert row["ticks"] == 6000 and row["target"] == row["reserve"] + row["operating"]
            assert row["operating"] == row["upkeep"] + row["interest"] + row["station_charge"]
            assert row["purchases"] == row["future_income_counted"] == row["refunds_counted"] == row["credit"] == 0
        native_offers = {(row["station"], row["cargo"]): row["waiting"] for row in rows["offers"]}
        assert native_offers == {(row["station"], row["cargo"]): row["waiting"] for row in operation["producers_before"]["offers"]}
    else:
        elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
        earned, = rows["earned"]
        service, = rows["service"]
        assert 0 < elapsed == earned["earned"] == service["ticks"] <= 3000
        assert earned["reached"] or elapsed == 3000
        assert all(row["funded"] and row["target"] <= first["company"]["cash"] for row in rows["admission"])
        assert service["restored"] == (last["company"]["cash"] >= 100000) and last["company"]["cash"] >= 40000
        assert {row["wagon"] for row in rows["use"]} == {57, 58, 59}
        expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
        assert sum(expenses.values()) == first["company"]["cash"] - last["company"]["cash"]
        operation["measured_cash_flow"] = dict(ticks=elapsed, actual_fares=-expenses[7] - expenses[8], interest=expenses[11],
            other_operating_cost=sum(v for k, v in expenses.items() if k not in (7, 8, 11)),
            cash_delta=last["company"]["cash"] - first["company"]["cash"], annual_projection=False)
        operation["cycle_interval"] = 1 if stage == "work-01" else 2
        operation["cycle_ticks"] = elapsed + (previous["operation"]["cycle_ticks"] if stage == "work-02" else 0)
        assert operation["cycle_ticks"] <= 6000
        operation["cycle_admission"] = identity(root)
    return operation


def main():
    """Publish one immutable admission or earned interval without replaying prior native work."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--stage", choices=("query", "work-01", "work-02"), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.stage, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     phase=QUERY if args.stage == "query" else WORK, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
