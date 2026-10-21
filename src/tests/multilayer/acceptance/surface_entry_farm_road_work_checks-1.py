"""Buy only the admitted native farm-road stops/truck and measure actual service.

@file surface_entry_farm_road_work_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGN.
Each paid boundary retains its own original checkpoint. Only the one real
named137Source7Livestock body extends the previous strict road-role contract.
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
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_farm_road_checks-1.py"))
PHASES = {stage: "staged-chemical-capital-farm-road-" + stage for stage in ("stops", "buy", "work")}


def observe(path):
    """@param path Actual native original farm-road checkpoint. @return Strict material/Space/old-role owners and only the actual optional137LVSbody."""
    result = observe_propellant_delivery(path, allow_capital_service=True, allow_goods_income=True,
        allow_farm_capacity=True, allow_farm_balance=True, allow_farm_road=True)
    assert result["components_at_center"] and result["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    return result


def strict_default_rejects(path):
    """@param path Actual native checkpoint. @return Whether the preceding exact road-role contract rejects the new named body."""
    try:
        BASE["observe"](path)
    except AssertionError:
        return True
    return False


def execute(configuration, stage, receipt, recover_run=None):
    """Execute one ordered native stop purchase, real truck purchase or bounded service.

    @param configuration Independently linked focused native application.
    @param stage Explicit stops/buy/work boundary, never a replay.
    @param receipt Exact immediately preceding actual native original receipt.
    @param recover_run Successful invocation to inspect without repeating commands/work.
    @return Actual gross invoices/ID, preserved physical owners and real service/cash endpoint.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    expected = dict(stops=BASE["PHASE"], buy=PHASES["stops"], work=PHASES["buy"])
    assert previous["complete"] and previous["phase"] == expected[stage]
    root = receipt if stage == "stops" else ROOT / previous["operation"]["farm_road_admission"]["path"]
    if stage != "stops":
        assert identity(root) == previous["operation"]["farm_road_admission"]
    proposal = json.loads(root.read_text(encoding="utf-8"))["operation"]
    assert proposal["farm_road_producer_admitted"]
    whole, = [row for row in proposal["rows"]["admission"] if row["funded"]]
    assert whole["target"] == 58752 <= whole["cash"] == 73667 and whole["gross"] == 6042 and whole["extra_running"] == 86
    files = BASE["BASE"]["FREIGHT"]["caller_files"]() + [Path(__file__), DIRECTORY / "surface_entry_farm_road_checks-1.py",
        DIRECTORY / "surface_entry_intertown_checks-1.py", DIRECTORY / "surface_entry_farm_rebalance_checks-1.py",
        ROOT / "src/tests/surface_entry_farm_road_work.h", ROOT / "src/tests/surface_entry_farm_road.h",
        ROOT / "src/tests/surface_entry_goods_income.h", ROOT / "src/tests/surface_entry_farm_upgrade.h",
        ROOT / "src/tests/surface_entry_module_capital.h", ROOT / "src/tests/surface_entry_income_capitalization.h",
        ROOT / "src/roadveh_cmd.cpp", ROOT / "src/station_cmd.cpp", ROOT / "src/economy.cpp"]
    operation = native(configuration, PHASES[stage], receipt, observe,
        dict(stops="FARM_ROAD_STOPS_PAID", bought="FARM_ROAD_TRUCK_PAID", service="FARM_ROAD_SERVICE",
             admission="FARM_ROAD_WORK_ADMISSION", invoices="INVOICE", earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"),
        files, recover_run=recover_run, chemical_limit=2000)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    comparable = dict(first)
    if stage == "stops":
        assert comparable.pop("farm_road") is None
    assert json.loads(json.dumps(comparable)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert all(row["funded"] and row["target"] <= first["company"]["cash"] for row in rows["admission"])
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    if stage in ("stops", "buy"):
        assert elapsed == 0 and chunks(before)[1][b"CAPA"] == chunks(output)[1][b"CAPA"]
        a, b = [TABLE(path, b"VEHS") for path in (before, output)]
        assert all(a[key] == b[key] for key in a)
        dates = [TABLE(path, b"DATE")[0] for path in (before, output)]
        assert {k: v for k, v in dates[0].items() if not k.startswith("random_state")} == {k: v for k, v in dates[1].items() if not k.startswith("random_state")}
        invoice = sum(row["cost"] for row in rows["invoices"])
        assert invoice == (300 if stage == "stops" else 5742)
        assert first["company"]["cash"] - last["company"]["cash"] == invoice
        lists = [TABLE(path, b"ORDL") for path in (before, output)]
        assert all(lists[0][key] == lists[1][key] for key in lists[0])
        if stage == "stops":
            assert a.keys() == b.keys() and lists[0] == lists[1]
            assert last["farm_road"] is None and last["company"]["cash"] == 73367
            assert not strict_default_rejects(output)
        else:
            body = last["farm_road"]
            assert body is not None and body["held"] == 0 and not body["stopped"]
            bought, = rows["bought"]
            assert body["vehicle"] == bought["vehicle"] and body["engine"] == 137 and body["capacity"] == 18
            assert b.keys() == a.keys() | {body["vehicle"]} and body["vehicle"] not in a
            assert last["company"]["cash"] == 67625 and strict_default_rejects(output)
    else:
        earned, = rows["earned"]
        service, = rows["service"]
        body = last["farm_road"]
        assert 0 < elapsed == earned["earned"] == service["ticks"] <= 2000
        assert earned["reached"] or elapsed == 2000
        assert service["vehicle"] == body["vehicle"] and service["held"] == body["held"]
        assert not service["delivered"] or service["picked_up"]
        assert service["restored"] == (last["company"]["cash"] >= 100000) and last["company"]["cash"] >= 40000
        delta = body["profit256"] - first["farm_road"]["profit256"]
        if body["previous_profit256"] != first["farm_road"]["previous_profit256"]:
            delta += body["previous_profit256"]
        assert service["profit_delta_256"] == delta
        expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
        assert sum(expenses.values()) == first["company"]["cash"] - last["company"]["cash"]
        operation["measured_cash_flow"] = dict(ticks=elapsed, actual_fares=-expenses[7] - expenses[8], interest=expenses[11],
            other_operating_cost=sum(v for k, v in expenses.items() if k not in (7, 8, 11)),
            cash_delta=last["company"]["cash"] - first["company"]["cash"], new_truck_profit_256=delta, annual_projection=False)
        assert strict_default_rejects(output)
    operation["farm_road_admission"] = identity(root)
    return operation


def main():
    """Publish one immutable actual farm-road boundary while preserving every successful prior payment."""
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
