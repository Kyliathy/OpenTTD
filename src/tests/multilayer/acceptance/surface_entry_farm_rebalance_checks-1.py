"""Retain every original wagon while buying and measuring the admitted farm balance.

@file surface_entry_farm_rebalance_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGH.
Native commands alone recall, park the exact empty Grain reserve and purchase
three actual Livestock tails. No parked capacity supplies active service/cash.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_component_delivery_ledger import observe_propellant_delivery
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_farm_balance_checks-1.py"))
OLD = runpy.run_path(str(DIRECTORY / "surface_entry_farm_upgrade_checks-1.py"))
PHASES = {name: "staged-chemical-capital-farm-rebalance-" + name for name in ("recall", "return", "park", "pay", "service")}


def observe(path):
    """@param path Actual native balance original. @return Strict material owners and explicit active/reserve train inventory."""
    result = observe_propellant_delivery(path, allow_capital_service=True, allow_goods_income=True,
                                        allow_farm_capacity=True, allow_farm_balance=True)
    assert result["components_at_center"] and result["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    return result


def strict_default_rejects(path):
    """@param path Actual changed farm original. @return Whether the preceding one-wagon contract rejects the changed prefix/body count."""
    try:
        OLD["observe"](path)
    except AssertionError:
        return True
    return False


def execute(configuration, phase, receipt, recover_run=None, limit=3000):
    """Execute one ordered native retained-asset balance boundary.

    @param configuration Independently linked focused native application.
    @param phase Explicit recall/return/park/pay/service boundary, never a replay.
    @param receipt Exact immediately preceding original native receipt.
    @param recover_run Successful invocation to inspect without repeating commands/work.
    @param limit Fresh actual return/service cap in1..3000.
    @return Actual invoices, physical inventory, elapsed work and whole-company cash.
    """
    assert 0 < limit <= 3000
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    expected = dict(recall="staged-chemical-capital-farm-balance-query", **{b: PHASES[a] for a, b in
                    zip(("recall", "return", "park", "pay"), ("return", "park", "pay", "service"))})
    assert previous["complete"] and previous["phase"] == expected[phase]
    root = receipt if phase == "recall" else ROOT / previous["operation"]["balance_admission"]["path"]
    if phase != "recall":
        assert identity(root) == previous["operation"]["balance_admission"]
    whole, = [row for row in json.loads(root.read_text(encoding="utf-8"))["operation"]["rows"]["admission"] if row["funded"]]
    assert whole["target"] == 81129 <= whole["cash"] == 81662 and whole["invoice"] == 3375 and whole["ticks"] == 6000
    if phase == "park":
        returned, = previous["operation"]["rows"]["returned"]
        assert returned["returned"]
    files = BASE["FREIGHT"]["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_farm_balance_checks-1.py",
        DIRECTORY / "surface_entry_farm_upgrade_checks-1.py", ROOT / "src/tests/surface_entry_farm_rebalance.h",
        ROOT / "src/tests/surface_entry_farm_balance.h", ROOT / "src/tests/surface_entry_farm_upgrade.h",
        ROOT / "src/tests/surface_entry_farm_capacity.h", ROOT / "src/tests/surface_entry_module_capital.h",
        ROOT / "src/tests/surface_entry_income_capitalization.h", ROOT / "src/train_cmd.cpp", ROOT / "src/vehicle_cmd.cpp", ROOT / "src/economy.cpp"]
    operation = native(configuration, PHASES[phase], receipt, observe,
        dict(recall="BALANCE_RECALL", returned="BALANCE_RETURN", parked="BALANCE_PARK", paid="BALANCE_PAID",
             wagons="BALANCE_WAGON", use="BALANCE_USE", service="BALANCE_SERVICE", admission="FARM_UPGRADE_ADMISSION",
             invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"),
        files, recover_run=recover_run, chemical_limit=limit)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    comparable = dict(first)
    if phase == "recall":
        comparable.pop("farm_balance")
    assert json.loads(json.dumps(comparable)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert all(row["funded"] and row["target"] <= first["company"]["cash"] for row in rows["admission"])
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    balance = last["farm_balance"]
    assert balance["original_wagons_owned"] == 32
    if phase in ("recall", "park", "pay"):
        assert elapsed == 0
        assert chunks(before)[1][b"CAPA"] == chunks(output)[1][b"CAPA"] and chunks(before)[1][b"ORDL"] == chunks(output)[1][b"ORDL"]
        a, b = [TABLE(path, b"VEHS") for path in (before, output)]
        original_farm = {52, 50, *range(12, 24)}
        # Only this exact farm cohort may change during zero-time native
        # commands. All other vehicles and non-RNG clocks remain identical.
        assert all(a[key] == b[key] for key in a if key not in original_farm)
        dates = [TABLE(path, b"DATE")[0] for path in (before, output)]
        assert {k: v for k, v in dates[0].items() if not k.startswith("random_state")} == {k: v for k, v in dates[1].items() if not k.startswith("random_state")}
        invoice = sum(row["cost"] for row in rows["invoices"])
        assert invoice == (3375 if phase == "pay" else 0)
        assert first["company"]["cash"] - last["company"]["cash"] == invoice
        new_ids = {body["vehicle"] for body in balance["new_wagons"]}
        assert b.keys() == a.keys() | new_ids
        if phase == "pay":
            assert len(new_ids) == 3 and not new_ids & a.keys()
            assert {row["wagon"] for row in rows["wagons"]} == new_ids
            assert balance["state"] == "balanced" and balance["livestock_capacity"] == 250 and balance["active_length"] == 112
        elif phase == "park":
            assert balance["state"] == "parked" and balance["active_length"] == 88
        else:
            assert balance["state"] == "original"
    else:
        earned, = rows["earned"]
        progress, = rows["returned"] if phase == "return" else rows["service"]
        assert 0 <= elapsed == earned["earned"] == progress["ticks"] <= limit
        assert earned["reached"] or elapsed == limit
        expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
        assert sum(expenses.values()) == first["company"]["cash"] - last["company"]["cash"]
        operation["measured_cash_flow"] = dict(ticks=elapsed, actual_fares=-expenses[7] - expenses[8], interest=expenses[11],
            other_operating_cost=sum(v for k, v in expenses.items() if k not in (7, 8, 11)),
            cash_delta=last["company"]["cash"] - first["company"]["cash"], annual_projection=False)
        if phase == "return":
            assert balance["state"] == "original" and last["company"]["cash"] >= 43375
        else:
            assert balance["state"] == "balanced" and last["company"]["cash"] >= 40000
            assert progress["restored"] == (last["company"]["cash"] >= 100000)
            assert len(rows["use"]) == 3 and {row["wagon"] for row in rows["use"]} == {body["vehicle"] for body in balance["new_wagons"]}
            if earned["reached"]:
                assert all(row["picked_up"] and row["delivered"] for row in rows["use"])
    changed = phase in ("park", "pay", "service")
    assert strict_default_rejects(output) == changed
    if changed:
        assert "farm_capacity" not in last
        assert balance["parked"] == [18, 19, 20] and balance["grain_capacity"] == balance["parked_grain_capacity"] == 90
    operation["balance_admission"] = identity(root)
    return operation


def main():
    """Publish one immutable native balance boundary, preserving successful progress on recovery."""
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
