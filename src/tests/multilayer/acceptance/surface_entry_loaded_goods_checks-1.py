"""Deliver actual loaded Goods only under the original forty-thousand reserve.

@file surface_entry_loaded_goods_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZR.
The current native calendar and complete bill, not a projected fare or a prior
month's refusal, determine whether this single short arrival interval is funded.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

HERE = Path(__file__).resolve().parent
RAIL = runpy.run_path(str(HERE / "surface_entry_goods_rail_delivery_checks-1.py"))
observe, rail_stock = [RAIL[name] for name in ("observe", "rail_stock")]
PHASE = "staged-chemical-capital-joint-goods-rail-delivery"


def execute(configuration, recover_run=None):
    """Price and earn one default-reserve arrival interval from the paid loaded train.

    @param configuration Original independently linked RelWithDebInfo or Debug native application.
    @param recover_run Exact already-executed native invocation to inspect without repeating work.
    @return Whole native arrival/payment/expense evidence, including any pure refusal.
    @throws AssertionError On unqualified source, changed default/authority or duplicated execution.
    """
    receipt = ROOT / ("logs/surface-entry-reserve-bridge-" + configuration + ".json")
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    prior = previous["operation"]
    assert prior["bridge_scope"]["ended"] and not prior["bridge_scope"]["another_window_authorized"]
    assert prior["final"]["company"]["cash"] == 46491
    assert all(identity(ROOT / ref["path"]) == ref for ref in prior["sources"])
    assert identity(ROOT / prior["application"]["path"]) == prior["application"]
    source = ROOT / prior["output"]["path"]
    assert identity(source) == prior["output"] and rail_stock(source)["quantity"] == 162
    attempt = ROOT / ("logs/surface-entry-loaded-goods-" + configuration + "-attempt.json")
    if recover_run is None:
        # Recovery must inspect the exact first native run. The original paid
        # world cannot silently fund another interval after a recorder failure.
        assert not attempt.exists(), "recover the recorded native invocation instead of repeating work"
        attempt.write_text(json.dumps(dict(source=identity(receipt), reserve=40000, ceiling=500,
                                          maximum_native_invocations=1), indent=2) + "\n", encoding="utf-8")
    else:
        assert attempt.is_file()
    files = [ROOT / ref["path"] for ref in prior["sources"]] + [Path(__file__)]
    operation = native(configuration, PHASE, receipt, observe,
        dict(admission="GOODS_RAIL_FEEDER_WORK_ADMISSION", selection="GOODS_RAIL_DELIVERY_SELECTION",
             service="GOODS_RAIL_DELIVERY_SERVICE", earned="EARNED", expenses="EXPENSE"),
        files, recover_run=recover_run, chemical_limit=500)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == prior["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert operation["application"] == prior["application"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    selected, = rows["selection"]
    assert selected["ceiling"] == 500 and not selected["settlement"] and selected["initial"] == 162
    assert all(row["reserve"] == 40000 and row["future_income_counted"] == 0 and row["unpaid"] == 0
               and row["target"] == row["operating"] + 40000 for row in rows["admission"])
    posted = False
    service = None
    if selected["found"]:
        service, = rows["service"]
        earned, = rows["earned"]
        assert rows["admission"][-1]["funded"] and rows["admission"][-1]["target"] <= first["company"]["cash"]
        assert 0 < elapsed == service["ticks"] == earned["earned"] <= selected["ticks"] <= 500
        final_stock = rail_stock(output)
        assert 162 + service["boarded"] == service["accepted"] + final_stock["quantity"]
        assert service["stop_on_payment"] and not service["cohort_only_fare_claimed"]
        posted = service["payment_posted"]
        if posted:
            # Only actual native unloading and posted CargoPayment settles this
            # arrival. Unbanked feeder shares never satisfy the cash target.
            assert service["accepted"] > 0 and service["observed_whole_unload_fare"] > 0
            assert not service["payment_pending"] and final_stock["quantity"] == 0
    else:
        assert elapsed == 0 and not rows["service"] and not rows["earned"]
        assert chunks(before)[1] == chunks(output)[1]
    expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
    assert sum(expenses.values()) == first["company"]["cash"] - last["company"]["cash"]
    assert last["company"]["cash"] >= 40000
    operation.update(startup_reserve=40000, rail_payment_posted=posted,
        cumulative_rail_accepted=service["accepted"] if service else 0,
        measured_cash_flow=dict(ticks=elapsed, actual_fares=-expenses.get(7,0)-expenses.get(8,0),
            interest=expenses.get(11,0), other_operating_cost=sum(value for key,value in expenses.items() if key not in (7,8,11)),
            cash_delta=last["company"]["cash"]-first["company"]["cash"],
            whole_rail_unload_fare=service["observed_whole_unload_fare"] if service else 0),
        arrival_scope=dict(window=1, maximum_windows=1, tick_cap=500, earned_ticks=elapsed, ended=True,
            end_reason="posted_payment" if posted else "tick_cap" if selected["found"] else "funding_refused"))
    return operation


def main():
    """Retain one actual freshly funded arrival or unchanged default-reserve refusal."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--recover-run")
    args = parser.parse_args()
    output = ROOT / ("logs/surface-entry-loaded-goods-" + args.configuration + ".json")
    assert not output.exists()
    operation = execute(args.configuration, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__,complete=True,configuration=args.configuration,
                                     phase=PHASE,operation=operation),indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),operation["arrival_scope"],operation["measured_cash_flow"],flush=True)


if __name__ == "__main__":
    main()
