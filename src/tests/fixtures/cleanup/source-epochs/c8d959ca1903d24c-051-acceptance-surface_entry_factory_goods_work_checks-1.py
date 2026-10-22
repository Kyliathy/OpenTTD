"""Apply actual zero-price direct Goods orders and measure only funded native service.

@file surface_entry_factory_goods_work_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGU.
Original3/25retain cargo and positions. Company books, native deliveries and
the preserved first22rail cohort remain distinct authoritative observations.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

DIRECTORY = Path(__file__).resolve().parent
BASE = runpy.run_path(str(DIRECTORY / "surface_entry_factory_goods_checks-1.py"))
observe = BASE["observe"]
PHASES = {stage: "staged-chemical-capital-factory-goods-" + stage for stage in ("orders", "work")}
ROAD = "roadveh[0].common[0]."


def execute(configuration, stage, receipt, recover_run=None):
    """Execute one zero-tick order boundary or freshly funded first2000tick service interval.

    @param configuration Independently linked focused native application.
    @param stage Explicit orders/work boundary, never a replay.
    @param receipt Exact preceding original native admission or order receipt.
    @param recover_run Successful invocation to inspect without repeating orders or work.
    @return Original cargo/owners, exact zero invoice and actual pickup/delivery/cash outcomes.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["phase"] == (BASE["MIXED_PHASE"] if stage == "orders" else PHASES["orders"])
    root = receipt if stage == "orders" else ROOT / previous["operation"]["factory_goods_admission"]["path"]
    if stage == "work":
        assert identity(root) == previous["operation"]["factory_goods_admission"]
    assert json.loads(root.read_text(encoding="utf-8"))["operation"]["factory_goods_admitted"]
    files = [ROOT / row["path"] for row in previous["operation"]["sources"]] + [Path(__file__), ROOT / "src/tests/surface_entry_factory_goods_work.h"]
    operation = native(configuration, PHASES[stage], receipt, observe,
        dict(ordered="FACTORY_GOODS_ORDERED", route="FACTORY_GOODS_ROUTE_PAID", service="FACTORY_GOODS_SERVICE",
             vehicles="FACTORY_GOODS_SERVICE_VEHICLE", admission="FARM_ROAD_WORK_ADMISSION", invoices="INVOICE",
             earned="EARNED", expenses="EXPENSE", delivered="DELIVERED"), files, recover_run=recover_run, chemical_limit=2000)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == previous["operation"]["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    admission, = rows["admission"]
    assert admission["funded"] and admission["target"] == admission["operating"] + 40000 <= first["company"]["cash"]
    assert admission["ticks"] == 2000 and admission["unpaid"] == admission["extra_running"] == 0
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    a, b = [TABLE(path, b"VEHS") for path in (before, output)]
    if stage == "orders":
        assert elapsed == 0 and chunks(before)[1][b"CAPA"] == chunks(output)[1][b"CAPA"]
        assert TABLE(before,b"DATE") == TABLE(output,b"DATE")
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key not in (3,25))
        lists = [TABLE(path,b"ORDL") for path in (before,output)]
        changed = {body[id][ROAD+"orders"]-1 for body in (a,b) for id in (3,25)}
        assert {k:v for k,v in lists[0].items() if k not in changed} == {k:v for k,v in lists[1].items() if k not in changed}
        for id in (3,25):
            order = lists[1][b[id][ROAD+"orders"]-1]
            assert order["orders.count"] == 2 and [order[f"orders[{i}].dest"] for i in (0,1)] == [13,20]
            assert [order[f"orders[{i}].flags"] for i in (0,1)] == [4,64]
            # Native raw type retains FarEnd position32alongside non-stop64
            # and GoToStation1; masking those fields separately avoids
            # mistaking a correct stopping-position policy for a new order.
            assert all(order[f"orders[{i}].type"] & 15 == 1 and order[f"orders[{i}].type"] & 192 == 64
                       and order[f"orders[{i}].type"] & 48 == 32 for i in (0,1))
            assert a[id][ROAD+"tile"] == b[id][ROAD+"tile"] and a[id][ROAD+"cargo.packets"] == b[id][ROAD+"cargo.packets"]
        assert [(r["vehicle"],r["held"],r["first_destination"]) for r in rows["ordered"]] == [(3,0,13),(25,18,20)]
        assert first["company"]["cash"] == last["company"]["cash"] == 54793
        assert all(r["cost"] == 0 for r in rows["invoices"])
    else:
        service, = rows["service"]
        earned, = rows["earned"]
        assert 0 < elapsed == service["ticks"] == earned["earned"] <= 2000
        assert earned["reached"] or elapsed == 2000
        assert service["restored"] == (last["company"]["cash"] >= 100000) and last["company"]["cash"] >= 40000
        assert service["rail_boarded"] == service["rail_delivered"] + service["rail_aboard"]
        assert service["picked_up"] == sum(r["picked_up"] for r in rows["vehicles"])
        assert service["delivered"] == sum(r["delivered"] for r in rows["vehicles"])
        # Real body profit is separate from whole-company fares. Each native
        # annual rollover is reconciled before interpreting the actual delta.
        for row in rows["vehicles"]:
            id = row["vehicle"]
            delta = b[id][ROAD+"profit_this_year"]-a[id][ROAD+"profit_this_year"]
            if a[id][ROAD+"profit_last_year"] != b[id][ROAD+"profit_last_year"]:
                delta += b[id][ROAD+"profit_last_year"]
            assert row["profit_delta_256"] == delta
            assert row["held"] == sum(TABLE(output,b"CAPA")[ref-1]["count"] for ref in b[id][ROAD+"cargo.packets"])
        expenses = {r["type"]:r["amount"] for r in rows["expenses"]}
        assert sum(expenses.values()) == first["company"]["cash"]-last["company"]["cash"]
        operation["measured_cash_flow"] = dict(ticks=elapsed, actual_fares=-expenses[7]-expenses[8], interest=expenses[11],
            other_operating_cost=sum(v for k,v in expenses.items() if k not in (7,8,11)), cash_delta=last["company"]["cash"]-first["company"]["cash"],
            picked_up=service["picked_up"], delivered=service["delivered"], rail_boarded=service["rail_boarded"],
            rail_delivered=service["rail_delivered"], rail_aboard=service["rail_aboard"], annual_projection=False)
    operation["factory_goods_admission"] = identity(root)
    return operation


def main():
    """Publish one immutable actual direct Goods order or first-work boundary."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--stage", choices=tuple(PHASES), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration,args.stage,ROOT / args.receipt,args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__,complete=True,configuration=args.configuration,
                                     phase=PHASES[args.stage],operation=operation),indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
