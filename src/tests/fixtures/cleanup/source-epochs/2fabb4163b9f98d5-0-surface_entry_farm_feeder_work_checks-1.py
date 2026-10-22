"""Pay the admitted native short feeder and measure its first real approach/cycle.

@file surface_entry_farm_feeder_work_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGQ.
Separate original checkpoints preserve paid roads/stops before order and work
boundaries. Virtual transfer profit is never presented as company cash.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_farm_feeder_checks-1.py"))
observe = BASE["observe"]
PHASES = {stage: "staged-chemical-capital-farm-feeder-" + stage for stage in ("road", "stops", "orders", "work")}
ROAD = "roadveh[0].base[0]."


def pickup_identity(path):
    """Read actual new station ownership from its paid native tile anchor.

    @param path Actual original native checkpoint after stop construction.
    @return Native pickup station ID, derived without guessing a pool number.
    """
    stations = TABLE(path, b"STNN")
    station, = [key for key, row in stations.items() if row.get("normal[0].base[0].xy") == 1316]
    assert station != 15 and stations[station]["normal[0].base[0].owner"] == 0
    assert stations[station]["normal[0].base[0].facilities"] == 2
    return station


def execute(configuration, stage, receipt, recover_run=None):
    """Execute one ordered paid road, stop, order or bounded first-work boundary.

    @param configuration Independently linked focused native application.
    @param stage Explicit road/stops/orders/work boundary, never a replay.
    @param receipt Exact preceding successful native original receipt.
    @param recover_run Successful invocation to inspect without repeating payment or work.
    @return Real invoices, derived pickup identity, preserved old owners and measured cash.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    expected = dict(road=BASE["PHASE"], stops=PHASES["road"], orders=PHASES["stops"], work=PHASES["orders"])
    assert previous["complete"] and previous["phase"] == expected[stage]
    root = receipt if stage == "road" else ROOT / previous["operation"]["feeder_admission"]["path"]
    if stage != "road":
        assert identity(root) == previous["operation"]["feeder_admission"]
    proposal = json.loads(root.read_text(encoding="utf-8"))["operation"]
    assert proposal["farm_feeder_admitted"]
    funded, = [row for row in proposal["rows"]["admission"] if row["funded"]]
    assert funded["invoice"] == 3843 and funded["target"] == 56555 <= funded["cash"] == 60955
    files = BASE["BASE"]["BASE"]["BASE"]["FREIGHT"]["caller_files"]() + [Path(__file__),
        DIRECTORY / "surface_entry_farm_feeder_checks-1.py", DIRECTORY / "surface_entry_farm_road_work_checks-1.py",
        ROOT / "src/tests/surface_entry_farm_feeder_work.h", ROOT / "src/tests/surface_entry_farm_feeder.h",
        ROOT / "src/tests/surface_entry_farm_road_work.h", ROOT / "src/tests/surface_entry_farm_rebalance.h",
        ROOT / "src/tests/surface_entry_road_shortcut.h", ROOT / "src/tests/surface_entry_module_capital.h",
        ROOT / "src/tests/surface_entry_income_capitalization.h", ROOT / "src/station_cmd.cpp",
        ROOT / "src/road_cmd.cpp", ROOT / "src/order_cmd.cpp", ROOT / "src/economy.cpp"]
    operation = native(configuration, PHASES[stage], receipt, observe,
        dict(roads="FARM_FEEDER_ROAD_PAID", stops="FARM_FEEDER_STOPS_PAID", ordered="FARM_FEEDER_ORDERED",
             service="FARM_FEEDER_SERVICE", admission="FARM_ROAD_WORK_ADMISSION", invoices="INVOICE",
             earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"), files, recover_run=recover_run, chemical_limit=2000)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert len(rows["admission"]) == 1 and all(row["funded"] and row["target"] <= first["company"]["cash"] for row in rows["admission"])
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    # Every construction/order boundary spends only its literal invoice and
    # preserves physical time, all cargo and all unrelated vehicles/orders.
    if stage in ("road", "stops", "orders"):
        assert elapsed == 0 and chunks(before)[1][b"CAPA"] == chunks(output)[1][b"CAPA"]
        a, b = [TABLE(path, b"VEHS") for path in (before, output)]
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if stage != "orders" or key != 62)
        dates = [TABLE(path, b"DATE")[0] for path in (before, output)]
        assert {k:v for k,v in dates[0].items() if not k.startswith("random_state")} == {k:v for k,v in dates[1].items() if not k.startswith("random_state")}
        invoice = sum(row["cost"] for row in rows["invoices"])
        assert invoice == dict(road=2509, stops=1334, orders=0)[stage]
        assert first["company"]["cash"] - last["company"]["cash"] == invoice
        lists = [TABLE(path, b"ORDL") for path in (before, output)]
        if stage == "orders":
            excluded = {a[62][ROAD + "orders"] - 1, b[62][ROAD + "orders"] - 1}
            assert {k:v for k,v in lists[0].items() if k not in excluded} == {k:v for k,v in lists[1].items() if k not in excluded}
            pickup = pickup_identity(output)
            order = lists[1][b[62][ROAD + "orders"] - 1]
            assert order["orders.count"] == 2
            assert [order[f"orders[{i}].dest"] for i in (0, 1)] == [pickup, 15]
            assert [order[f"orders[{i}].flags"] for i in (0, 1)] == [4, 66]
            assert last["farm_road"]["held"] == 0 and last["farm_road"]["vehicle"] == 62
        else:
            assert lists[0] == lists[1]
            row, = rows["roads" if stage == "road" else "stops"]
            assert row["invoice"] == invoice and row["elapsed"] == 0
            if stage == "stops":
                assert row["pickup"] == pickup_identity(output)
    else:
        service, = rows["service"]
        earned, = rows["earned"]
        assert 0 < elapsed == earned["earned"] == service["ticks"] <= 2000
        assert earned["reached"] or elapsed == 2000
        assert service["pickup"] == pickup_identity(output) and service["vehicle"] == last["farm_road"]["vehicle"] == 62
        assert service["held"] == last["farm_road"]["held"] and not service["feeder_credit_is_cash"] and not service["final_payment_proven"]
        assert not service["transferred"] or service["picked_up"]
        assert service["partial"] == (not service["transferred"])
        assert service["restored"] == (last["company"]["cash"] >= 100000) and last["company"]["cash"] >= 40000
        delta = last["farm_road"]["profit256"] - first["farm_road"]["profit256"]
        if first["farm_road"]["previous_profit256"] != last["farm_road"]["previous_profit256"]:
            delta += last["farm_road"]["previous_profit256"]
        assert service["profit_delta_256"] == delta
        expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
        assert sum(expenses.values()) == first["company"]["cash"] - last["company"]["cash"]
        operation["measured_cash_flow"] = dict(ticks=elapsed, actual_fares=-expenses[7]-expenses[8], interest=expenses[11],
            other_operating_cost=sum(v for k,v in expenses.items() if k not in (7,8,11)),
            cash_delta=last["company"]["cash"]-first["company"]["cash"], virtual_truck_profit_256=delta,
            first_transfer=service["transferred"], feeder_credit_is_cash=False, annual_projection=False)
    assert BASE["BASE"]["strict_default_rejects"](output)
    operation["feeder_admission"] = identity(root)
    return operation


def main():
    """Publish one immutable actual short-feeder payment/order/work boundary."""
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
