"""Complete the actual empty farm return with a smaller prospective wagon batch.

@file surface_entry_farm_two_return_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVBC.
Native orders, packets, posted money and full operating costs remain the sole
owners. The unchanged forty-thousand floor and prospective2,226are protected together.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import TABLE
from surface_original_evidence import verify
from surface_farm_two_return_native import execute as native

HERE = Path(__file__).resolve().parent
RAIL = runpy.run_path(str(HERE / "surface_entry_goods_rail_delivery_checks-1.py"))
observe, TRAIN = [RAIL[name] for name in ("observe", "TRAIN")]
VERSIONS = [ROOT / "logs/surface-entry-farm-two-return-before-edit.json",
            ROOT / "logs/surface-entry-rail-productivity-recorder-versions.json"]
MAX_WINDOWS, MAX_TICKS = 4, 2000


def held(path):
    """Read the whole actual52consist's sole native freight.

    @param path Actual native saved checkpoint.
    @return Whole real coupled Livestock/Grain quantity.
    """
    vehicles, packets = TABLE(path, b"VEHS"), TABLE(path, b"CAPA")
    current, visited, quantity = 52, set(), 0
    # Follow actual one-based native links; every trailing wagon owns its own
    # packets, and an empty powered head proves no delivery by itself.
    while current is not None:
        assert current not in visited
        visited.add(current)
        row = vehicles[current]
        quantity += sum(packets[ref-1]["count"] for ref in row[TRAIN+"cargo.packets"])
        next_ref = row[TRAIN+"next"]
        current = next_ref-1 if next_ref else None
    return quantity


def execute(configuration, phase, window=0, recover_run=None):
    """Execute one exact admitted query or finite minimum-capital real return interval.

    @param configuration Independently linked original native build configuration.
    @param phase Explicit query or work boundary of this return producer.
    @param window Strictly consecutive work ordinal; zero for the pure query.
    @param recover_run Exact original native invocation to inspect without repeating a successful action.
    @return Native boundary and immutable original whole-cost/packet/payment/stop evidence.
    @throws AssertionError On changed authorities, unfunded work, discarded freight or a closed scope.
    """
    assert phase in ("query", "work")
    total_before = accepted_before = 0
    if phase == "query":
        receipt = ROOT / f"logs/surface-entry-farm-return-work-006-{configuration}.json"
    else:
        assert 1 <= window <= MAX_WINDOWS
        name = "query" if window == 1 else f"work-{window-1:03d}"
        receipt = ROOT / f"logs/surface-entry-farm-two-return-{name}-{configuration}.json"
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration
    prior = previous["operation"]
    accepted_before = prior["cumulative_accepted"]
    if phase == "query":
        assert prior["return_scope"]["end_reason"] == "funding_refused"
        assert prior["unpaid_wagons"] == 3339 and prior["final"]["company"]["cash"] == 51796
    # Only explicitly archived isolated-code/recorder epochs may differ. The
    # native financial owners, old public defaults and all original apps remain.
    for ref in prior["sources"]:
        verify(ref, VERSIONS)
    if window > 1:
        scope = prior["return_scope"]
        assert scope["window"] == window-1 and not scope["ended"]
        total_before, accepted_before = scope["total_earned_ticks"], prior["cumulative_accepted"]
    assert total_before <= MAX_TICKS-64
    ceiling = min(500, MAX_TICKS-total_before)
    name = phase if phase != "work" else f"work-{window:03d}"
    attempt = ROOT / f"logs/surface-entry-farm-two-return-{name}-{configuration}-attempt.json"
    if recover_run is None:
        assert not attempt.exists(), "recover the original native invocation instead of repeating it"
        attempt.write_text(json.dumps(dict(receipt=identity(receipt), unpaid=2226, reserve=40000,
            phase=phase, window=window, ceiling=ceiling), indent=2)+"\n", encoding="utf-8")
    else:
        assert json.loads(attempt.read_text(encoding="utf-8"))["receipt"] == identity(receipt)
    files = [ROOT / ref["path"] for ref in prior["sources"] if verify(ref, VERSIONS)] + [
        Path(__file__), ROOT / "src/tests/surface_entry_mixed_farm_return.h"] + VERSIONS
    operation = native(configuration, "farm-two-return-"+phase, receipt, observe,
        dict(admission="GOODS_RAIL_FEEDER_WORK_ADMISSION", plan="FARM_TWO_RETURN_PLAN", ordered="FARM_RETURN_ORDERED",
             selection="FARM_RETURN_SELECTION", service="FARM_RETURN_SERVICE", earned="EARNED", expenses="EXPENSE"),
        files, recover_run=recover_run, work_limit=ceiling)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == prior["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"]-TABLE(before, b"DATE")[0]["tick_counter"]
    initial, final = held(before), held(output)
    assert all(row["reserve"] == 40000 and row["unpaid"] in (2226, 3339) and row["future_income_counted"] == 0
               and row["target"] == 40000+row["unpaid"]+row["operating"] for row in rows["admission"])
    accepted, refused, reached = 0, False, False
    if phase == "work":
        assert all(row["unpaid"] == 2226 for row in rows["admission"])
        selected, = rows["selection"]
        assert selected["initial"] == initial and selected["ceiling"] == ceiling
        refused = not selected["found"]
        if refused:
            assert elapsed == 0 and not rows["service"] and not rows["earned"]
            assert chunks(before)[1] == chunks(output)[1]
            assert not any(row["funded"] for row in rows["admission"])
        else:
            service, = rows["service"]
            earned, = rows["earned"]
            assert 0 < elapsed == service["ticks"] == earned["earned"] <= selected["ticks"] <= ceiling
            assert rows["admission"][-1]["funded"] and rows["admission"][-1]["target"] <= first["company"]["cash"]
            accepted, reached = service["accepted"], service["depot_halt"]
            assert service["initial"] == initial and service["held"] == final and initial == accepted+final
            assert reached == earned["reached"]
            if reached:
                assert final == 0 and not service["pending_payment"]
                assert TABLE(output, b"VEHS")[52][TRAIN+"tile"] == 1638
    else:
        assert elapsed == 0 and initial == final == 0 and first == last
        assert chunks(before)[1] == chunks(output)[1]
        plan, = rows["plan"]
        assert plan["funded"] and plan["unpaid_wagons"] == 2226 and plan["old_default_earned"] == 0
        old = [row for row in rows["admission"] if row["unpaid"] == 3339]
        revised = [row for row in rows["admission"] if row["unpaid"] == 2226]
        assert old and not any(row["funded"] for row in old)
        assert revised[-1]["funded"] and revised[-1]["target"] <= first["company"]["cash"]
    expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
    delta = last["company"]["cash"]-first["company"]["cash"]
    assert sum(expenses.values()) == -delta
    assert last["company"]["cash"] >= 42226
    total, cumulative = total_before+elapsed, accepted_before+accepted
    assert total <= MAX_TICKS and initial == final+accepted
    # This physical preparation ends at real empty depot arrival, first refusal
    # or its finite cap. It never authorizes buying a wagon or another scope.
    reason = ("empty_depot" if reached else "funding_refused" if refused else
              "tick_cap" if MAX_TICKS-total < 64 else "window_cap" if window == MAX_WINDOWS else None)
    operation.update(earned_ticks=elapsed, startup_reserve=40000, unpaid_wagons=2226,
        cumulative_accepted=cumulative, depot_halt=reached, historical_versions=[identity(path) for path in VERSIONS],
        measured_cash_flow=dict(ticks=elapsed, actual_fares=-expenses.get(7, 0)-expenses.get(8, 0),
            interest=expenses.get(11, 0), other_operating_cost=sum(value for kind, value in expenses.items()
                if kind not in (7, 8, 11)), cash_delta=delta),
        return_scope=dict(window=window, total_earned_ticks=total, maximum_windows=MAX_WINDOWS,
            tick_cap=MAX_TICKS, ended=reason is not None, end_reason=reason))
    return operation


def main():
    """Retain one exact original farm-return boundary without repeating any successful work."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("query", "work"), required=True)
    parser.add_argument("--window", type=int, default=0)
    parser.add_argument("--recover-run")
    args = parser.parse_args()
    name = args.phase if args.phase != "work" else f"work-{args.window:03d}"
    output = ROOT / f"logs/surface-entry-farm-two-return-{name}-{args.configuration}.json"
    assert not output.exists()
    operation = execute(args.configuration, args.phase, args.window, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
        phase="farm-two-return-"+args.phase, operation=operation), indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), operation["return_scope"], operation["measured_cash_flow"], flush=True)


if __name__ == "__main__":
    main()
