"""Execute the expressly authorized single native thirty-thousand-reserve bridge.

@file surface_entry_reserve_bridge_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZQ.
One original native interval per build stops at posted Goods payment or2000ticks.
Existing forty-thousand defaults, prior scopes, sources and binaries stay exact.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity
from surface_regional_ledger import TABLE
from surface_salt_native import execute as native

HERE = Path(__file__).resolve().parent
RAIL = runpy.run_path(str(HERE / "surface_entry_goods_rail_delivery_checks-1.py"))
observe, rail_stock, TRAIN = [RAIL[name] for name in ("observe", "rail_stock", "TRAIN")]
AUTHORIZATION = ROOT / "logs/surface-entry-reserve-bridge-authorization.json"
PHASE = "staged-chemical-capital-joint-settlement-work"


def execute(configuration, recover_run=None):
    """Earn only the user's one original whole-bill-funded native opportunity.

    @param configuration Independently linked original RelWithDebInfo or Debug supply application.
    @param recover_run Exact already-successful native invocation to inspect without repeating work.
    @return Current native operation with whole actual fare, packet and expense reconciliation.
    @throws AssertionError On altered authority, duplicate execution, changed originals or excessive work.
    """
    authorization = json.loads(AUTHORIZATION.read_text(encoding="utf-8"))
    assert authorization["complete"] and authorization["user_reply"] == "Authorize the single capped attempt"
    assert authorization["maximum_ticks"] == 2000 and authorization["maximum_windows_per_configuration"] == 1
    assert authorization["temporary_reserve"] == 30000 and not authorization["second_window_authorized"]
    assert identity(ROOT / authorization["proposal"]["path"]) == authorization["proposal"]
    receipt = ROOT / ("logs/surface-entry-joint-cycle-reserve-005-" + configuration + ".json")
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    prior = previous["operation"]
    assert prior["working_scope"]["ended"] and prior["working_scope"]["end_reason"] == "funding_refused"
    assert prior["final"]["company"]["cash"] == 47839 and previous["configuration"] == configuration
    # The accepted complete original bill funds this one newly authorized scope.
    # No projected fare, lower partial bill or reopened old consumer can admit it.
    quote = next(row for row in prior["rows"]["admission"] if row["ticks"] == 2000)
    assert quote["operating"] == 12712 and quote["reserve"] == 40000 and not quote["funded"]
    assert quote["operating"] + 30000 == 42712 <= quote["cash"] == 47839
    graphics = ROOT / "logs/proofs/mlm/native-art/space/launch-presentation-v1/verification.json"
    assert json.loads(graphics.read_text(encoding="utf-8"))["complete"]
    current_sources = []
    dated_render_inputs = []
    # The intervening qualified graphics append legitimately changed only these
    # dated renderer inputs. This run executes the EXACT old financial binary;
    # every actual financial source/helper still matches its original receipt.
    # Keep historical hashes intact instead of rebasing their source records.
    for reference in prior["sources"]:
        current = identity(ROOT / reference["path"])
        if current != reference:
            assert reference["path"] in ("src/vehicle.cpp", "src/station_cmd.cpp", "src/space_vehicle.cpp")
            dated_render_inputs.append(dict(historical=reference, current=current,
                                            qualified_graphics=identity(graphics)))
        else:
            current_sources.append(ROOT / reference["path"])
    assert identity(ROOT / prior["application"]["path"]) == prior["application"]
    source = ROOT / prior["output"]["path"]
    assert identity(source) == prior["output"] and json.loads(json.dumps(observe(source))) == prior["final"]
    assert rail_stock(source)["quantity"] == 0
    assert not [row for row in TABLE(source, b"CAPY").values() if row["front"] == 50]
    attempt = ROOT / ("logs/surface-entry-reserve-bridge-" + configuration + "-attempt.json")
    if recover_run is None:
        # A validation failure after real execution must recover that exact run;
        # it never silently spends a second copy of the approved opportunity.
        assert not attempt.exists(), "existing attempt requires --recover-run, never another native execution"
        attempt.write_text(json.dumps(dict(authorization=identity(AUTHORIZATION), input=identity(receipt),
                                          maximum_native_invocations=1), indent=2) + "\n", encoding="utf-8")
    else:
        assert attempt.is_file()
    files = current_sources + [Path(__file__), AUTHORIZATION, graphics]
    operation = native(configuration, PHASE, receipt, observe,
        dict(admission="GOODS_RAIL_FEEDER_WORK_ADMISSION", selection="GOODS_RAIL_DELIVERY_SELECTION",
             service="GOODS_RAIL_DELIVERY_SERVICE", earned="EARNED", expenses="EXPENSE"),
        files, recover_run=recover_run, chemical_limit=2000)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == prior["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert identity(ROOT / prior["application"]["path"]) == prior["application"] == operation["application"]
    before, output = [ROOT / operation[name]["path"] for name in ("before", "output")]
    initial, final = [rail_stock(path) for path in (before, output)]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    selected, = rows["selection"]
    assert selected["found"] and selected["ticks"] == selected["ceiling"] == 2000
    assert not selected["settlement"] and selected["initial"] == initial["quantity"] == 0
    assert all(row["reserve"] == 30000 and row["unpaid"] == 0 and row["future_income_counted"] == 0
               and row["target"] == row["operating"] + 30000 for row in rows["admission"])
    assert rows["admission"][-1]["operating"] == 12712 and rows["admission"][-1]["funded"]
    service, = rows["service"]
    earned, = rows["earned"]
    assert 0 < elapsed == service["ticks"] == earned["earned"] <= 2000
    assert initial["quantity"] + service["boarded"] == service["accepted"] + final["quantity"]
    assert service["aboard"] == final["quantity"] and service["share"] == final["share"]
    assert service["stop_on_payment"] and not service["cohort_only_fare_claimed"] and not service["feeder_credit_is_cash"]
    posted = service["payment_posted"]
    if posted:
        # Only completed original unloading can close the bridge as paid.
        # Native pending share/acceptance alone never becomes company income.
        assert service["accepted"] > 0 and service["observed_whole_unload_fare"] > 0
        assert final["quantity"] == 0 and not service["payment_pending"]
    else:
        assert elapsed == 2000 or last["company"]["cash"] >= 100000
    expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
    assert sum(expenses.values()) == first["company"]["cash"] - last["company"]["cash"]
    assert last["company"]["cash"] >= 30000
    for reference in operation["sources"]:
        assert identity(ROOT / reference["path"]) == reference
    operation.update(authorization=identity(AUTHORIZATION), paid_root_receipt=identity(receipt),
        dated_render_inputs=dated_render_inputs,
        startup_reserve=30000, default_reserve_unchanged=40000, rail_payment_posted=posted,
        cumulative_rail_accepted=service["accepted"],
        measured_cash_flow=dict(ticks=elapsed, actual_fares=-expenses[7]-expenses[8], interest=expenses[11],
            other_operating_cost=sum(value for key, value in expenses.items() if key not in (7, 8, 11)),
            cash_delta=last["company"]["cash"]-first["company"]["cash"],
            whole_rail_unload_fare=service["observed_whole_unload_fare"]),
        bridge_scope=dict(kind="authorized_single_reserve_bridge", window=1, maximum_windows=1,
            tick_cap=2000, earned_ticks=elapsed, ended=True,
            end_reason="posted_payment" if posted else "cash_goal" if last["company"]["cash"] >= 100000 else "tick_cap",
            another_window_authorized=False))
    return operation


def main():
    """Publish one explicit original bridge result, refusing duplicate native attempts."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--recover-run")
    args = parser.parse_args()
    output = ROOT / ("logs/surface-entry-reserve-bridge-" + args.configuration + ".json")
    assert not output.exists()
    operation = execute(args.configuration, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
        phase=PHASE, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), operation["bridge_scope"], operation["measured_cash_flow"], flush=True)


if __name__ == "__main__":
    main()
