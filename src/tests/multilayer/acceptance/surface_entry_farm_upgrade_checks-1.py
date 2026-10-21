"""Buy only the admitted actual farm wagon and measure native service.

@file surface_entry_farm_upgrade_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGF.
Original32wagons retain identity/coupling. One new31LVST25tail is recognized only
after native purchase; all launch stock and actual Goods trade stay separate.
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
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_farm_capacity_checks-1.py"))
PHASES = {name: "staged-chemical-capital-farm-" + name for name in ("recall", "return", "pay", "service")}


def observe(path):
    """@param path Actual original upgrade checkpoint. @return Sole original supply/trade owners and only the real optional one-wagon capacity."""
    result = observe_propellant_delivery(path, allow_capital_service=True, allow_goods_income=True, allow_farm_capacity=True)
    assert result["components_at_center"] and result["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    return result


def strict_default_rejects(path):
    """@param path Actual native extra-wagon original. @return Whether the preceding exact35body Goods trade contract rejects the added body."""
    try:
        BASE["observe"](path)
    except AssertionError:
        return True
    return False


def execute(configuration, phase, receipt, recover_run=None, limit=3000):
    """Execute one native empty recall, return, paid coupling or bounded useful service.

    @param configuration Independently linked focused native application.
    @param phase Explicit ordered actual boundary, never a replay.
    @param receipt Exact immediately preceding original native receipt.
    @param recover_run Successful invocation to inspect without repeating its command/work.
    @param limit Fresh actual return/service cap in1..3000.
    @return Native invoices/IDs, physical cargo, honest clocks and real financial outcome.
    """
    assert 0 < limit <= 3000
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    expected = {"recall": "staged-chemical-capital-farm-query", "return": PHASES["recall"], "pay": PHASES["return"], "service": PHASES["pay"]}
    assert previous["complete"] and previous["phase"] == expected[phase]
    if phase == "recall":
        root = receipt
    else:
        reference = previous["operation"]["upgrade_admission"]
        root = ROOT / reference["path"]
        assert identity(root) == reference
    proposal = json.loads(root.read_text(encoding="utf-8"))["operation"]
    whole, = [row for row in proposal["rows"]["admission"] if row["funded"]]
    assert whole["target"] == 78897 <= whole["cash"] == 85710 and whole["invoice"] == 1125 and whole["ticks"] == 6000
    if phase == "pay":
        returned, = previous["operation"]["rows"]["returned"]
        assert returned["returned"]
    files = BASE["BASE"]["FREIGHT"]["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_farm_capacity_checks-1.py",
        DIRECTORY / "surface_entry_productive_capital_checks-1.py", DIRECTORY / "surface_entry_goods_trade_checks-1.py",
        ROOT / "src/tests/surface_entry_farm_upgrade.h", ROOT / "src/tests/surface_entry_farm_capacity.h",
        ROOT / "src/tests/surface_entry_productive_capital.h", ROOT / "src/tests/surface_entry_goods_trade.h",
        ROOT / "src/tests/surface_entry_goods_income.h", ROOT / "src/tests/surface_entry_income_capitalization.h",
        ROOT / "src/tests/surface_entry_passenger_coverage.h", ROOT / "src/train_cmd.cpp", ROOT / "src/vehicle_cmd.cpp", ROOT / "src/economy.cpp"]
    operation = native(configuration, PHASES[phase], receipt, observe,
        dict(recall="FARM_RECALL", returned="FARM_RETURN", paid="FARM_PAID", service="FARM_SERVICE", admission="FARM_UPGRADE_ADMISSION",
             invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"),
        files, recover_run=recover_run, chemical_limit=limit)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    comparable = dict(first)
    if phase == "recall":
        comparable.pop("farm_capacity")
    assert json.loads(json.dumps(comparable)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    assert all(row["funded"] and row["target"] <= first["company"]["cash"] for row in rows["admission"])
    if phase in ("recall", "pay"):
        assert elapsed == 0
        assert chunks(before)[1][b"CAPA"] == chunks(output)[1][b"CAPA"] and chunks(before)[1][b"ORDL"] == chunks(output)[1][b"ORDL"]
        a, b = [TABLE(path, b"VEHS") for path in (before, output)]
        original_farm = {52, *range(12, 24)}
        # Native paid startup changes only the actual Farm consist. Every
        # other vehicle/packet/order and all non-RNG clock fields survive.
        assert all(a[key] == b[key] for key in a if key not in original_farm)
        dates = [TABLE(path, b"DATE")[0] for path in (before, output)]
        assert {key: value for key, value in dates[0].items() if not key.startswith("random_state")} == {key: value for key, value in dates[1].items() if not key.startswith("random_state")}
        invoice = sum(row["cost"] for row in rows["invoices"])
        assert invoice == (1125 if phase == "pay" else 0)
        assert first["company"]["cash"] - last["company"]["cash"] == invoice
        if phase == "pay":
            paid, = rows["paid"]
            body = last["farm_capacity"]["body"]
            assert body is not None and body["vehicle"] == paid["wagon"] and body["vehicle"] not in a
            assert b.keys() == a.keys() | {body["vehicle"]}
            assert last["farm_capacity"]["added"] == 1 and paid["length"] == 112 and paid["livestock"] == 175 and paid["grain"] == 180
            assert strict_default_rejects(output)
            admission, = rows["admission"]
            assert admission["restart"] > 0 and admission["unpaid"] == 1125
        else:
            assert a.keys() == b.keys() and last["farm_capacity"]["added"] == 0 and not strict_default_rejects(output)
    else:
        earned, = rows["earned"]
        progress, = rows["returned"] if phase == "return" else rows["service"]
        assert 0 <= elapsed == earned["earned"] == progress["ticks"] <= limit
        assert earned["reached"] or elapsed == limit
        expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
        assert sum(expenses.values()) == first["company"]["cash"] - last["company"]["cash"]
        operation["measured_cash_flow"] = dict(ticks=elapsed, actual_fares=-expenses[7] - expenses[8], interest=expenses[11],
            other_operating_cost=sum(value for key, value in expenses.items() if key not in (7, 8, 11)),
            cash_delta=last["company"]["cash"] - first["company"]["cash"], annual_projection=False)
        if phase == "return":
            assert last["farm_capacity"]["added"] == 0 and last["company"]["cash"] >= 41125
            if progress["returned"]:
                row = TABLE(output, b"VEHS")[52]
                prefix = "train[0].common[0]."
                assert row[prefix + "tile"] == 1638 and row[prefix + "vehstatus"] & 2
        else:
            assert elapsed > 0 and last["farm_capacity"]["added"] == 1 and last["company"]["cash"] >= 40000
            assert progress["restored"] == (last["company"]["cash"] >= 100000)
            assert progress["picked_up"] if earned["reached"] else True
            assert strict_default_rejects(output)
    operation["upgrade_admission"] = identity(root)
    return operation


def main():
    """Publish one immutable ordered actual single-wagon upgrade boundary."""
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
