"""Recover actual working cash through freshly funded existing paid services.

@file surface_entry_working_cash_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEEF.
Only native fares and actual costs move money; retired components remain history.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_original_evidence import verify
from surface_regional_ledger import TABLE
from surface_constructor_retirement_ledger import observe
from surface_working_cash_native import execute as native
from surface_evidence_identity_cache import IdentityCache

HERE = Path(__file__).resolve().parent
PREVIOUS = runpy.run_path(str(HERE / "surface_entry_funded_farm_goods_checks-3.py"))
VERSIONS = PREVIOUS["VERSIONS"] + [ROOT / "logs/surface-entry-working-cash-before-edit.json"]
MAX_WINDOWS, MAX_TICKS = 8, 16000


def execute(configuration, phase, window=0, recover_run=None):
    """Run or recover one exact proposal or consecutive actual cash-goal interval.

    @param configuration Independently linked isolated native application.
    @param phase Pure query or normal work; no asset or route command is selected.
    @param window Strict consecutive work ordinal; zero for the pure query.
    @param recover_run Exact successful invocation to inspect without repeating earned work.
    @return Whole actual fares, costs, current/retired owners and the finite cash-goal status.
    """
    assert phase in ("query", "work")
    total_before = 0
    if phase == "query":
        assert window == 0
        receipt = ROOT / f"logs/surface-entry-funded-farm-goods-work-002-{configuration}.json"
    else:
        assert 1 <= window <= MAX_WINDOWS
        name = "query" if window == 1 else f"work-{window-1:03d}"
        receipt = ROOT / f"logs/surface-entry-working-cash-{name}-{configuration}.json"
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration
    prior = previous["operation"]
    cache = IdentityCache()
    for reference in prior["sources"] + [prior["application"]]:
        verify(reference, VERSIONS, identity_reader=cache)
    source = ROOT / prior["output"]["path"]
    assert identity(source) == prior["output"] and json.loads(json.dumps(observe(source))) == prior["final"]
    if phase == "query":
        assert prior["goods_scope"]["end_reason"] == "goods_payment_posted" and prior["payment_posted"]
        assert prior["final"]["company"]["cash"] == 83268 and prior["cumulative_accepted"] == 50
        first_goods = identity(receipt)
    else:
        scope = prior["reserve_scope"]
        assert scope["window"] == window-1 and not scope["ended"]
        total_before = scope["total_earned_ticks"]
        first_goods = prior["first_goods_receipt"]
        verify(first_goods)
    assert prior["final"]["company"]["cash"] < 100000
    ceiling = min(2000, MAX_TICKS-total_before)
    assert ceiling >= 64
    name = phase if phase != "work" else f"work-{window:03d}"
    attempt = ROOT / f"logs/surface-entry-working-cash-{name}-{configuration}-attempt.json"
    if recover_run is None:
        assert not attempt.exists(), "recover the exact original invocation instead of repeating income or work"
        attempt.write_text(json.dumps(dict(_documentation=__doc__, receipt=identity(receipt), reserve=40000,
            target=100000, phase=phase, window=window, ceiling=ceiling), indent=2)+"\n", encoding="utf-8")
    else:
        assert json.loads(attempt.read_text(encoding="utf-8"))["receipt"] == identity(receipt)
    files = [ROOT / reference["path"] for reference in prior["sources"] if verify(reference, VERSIONS, identity_reader=cache)] + [
        Path(__file__), ROOT / "src/tests/surface_entry_funded_farm_goods.h", ROOT / "src/tests/surface_entry_working_cash.h"] + VERSIONS
    operation = native(configuration, "working-cash-"+phase, receipt, observe,
        dict(plan="FUNDED_FARM_GOODS_PLAN", admission="GOODS_RAIL_FEEDER_WORK_ADMISSION", selection="FUNDED_FARM_GOODS_SELECTION",
            service="FUNDED_FARM_GOODS_SERVICE", mode="WORKING_CASH_MODE", earned="EARNED", expenses="EXPENSE"),
        list(dict.fromkeys(files)), recover_run=recover_run, work_limit=ceiling)
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
    expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
    refused = False
    restored = last["company"]["cash"] >= 100000
    if phase == "query":
        assert elapsed == delta == 0 and first == last and chunks(before)[1] == chunks(output)[1]
        plan, = rows["plan"]
        assert plan["funded"] and plan["selected"] == 2000 and plan["cash"] == 83268
        assert rows["admission"][-1]["funded"] and rows["admission"][-1]["target"] <= 83268
    else:
        selected, = rows["selection"]
        assert selected["ceiling"] == ceiling and not selected["settlement"]
        refused = not selected["found"]
        if refused:
            assert elapsed == delta == 0 and not rows["earned"] and chunks(before)[1] == chunks(output)[1]
        else:
            service, = rows["service"]
            mode, = rows["mode"]
            earned, = rows["earned"]
            assert 0 < elapsed == service["ticks"] == earned["earned"] <= selected["ticks"] <= ceiling
            assert rows["admission"][-1]["funded"] and rows["admission"][-1]["target"] <= first["company"]["cash"]
            # A paid Goods event cannot end this separate cash-goal scope.
            # Only actual money completes the working-floor requirement.
            assert not mode["stop_on_payment"] and mode["target"] == 100000 and mode["reserve"] == 40000
            assert earned["reached"] == mode["restored"] == service["restored"] == restored
            assert mode["cash"] == service["cash"] == last["company"]["cash"]
        assert sum(expenses.values()) == -delta
    assert last["company"]["cash"] >= 40000
    total = total_before+elapsed
    reason = ("working_floor" if restored else "funding_refused" if refused else "tick_cap" if MAX_TICKS-total < 64
              else "window_cap" if window == MAX_WINDOWS else None)
    operation.update(earned_ticks=elapsed, startup_reserve=40000, first_goods_receipt=first_goods, working_floor_restored=restored,
        historical_versions=[identity(path) for path in VERSIONS],
        measured_cash_flow=dict(ticks=elapsed, actual_fares=-expenses.get(7, 0)-expenses.get(8, 0), interest=expenses.get(11, 0),
            other_operating_cost=sum(value for kind, value in expenses.items() if kind not in (7, 8, 11)), cash_delta=delta),
        reserve_scope=dict(window=window, total_earned_ticks=total, maximum_windows=MAX_WINDOWS,
            tick_cap=MAX_TICKS, ended=reason is not None, end_reason=reason))
    return operation


def main():
    """Retain one exact original pure proposal or cash-goal work endpoint."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("query", "work"), required=True)
    parser.add_argument("--window", type=int, default=0)
    parser.add_argument("--recover-run")
    args = parser.parse_args()
    name = args.phase if args.phase != "work" else f"work-{args.window:03d}"
    output = ROOT / f"logs/surface-entry-working-cash-{name}-{args.configuration}.json"
    assert not output.exists()
    operation = execute(args.configuration, args.phase, args.window, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
        phase="working-cash-"+args.phase, operation=operation), indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), operation["reserve_scope"], operation["measured_cash_flow"], flush=True)


if __name__ == "__main__":
    main()
