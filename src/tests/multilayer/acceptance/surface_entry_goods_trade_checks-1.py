"""Start only funded native Goods trade and measure actual first delivery.

@file surface_entry_goods_trade_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGB.
Two real idle reserves retain their identities. New Source11income packets stay
separate from fifty original Goods at center21; old defaults remain strict.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks
from surface_gateway_component_delivery_ledger import observe_propellant_delivery
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_goods_income_checks-1.py"))
PHASES = {name: "staged-chemical-capital-trade-" + name for name in ("start", "work")}


def observe(path):
    """@param path Actual original native trade world. @return Narrow real3/25trade custody and unchanged fiftyGoods/fuel/Core/eleven roles."""
    result = observe_propellant_delivery(path, allow_capital_service=True, allow_goods_income=True)
    assert result["components_at_center"] and result["delivery_stocks"][21] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert {row["vehicle"] for row in result["goods_income"].values()} == {3, 25}
    return result


def strict_default_rejects(path):
    """@param path Actual later reassigned original. @return Whether the historical completed-carrier contract rejects the new duty."""
    try:
        observe_propellant_delivery(path, allow_capital_service=True)
    except AssertionError:
        return True
    return False


def execute(configuration, phase, receipt, recover_run=None, limit=3000):
    """Execute only native paid startup or the funded first actual Goods-income interval.

    @param configuration Independently linked focused native application.
    @param phase Separate actual construction/order/start or bounded work boundary.
    @param receipt Exact selected original pure quote or completed startup.
    @param recover_run Successful invocation to inspect without repeating payment/work.
    @param limit Explicit actual native cap in1..3000, priced before advancing.
    @return Real invoices, sole cargo ownership, strict-default rejection and actual financial result.
    """
    assert 0 < limit <= 3000
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] == ("staged-chemical-capital-goods-query" if phase == "start" else PHASES["start"])
    files = BASE["BASE"]["BASE"]["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_goods_income_checks-1.py",
        ROOT / "src/tests/surface_entry_goods_trade.h", ROOT / "src/tests/surface_entry_goods_income.h",
        ROOT / "src/tests/surface_entry_income_capitalization.h", ROOT / "src/tests/surface_entry_passenger_coverage.h",
        ROOT / "src/economy.cpp", ROOT / "src/station_cmd.cpp", ROOT / "src/order_cmd.cpp", ROOT / "src/roadveh_cmd.cpp"]
    operation = native(configuration, PHASES[phase], receipt, observe,
        dict(paid="TRADE_PAID", admission="TRADE_ADMISSION", progress="TRADE_PROGRESS", trucks="TRADE_VEHICLE",
             fixed="INCOME_FIXED_PRICES", invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"),
        files, recover_run=recover_run, chemical_limit=limit)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    comparable = dict(first)
    if phase == "start":
        comparable.pop("goods_income")
    assert json.loads(json.dumps(comparable)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    if phase == "start":
        assert not strict_default_rejects(before) and strict_default_rejects(output)
        paid, = rows["paid"]
        assert paid["invoice"] == sum(row["cost"] for row in rows["invoices"]) == 150
        assert paid["vehicles"] == 2 and paid["capacity"] == 36 and paid["added_vehicles"] == paid["credit"] == 0
        assert elapsed == 0 and first["company"]["cash"] == 96397 and last["company"]["cash"] == 96247
        assert not last["component_carriers_returned"] and all(not row["stopped"] and row["held"] == 0 for row in last["goods_income"].values())
        # Only the two empty reserves change orders/status. All other actual
        # vehicles, packets and simulation clocks survive the paid startup.
        a, b = [TABLE(path, b"VEHS") for path in (before, output)]
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key not in (3, 25))
        assert chunks(before)[1][b"CAPA"] == chunks(output)[1][b"CAPA"]
        dates = [TABLE(path, b"DATE")[0] for path in (before, output)]
        assert {key: value for key, value in dates[0].items() if not key.startswith("random_state")} == {key: value for key, value in dates[1].items() if not key.startswith("random_state")}
    else:
        admission, = rows["admission"]
        progress, = rows["progress"]
        earned, = rows["earned"]
        assert admission["funded"] and admission["target"] <= first["company"]["cash"]
        assert admission["ticks"] == limit and admission["reserve"] == 40000
        assert admission["purchases"] == admission["future_income_counted"] == admission["refunds_counted"] == admission["credit"] == 0
        assert 0 < elapsed == earned["earned"] == progress["ticks"] <= limit
        assert earned["reached"] or elapsed == limit
        assert progress["restored"] == (last["company"]["cash"] >= 100000)
        assert last["company"]["cash"] >= 40000 and not progress["module_capital_complete"]
        assert {row["vehicle"] for row in rows["trucks"]} == {3, 25}
        expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
        assert sum(expenses.values()) == first["company"]["cash"] - last["company"]["cash"]
        operation["measured_cash_flow"] = dict(ticks=elapsed, actual_fares=-expenses[7] - expenses[8], interest=expenses[11],
            other_operating_cost=sum(value for key, value in expenses.items() if key not in (7, 8, 11)),
            cash_delta=last["company"]["cash"] - first["company"]["cash"], annual_projection=False)
    assert strict_default_rejects(output)
    operation["strict_default_rejected"] = True
    return operation


def main():
    """Publish one immutable actual Goods trade startup or first bounded work receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=tuple(PHASES), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--limit", type=int, default=3000)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration, args.phase, ROOT / args.receipt, args.recover_run, args.limit)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     phase=PHASES[args.phase], operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
