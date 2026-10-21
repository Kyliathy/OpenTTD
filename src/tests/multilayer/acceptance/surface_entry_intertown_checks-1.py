"""Improve only the existing paid extra bus through actual intertown delivery.

@file surface_entry_intertown_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGK.
Town0 remains the only passenger source. Existing Town1station20 accepts final
delivery under NoLoad, retaining every earlier material and original-role gate.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_farm_cycle_checks-1.py"))
observe = BASE["observe"]
PHASES = {stage: "staged-chemical-capital-intertown-" + stage for stage in ("query", "change", "work")}
ROAD = "roadveh[0].common[0]."


def bus(path):
    """@param path Actual native original. @return Exact paid extra-bus model, owned passengers, route and native cash book."""
    vehicles, packets, orders = [TABLE(path, key) for key in (b"VEHS", b"CAPA", b"ORDL")]
    row = vehicles[9]
    assert row["type"] == 1 and row[ROAD + "owner"] == 0 and row[ROAD + "engine_type"] == 119
    assert row[ROAD + "cargo_type"] == 0 and row[ROAD + "cargo_cap"] == 40
    cargo = [dict(packet=ref - 1, **packets[ref - 1]) for ref in row[ROAD + "cargo.packets"]]
    assert all(packet["source_type"] == 1 and packet["source_id"] == 0 for packet in cargo)
    held = sum(packet["count"] for packet in cargo)
    assert held == sum(row[ROAD + "cargo.action_counts"]) <= 40
    order_list = row[ROAD + "orders"] - 1
    return dict(vehicle=9, engine=119, capacity=40, held=held, packets=cargo, order_list=order_list, orders=orders[order_list],
                profit256=row[ROAD + "profit_this_year"], previous_profit256=row[ROAD + "profit_last_year"],
                remaining=row[ROAD + "max_age"] - row[ROAD + "age"])


def execute(configuration, stage, receipt, recover_run=None):
    """Run a pure intertown admission, zero-time order change or bounded real service.

    @param configuration Independently linked focused native application.
    @param stage Explicit query/change/work boundary, never a replay.
    @param receipt Exact immediately preceding actual native original receipt.
    @param recover_run Successful invocation to inspect without repeating commands/work.
    @return Real source/receiver/cost authority, preserved packet ownership and actual fares/profit/cash.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    expected = dict(query="staged-chemical-capital-paid-recovery-work", change=PHASES["query"], work=PHASES["change"])
    assert previous["complete"] and previous["phase"] == expected[stage]
    if stage == "query":
        assert previous["operation"]["recovery_window"] == 3 and previous["operation"]["final"]["company"]["cash"] == 62726
    root = receipt if stage == "change" else ROOT / previous["operation"]["intertown_admission"]["path"] if stage == "work" else None
    if stage == "work":
        assert identity(root) == previous["operation"]["intertown_admission"]
    if root is not None:
        whole, = [row for row in json.loads(root.read_text(encoding="utf-8"))["operation"]["rows"]["whole"] if row["funded"]]
        assert whole["reserve"] == 40000 and whole["target"] <= whole["cash"] == 62726 and whole["ticks"] == 2000
    files = BASE["BASE"]["BASE"]["FREIGHT"]["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_farm_cycle_checks-1.py",
        DIRECTORY / "surface_entry_farm_rebalance_checks-1.py", ROOT / "src/tests/surface_entry_intertown.h",
        ROOT / "src/tests/surface_entry_goods_income.h", ROOT / "src/tests/surface_entry_capital_income.h",
        ROOT / "src/tests/surface_entry_farm_upgrade.h", ROOT / "src/tests/surface_entry_module_capital.h",
        ROOT / "src/tests/surface_entry_income_capitalization.h", ROOT / "src/order_cmd.cpp", ROOT / "src/economy.cpp"]
    operation = native(configuration, PHASES[stage], receipt, observe,
        dict(source="INTERTOWN_SOURCE", whole="INTERTOWN_ADMISSION", changed="INTERTOWN_CHANGED", service="INTERTOWN_SERVICE",
             admission="FARM_UPGRADE_ADMISSION", invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"),
        files, recover_run=recover_run, chemical_limit=2000)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    a, b = bus(before), bus(output)
    operation["bus_before"], operation["bus_after"] = a, b
    assert a["remaining"] > 0 and b["remaining"] > 0
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    if stage == "query":
        assert chunks(before)[1] == chunks(output)[1] and first == last
        source, = rows["source"]
        assert source["vehicle"] == 9 and source["held"] == a["held"] == 40 and source["waiting"] == 55
        assert source["source_town"] == 0 and source["receiver_town"] == 1 and source["connected"] and source["component"] > 0
        assert source["order_cost"] == source["purchases"] == source["projected_fares"] == 0
        assert [row["reserve"] for row in rows["whole"]] == [100000, 40000] and not rows["whole"][0]["funded"]
        for row in rows["whole"]:
            assert row["ticks"] == 2000 and row["target"] == row["reserve"] + row["operating"]
            assert row["order_cost"] == row["future_income_counted"] == row["refunds_counted"] == row["credit"] == 0
    else:
        orders = b["orders"]
        assert orders["orders.count"] == 2 and orders["orders[0].dest"] == 17 and orders["orders[1].dest"] == 20
        assert orders["orders[0].flags"] & 7 == 4 and (orders["orders[0].flags"] >> 4) & 7 == 0
        assert orders["orders[1].flags"] & 7 == 0 and (orders["orders[1].flags"] >> 4) & 7 == 4
        assert all(row["funded"] and row["target"] <= first["company"]["cash"] for row in rows["admission"])
        if stage == "change":
            assert elapsed == 0 and first["company"] == last["company"]
            assert a["packets"] == b["packets"] and a["held"] == b["held"] == 40
            assert chunks(before)[1][b"CAPA"] == chunks(output)[1][b"CAPA"]
            vehicles = [TABLE(path, b"VEHS") for path in (before, output)]
            assert vehicles[0].keys() == vehicles[1].keys()
            # Native commands may reassign only extra9. Every original role,
            # freight reserve, train body and all other order lists stay exact.
            assert all(vehicles[0][key] == vehicles[1][key] for key in vehicles[0] if key != 9)
            lists = [TABLE(path, b"ORDL") for path in (before, output)]
            assert a["order_list"] == b["order_list"] and lists[0].keys() == lists[1].keys()
            assert all(lists[0][key] == lists[1][key] for key in lists[0] if key != a["order_list"])
            assert TABLE(before, b"DATE") == TABLE(output, b"DATE")
            assert sum(row["cost"] for row in rows["invoices"]) == 0
        else:
            earned, = rows["earned"]
            service, = rows["service"]
            assert 0 < elapsed == earned["earned"] == service["ticks"] <= 2000
            assert earned["reached"] or elapsed == 2000
            assert service["restored"] == (last["company"]["cash"] >= 100000) and last["company"]["cash"] >= 40000
            delta = b["profit256"] - a["profit256"]
            if a["previous_profit256"] != b["previous_profit256"]:
                delta += b["previous_profit256"]
            assert service["profit_delta_256"] == delta
            expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
            assert sum(expenses.values()) == first["company"]["cash"] - last["company"]["cash"]
            operation["measured_cash_flow"] = dict(ticks=elapsed, actual_fares=-expenses[7] - expenses[8], interest=expenses[11],
                other_operating_cost=sum(v for k, v in expenses.items() if k not in (7, 8, 11)),
                cash_delta=last["company"]["cash"] - first["company"]["cash"], extra_bus_profit_256=delta, annual_projection=False)
        operation["intertown_admission"] = identity(root)
    return operation


def main():
    """Publish one immutable actual intertown boundary without replaying successful native commands/work."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--stage", choices=tuple(PHASES), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.stage, ROOT / args.receipt, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     phase=PHASES[args.stage], operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
