"""Characterize both genuine originals and quote complete paid income startup.

@file surface_entry_module_capital_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CO.
Native command tests grant no assets, passengers, cash or progress. Saved books
describe measured history; prospective fares never contribute to admission.
"""
import argparse
import json
from pathlib import Path
import re
import runpy

from space_preview_checks import ROOT, chunks
from surface_gateway_component_delivery_ledger import observe_propellant_delivery as observe
from surface_regional_ledger import TABLE, lives
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
PREVIOUS = runpy.run_path(str(DIRECTORY / "surface_entry_propellant_delivery_checks-1.py"))


def characterize(path):
    """Read actual useful services, packet demand and historical operating books.

    @param path Complete unchanged original native world.
    @return Reproducible observations, with capital purchases excluded from recurring cash history.
    """
    current = observe(path)
    vehicles, stations, packets, towns = [TABLE(path, key) for key in (b"VEHS", b"STNN", b"CAPA", b"CITY")]
    service = []
    # Only the eleven actual useful roles define renewal obligations. Empty
    # material reserves remain owned but never supply fictional operating profit.
    for item in lives(path):
        row = vehicles[item["vehicle"]]
        prefix = "train[0].common[0]." if row["type"] == 0 else "roadveh[0].common[0]."
        fields = {key: row[prefix + key] for key in ("cargo_cap", "cargo_type", "tile", "vehstatus", "cur_speed", "profit_last_year", "profit_this_year")}
        assert not fields["vehstatus"] & 2
        service.append(dict(item, **fields))
    assert len(service) == 11 and min(item["remaining"] for item in service) == 191
    company = TABLE(path, b"PLYR")[0]
    expenses = company["yearly_expenses"]
    assert len(expenses) == 39
    # Construction and purchased/sold vehicles are one-time capital movements.
    # Their refunds cannot disguise a recurring loss as sustainable fare income.
    history = [dict(period=index, expenses=expenses[index * 13:(index + 1) * 13],
                    operating_cash=-sum(expenses[index * 13 + 2:(index + 1) * 13])) for index in range(3)]
    assert history[1]["operating_cash"] == -18486
    demand = []
    # Saved native town history distinguishes actual production and transport;
    # the partial current month is retained separately from completed months.
    for town, row in towns.items():
        assert row["supplied[0].cargo"] == 0
        demand.append(dict(town=town, tile=row["xy"], passenger_history=[
            dict(production=row[f"supplied[0].history[{index}].production"],
                 transported=row[f"supplied[0].history[{index}].transported"]) for index in range(6)]))
    waiting = []
    # Sum only actual unreserved station packet references. Town production
    # counters and already loaded/reserved passengers are not station inventory.
    for station in (17, 18, 20):
        total = 0
        for key, references in stations[station].items():
            if re.fullmatch(r"normal\[0\]\.goods\[0\]\.cargo\[\d+\]\.second", key):
                total += sum(packets[packet - 1]["count"] for packet in references)
        waiting.append(dict(station=station, passengers=total))
    assert [item["passengers"] for item in waiting] == [29, 55, 15]
    return dict(company=current["company"], history=history, services=service, demand=demand, waiting=waiting,
                reserve_policy_changed=False, cargo=current["totals"], upcoming_renewals=[item for item in service if item["remaining"] <= 365])


def execute(configuration, receipt, recover_run=None):
    """Run one pure native coverage, purchase, renewal and two-interval cost quote.

    @param configuration Independently linked native focused application.
    @param receipt Completed actual post-fuel module quote, consumed unchanged.
    @param recover_run Successful invocation to inspect without rerunning it.
    @return Exact pure command evidence and actual financial/demand characterization.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["phase"] == "expansion-query" and predecessor["complete"]
    files = PREVIOUS["caller_files"]() + [Path(__file__), ROOT / "src/tests/surface_entry_module_capital.h",
        ROOT / "src/tests/surface_regional_layout.h", ROOT / "src/tests/surface_passenger_layout.h",
        ROOT / "src/engine.cpp", ROOT / "src/roadveh_cmd.cpp", ROOT / "src/station_cmd.cpp"]
    operation = native(configuration, "staged-chemical-capital-query", receipt, observe,
        dict(coverage="CAPITAL_COVERAGE", admission="CAPITAL_ADMISSION", renewal="CAPITAL_RENEWAL", services="CAPITAL_SERVICE"),
        files, recover_run=recover_run)
    assert operation["initial"] == operation["final"]
    assert chunks(ROOT / operation["before"]["path"])[1] == chunks(ROOT / operation["output"]["path"])[1]
    assert json.loads(json.dumps(operation["initial"])) == predecessor["operation"]["final"]
    rows = operation["rows"]
    assert len(rows["coverage"]) == 1 and len(rows["admission"]) == 12 and len(rows["renewal"]) == 4
    assert all(row["projected_fares"] == row["refund_counted"] == row["credit_counted"] == 0 for row in rows["admission"])
    assert all(row["ticks"] == 6000 and row["intervals"] == 2 and row["module_reserve"] == 150000 for row in rows["admission"])
    assert {row["reserve"] for row in rows["admission"]} == {40000, 100000}
    operation["characterization"] = characterize(ROOT / operation["output"]["path"])
    return operation


def main():
    """Publish one immutable pure admission receipt under project logs."""
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
                                     phase="staged-chemical-capital-query", operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
