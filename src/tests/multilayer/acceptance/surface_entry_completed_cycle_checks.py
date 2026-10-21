"""Reconcile the real completed Goods cycle before admitting productive recovery.

@file surface_entry_completed_cycle_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZT.
Immutable original checkpoints and native expense/profit owners supply every
quantity. This diagnostic executes no game, command, work or reserve exception.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, identity
from surface_regional_ledger import TABLE

STAGES = (tuple(f"joint-cycle-reserve-{i:03d}" for i in range(1, 6)) +
          ("reserve-bridge", "loaded-goods", "loaded-arrival-001", "loaded-arrival-002",
           "arrival-settlement-001"))
FOUNDATIONS = ("joint-cycle-reserve", "reserve-bridge", "loaded-goods", "loaded-arrival",
               "arrival-settlement")


def reconcile(configuration):
    """Read one exact chronological cycle and reconcile its actual books and vehicles.

    @param configuration Independently linked original native build configuration.
    @return Immutable endpoints, real elapsed work, expense totals and native vehicle profit deltas.
    @throws AssertionError On a broken predecessor, changed artifact or inconsistent company books.
    """
    operations, receipts = [], []
    # Each native interval must consume the immediately preceding original,
    # including pure refusals. A later paid fare cannot retroactively fund it.
    for name in STAGES:
        receipt = ROOT / f"logs/surface-entry-{name}-{configuration}.json"
        value = json.loads(receipt.read_text(encoding="utf-8"))
        assert value["complete"] and value["configuration"] == configuration
        operation = value["operation"]
        assert all(identity(ROOT / operation[key]["path"]) == operation[key]
                   for key in ("source", "before", "output", "application", "terminal", "retained_harness"))
        if operations:
            assert operation["source"] == operations[-1]["output"]
            assert operation["initial"] == operations[-1]["final"]
        operations.append(operation)
        receipts.append(identity(receipt))
    first, last = operations[0], operations[-1]
    assert last["rail_payment_posted"] and last["settlement_scope"]["ended"]
    assert last["settlement_scope"]["end_reason"] == "posted_payment"
    assert first["initial"]["company"] == dict(cash=49257, debt=5000000)
    assert last["final"]["company"] == dict(cash=45080, debt=5000000)
    totals = dict(ticks=0, actual_fares=0, interest=0, other_operating_cost=0, cash_delta=0)
    # Native company expense rows, rather than unbanked packet shares or
    # selected-cohort fares, explain every actually earned interval's money.
    for operation in operations:
        expenses = {row["type"]: row["amount"] for row in operation["rows"]["expenses"]}
        before, after = [ROOT / operation[key]["path"] for key in ("before", "output")]
        elapsed = TABLE(after, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
        delta = operation["final"]["company"]["cash"]-operation["initial"]["company"]["cash"]
        assert sum(expenses.values()) == -delta
        assert elapsed == operation["measured_cash_flow"]["ticks"]
        totals["ticks"] += elapsed
        totals["cash_delta"] += delta
        totals["actual_fares"] -= expenses.get(7, 0)+expenses.get(8, 0)
        totals["interest"] += expenses.get(11, 0)
        totals["other_operating_cost"] += sum(amount for kind, amount in expenses.items()
                                               if kind not in (7, 8, 11))
    assert totals == dict(ticks=3578, actual_fares=20209, interest=16667,
                         other_operating_cost=7719, cash_delta=-4177)
    source, output = [ROOT / ref["path"] for ref in (first["source"], last["output"])]
    start, finish = [TABLE(path, b"VEHS") for path in (source, output)]
    assert start.keys() == finish.keys()
    vehicles = []
    # Inspect actual surviving native bodies. The saved year identity must
    # remain stable before subtracting the same vehicle's fixed-point profits;
    # no projected replacement, retirement or avoided charge enters this ledger.
    for vehicle_id, row in finish.items():
        old = start[vehicle_id]
        keys = [key for key in row if key.endswith("profit_this_year")]
        for key in keys:
            prefix = key[:-len("profit_this_year")]
            assert old[prefix+"profit_last_year"] == row[prefix+"profit_last_year"]
            delta = row[key]-old[key]
            if delta:
                vehicles.append(dict(vehicle=vehicle_id, type=row["type"],
                    engine=row[prefix+"engine_type"], actual_profit_delta_256=delta))
    constructor, = [row for row in vehicles if row["vehicle"] == 38]
    assert constructor["actual_profit_delta_256"] == -4835*256
    return dict(configuration=configuration, receipts=receipts, source=first["source"], output=last["output"],
        actual_company_cash_flow=totals, native_vehicle_profits=vehicles,
        constructor_actual_idle_expense=4835, future_income_counted=0, avoided_upkeep_counted=0,
        refund_banked=0, unchanged_cycle_continuation_admitted=False)


def main():
    """Retain matching original paid-cycle accounts without invoking a game or changing a save."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    foundations = [ROOT / f"logs/surface-entry-{name}-verification.json" for name in FOUNDATIONS]
    assert all(json.loads(path.read_text(encoding="utf-8"))["complete"] for path in foundations)
    configurations = [reconcile(configuration) for configuration in ("RelWithDebInfo", "Debug")]
    assert configurations[0]["actual_company_cash_flow"] == configurations[1]["actual_company_cash_flow"]
    assert configurations[0]["native_vehicle_profits"] == configurations[1]["native_vehicle_profits"]
    result = dict(_documentation=__doc__, complete=True, foundations=[identity(path) for path in foundations],
        source=identity(Path(__file__)), configurations=configurations, native_invocations=0, normal_ticks_executed=0,
        next_prerequisite="Pure native current productive-service demand, capacity and complete purchase/operating admission; preserve current originals, forty-thousand reserve and needed constructor.")
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), configurations[0]["actual_company_cash_flow"], flush=True)


if __name__ == "__main__":
    main()
