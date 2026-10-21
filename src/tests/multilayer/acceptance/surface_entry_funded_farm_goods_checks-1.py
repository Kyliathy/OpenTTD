"""Earn genuine first Goods52payment only from freshly funded actual post-retirement cash.

@file surface_entry_funded_farm_goods_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEE.
Native packets, normal ticks, current costs and posted money retain ownership.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_original_evidence import verify
from surface_regional_ledger import TABLE
from surface_constructor_retirement_ledger import observe
from surface_funded_farm_goods_native import execute as native
from surface_evidence_identity_cache import IdentityCache

HERE = Path(__file__).resolve().parent
PREVIOUS = runpy.run_path(str(HERE / "surface_entry_constructor_retirement_checks-3.py"))
VERSIONS = PREVIOUS["VERSIONS"] + [ROOT / "logs/surface-entry-funded-farm-goods-before-edit.json"]
MAX_WINDOWS, MAX_TICKS = 8, 16000


def goods(state):
    """Read only actual Goods on the farm's genuinely paid two tails.

    @param state Direct same-save sole-owner material observation, never hypothetical stock.
    @return Actual quantity, native virtual feeder share and complete physical packet rows.
    """
    bodies = state["farm_balance"]["goods_wagons"]
    assert {body["vehicle"] for body in bodies} == {39, 60}
    return dict(quantity=sum(body["held"] for body in bodies),
        share=sum(packet["feeder_share"] for body in bodies for packet in body["packets"]), bodies=bodies)


def execute(configuration, phase, window=0, recover_run=None):
    """Run or recover one exact pure proposal or consecutive funded Goods interval.

    @param configuration Independently linked isolated native application.
    @param phase Pure query or actual normal work; pending payment is selected only from earlier real evidence.
    @param window Strict consecutive work ordinal; zero for the pure proposal.
    @param recover_run Exact successful invocation to inspect without repeating native work.
    @return Current/retired material custody, actual Goods/payment and whole-company cash flow.
    """
    assert phase in ("query", "work")
    total_before = boarded_before = accepted_before = 0
    if phase == "query":
        assert window == 0
        receipt = ROOT / f"logs/surface-entry-constructor-retirement-pay-{configuration}.json"
    else:
        assert 1 <= window <= MAX_WINDOWS
        name = "query" if window == 1 else f"work-{window-1:03d}"
        receipt = ROOT / f"logs/surface-entry-funded-farm-goods-{name}-{configuration}.json"
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration
    prior = previous["operation"]
    cache = IdentityCache()
    for reference in prior["sources"] + [prior["application"]]:
        verify(reference, VERSIONS, identity_reader=cache)
    source = ROOT / prior["output"]["path"]
    state = observe(source)
    assert identity(source) == prior["output"] and json.loads(json.dumps(state)) == prior["final"]
    if phase == "query":
        assert prior["actual_retirement_complete"] and prior["actual_cash"] == 71344 and prior["refund_banked"] == 29927
    else:
        scope = prior["goods_scope"]
        assert scope["window"] == window-1 and not scope["ended"]
        total_before = scope["total_earned_ticks"]
        boarded_before, accepted_before = prior["cumulative_boarded"], prior["cumulative_accepted"]
    ceiling = min(2000, MAX_TICKS-total_before)
    assert ceiling >= 64
    pending = [row for row in TABLE(source, b"CAPY").values() if row["front"] == 53]
    # Only an earlier actual accepted-Goods receipt can qualify continuation
    # of this52payment; old crops at13or other trains never substitute for it.
    settlement = bool(accepted_before > 0 and goods(state)["quantity"] == 0 and pending and pending[0]["route_profit"] > 0)
    native_phase = "payment" if phase == "work" and settlement else phase
    name = phase if phase != "work" else f"work-{window:03d}"
    attempt = ROOT / f"logs/surface-entry-funded-farm-goods-{name}-{configuration}-attempt.json"
    if recover_run is None:
        assert not attempt.exists(), "recover the precise original invocation instead of repeating earned work"
        attempt.write_text(json.dumps(dict(_documentation=__doc__, receipt=identity(receipt), reserve=40000,
            phase=phase, window=window, ceiling=ceiling), indent=2)+"\n", encoding="utf-8")
    else:
        assert json.loads(attempt.read_text(encoding="utf-8"))["receipt"] == identity(receipt)
    files = [ROOT / reference["path"] for reference in prior["sources"] if verify(reference, VERSIONS, identity_reader=cache)] + [
        Path(__file__), ROOT / "src/tests/surface_entry_funded_farm_goods.h"] + VERSIONS
    files = list(dict.fromkeys(files))
    operation = native(configuration, "funded-farm-goods-"+native_phase, receipt, observe,
        dict(plan="FUNDED_FARM_GOODS_PLAN", admission="GOODS_RAIL_FEEDER_WORK_ADMISSION", selection="FUNDED_FARM_GOODS_SELECTION",
            service="FUNDED_FARM_GOODS_SERVICE", earned="EARNED", expenses="EXPENSE"),
        files, recover_run=recover_run, work_limit=ceiling)
    cache.assert_unchanged()
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == prior["final"]
    before, output = [ROOT / operation[name]["path"] for name in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"]-TABLE(before, b"DATE")[0]["tick_counter"]
    delta = last["company"]["cash"]-first["company"]["cash"]
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["constructor_retired"] and last["constructor_retired"]
    assert last["retired_embodied_inputs"] == {"STEL": 4, "GOOD": 8, "PROP": 0}
    assert all(row["reserve"] == 40000 and row["unpaid"] == 0 and row["target"] == row["operating"]+40000
        and row["future_income_counted"] == 0 for row in rows["admission"])
    initial, final = goods(first), goods(last)
    boarded = accepted = 0
    refused = posted = False
    expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
    if phase == "query":
        assert elapsed == delta == 0 and first == last and chunks(before)[1] == chunks(output)[1]
        plan, = rows["plan"]
        assert plan["funded"] and plan["selected"] == 2000 and plan["initial"] == 0
        assert rows["admission"][-1]["funded"] and rows["admission"][-1]["target"] <= 71344
    else:
        selected, = rows["selection"]
        assert selected["ceiling"] == ceiling and selected["settlement"] == settlement
        refused = not selected["found"]
        if refused:
            assert elapsed == delta == 0 and not rows["earned"] and chunks(before)[1] == chunks(output)[1]
        else:
            service, = rows["service"]
            earned, = rows["earned"]
            assert 0 < elapsed == service["ticks"] == earned["earned"] <= selected["ticks"] <= ceiling
            assert rows["admission"][-1]["funded"] and rows["admission"][-1]["target"] <= first["company"]["cash"]
            boarded, accepted, posted = service["boarded"], service["accepted"], service["payment_posted"]
            assert initial["quantity"]+boarded == accepted+final["quantity"]
            assert service["settlement"] == settlement and service["cash"] == last["company"]["cash"]
            assert not service["cohort_only_fare_claimed"] and not service["feeder_credit_is_cash"]
            if posted:
                assert accepted_before+accepted > 0 and final["quantity"] == 0 and service["observed_whole_unload_fare"] > 0
                assert not service["payment_pending"]
        assert sum(expenses.values()) == -delta
    assert last["company"]["cash"] >= 40000
    total = total_before+elapsed
    reason = ("goods_payment_posted" if posted else "working_floor" if last["company"]["cash"] >= 100000 else
              "funding_refused" if refused else "tick_cap" if MAX_TICKS-total < 64 else "window_cap" if window == MAX_WINDOWS else None)
    operation.update(earned_ticks=elapsed, startup_reserve=40000, cumulative_boarded=boarded_before+boarded,
        cumulative_accepted=accepted_before+accepted, actual_goods=final, payment_posted=posted,
        historical_versions=[identity(path) for path in VERSIONS],
        measured_cash_flow=dict(ticks=elapsed, actual_fares=-expenses.get(7, 0)-expenses.get(8, 0), interest=expenses.get(11, 0),
            other_operating_cost=sum(value for kind, value in expenses.items() if kind not in (7, 8, 11)), cash_delta=delta),
        goods_scope=dict(window=window, total_earned_ticks=total, maximum_windows=MAX_WINDOWS,
            tick_cap=MAX_TICKS, ended=reason is not None, end_reason=reason))
    return operation


def main():
    """Retain exactly one original proposal or real normal-work endpoint."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("query", "work"), required=True)
    parser.add_argument("--window", type=int, default=0)
    parser.add_argument("--recover-run")
    args = parser.parse_args()
    name = args.phase if args.phase != "work" else f"work-{args.window:03d}"
    output = ROOT / f"logs/surface-entry-funded-farm-goods-{name}-{args.configuration}.json"
    assert not output.exists()
    operation = execute(args.configuration, args.phase, args.window, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
        phase="funded-farm-goods-"+args.phase, operation=operation), indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), operation["goods_scope"], operation["measured_cash_flow"], flush=True)


if __name__ == "__main__":
    main()
