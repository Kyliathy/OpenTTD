"""Buy and measure only genuine, fully funded mixed farm/Goods backhaul.

@file surface_entry_farm_backhaul_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVD.
Actual commands, native packets, ordinary work and posted money own all results.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_regional_ledger import TABLE
from surface_original_evidence import verify
from surface_farm_backhaul_ledger import observe
from surface_farm_backhaul_native import execute as native

HERE = Path(__file__).resolve().parent
OLD = runpy.run_path(str(HERE / "surface_entry_goods_rail_delivery_checks-1.py"))["observe"]
TRAIN = "train[0].common[0]."
VERSIONS = [ROOT / ("logs/"+name+".json") for name in (
    "surface-entry-farm-backhaul-before-edit", "surface-entry-farm-backhaul-slot-reuse-before-edit", "surface-entry-farm-two-return-before-edit",
    "surface-entry-rail-productivity-recorder-versions")]
MAX_WINDOWS, MAX_TICKS = 24, 6000


def goods(path):
    """Read only the actual farm train's new Goods freight.

    @param path Actual same-save native vehicle and cargo tables.
    @return Goods quantity, native virtual share and full actual paid-body packet rows.
    """
    state = observe(path)["farm_balance"]
    bodies = state.get("goods_wagons", [])
    return dict(quantity=sum(body["held"] for body in bodies),
        share=sum(packet["feeder_share"] for body in bodies for packet in body["packets"]), bodies=bodies)


def execute(configuration, phase, window=0, recover_run=None, retry_failed_query=False):
    """Run or recover exactly one consecutive native mixed-service boundary.

    @param configuration Independently linked original native build configuration.
    @param phase Pure query, actual paid change or freshly funded work interval.
    @param window Strict consecutive work ordinal; zero for query and payment commands.
    @param recover_run Exact successful invocation to inspect without repeating commands or work.
    @param retry_failed_query One corrected pure proposal after the explicitly retained unchanged failed query.
    @return Exact paid identities, original material custody and measured whole-company cash flow.
    """
    assert phase in ("query", "pay", "work")
    total_before = boarded_before = accepted_before = 0
    if phase == "query":
        receipt = ROOT / f"logs/surface-entry-farm-two-return-work-003-{configuration}.json"
    elif phase == "pay":
        receipt = ROOT / f"logs/surface-entry-farm-backhaul-query-{configuration}.json"
    else:
        assert 1 <= window <= MAX_WINDOWS
        previous_name = "pay" if window == 1 else f"work-{window-1:03d}"
        receipt = ROOT / f"logs/surface-entry-farm-backhaul-{previous_name}-{configuration}.json"
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration
    prior = previous["operation"]
    for reference in prior["sources"] + [prior["application"]]:
        verify(reference, VERSIONS)
    source = ROOT / prior["output"]["path"]
    assert identity(source) == prior["output"]
    assert json.loads(json.dumps(observe(source))) == prior["final"]
    if phase == "query":
        assert prior["return_scope"]["end_reason"] == "empty_depot" and prior["final"]["company"]["cash"] == 43368
    else:
        assert not prior["backhaul_scope"]["ended"]
        if window > 1:
            assert prior["backhaul_scope"]["window"] == window-1
            total_before = prior["backhaul_scope"]["total_earned_ticks"]
            boarded_before, accepted_before = prior["cumulative_boarded"], prior["cumulative_accepted"]
    ceiling = min(1000, MAX_TICKS-total_before)
    assert ceiling >= 64
    pending = [row for row in TABLE(source, b"CAPY").values() if row["front"] == 53]
    # A payment-only interval requires an actual preceding Goods acceptance,
    # an empty Goods hold and this train's own positive native pending fare.
    settlement = bool(accepted_before > 0 and goods(source)["quantity"] == 0 and pending and pending[0]["route_profit"] > 0)
    native_phase = "payment" if phase == "work" and settlement else phase
    name = phase if phase != "work" else f"work-{window:03d}"
    attempt = ROOT / f"logs/surface-entry-farm-backhaul-{name}-{configuration}-attempt.json"
    if recover_run is None:
        if retry_failed_query:
            assert phase == "query" and configuration == "RelWithDebInfo" and attempt.exists()
            failure = ROOT / "logs/surface-entry-farm-backhaul-failed-query-RelWithDebInfo.json"
            proof = json.loads(failure.read_text(encoding="utf-8"))
            assert proof["complete"] and proof["no_commands_or_work"] and proof["source"] == prior["output"]
            for reference in proof["artifacts"]:
                verify(reference)
            assert chunks(ROOT / proof["before"]["path"])[1] == chunks(ROOT / proof["failed"]["path"])[1]
            attempt = attempt.with_name(attempt.stem+"-corrected.json")
        assert not attempt.exists(), "recover the exact successful invocation; never repeat native work or purchases"
        attempt.write_text(json.dumps(dict(_documentation=__doc__, receipt=identity(receipt), phase=phase,
            window=window, ceiling=ceiling, reserve=40000), indent=2)+"\n", encoding="utf-8")
    else:
        assert json.loads(attempt.read_text(encoding="utf-8"))["receipt"] == identity(receipt)
    files = [ROOT / ref["path"] for ref in prior["sources"] if verify(ref, VERSIONS)] + [
        Path(__file__), HERE / "surface_farm_backhaul_ledger.py", HERE / "surface_gateway_component_delivery_ledger.py",
        ROOT / "src/tests/surface_entry_mixed_farm_backhaul.h", ROOT / "src/train_cmd.cpp",
        ROOT / "src/order_cmd.cpp", ROOT / "src/vehicle_cmd.cpp"] + VERSIONS
    operation = native(configuration, "farm-backhaul-"+native_phase, receipt, observe,
        dict(startup="FARM_BACKHAUL_ADMISSION", plan="FARM_BACKHAUL_PLAN", paid="FARM_BACKHAUL_PAID",
            wagons="FARM_BACKHAUL_WAGON", admission="GOODS_RAIL_FEEDER_WORK_ADMISSION", invoices="INVOICE",
            selection="FARM_BACKHAUL_SELECTION", service="FARM_BACKHAUL_SERVICE", earned="EARNED", expenses="EXPENSE"),
        files, recover_run=recover_run, work_limit=ceiling)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    assert json.loads(json.dumps(first)) == prior["final"]
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"]-TABLE(before, b"DATE")[0]["tick_counter"]
    delta = last["company"]["cash"]-first["company"]["cash"]
    initial, final = goods(before), goods(output)
    boarded = accepted = 0
    refused = posted = False
    expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
    if phase in ("query", "pay"):
        assert elapsed == 0 and not rows["earned"]
        assert all(row["target"] == row["operating"]+42226 and row["existing"]+row["reactivation"]+4 == row["operating"]
                   and row["future_income_counted"] == row["refund_counted"] == 0 for row in rows["startup"])
        plan, = rows["plan"]
        refused = not plan["funded"]
        if phase == "query":
            assert first == last and chunks(before)[1] == chunks(output)[1] and delta == 0
        else:
            assert not refused and rows["startup"][-1]["funded"]
            assert rows["startup"][-1]["target"] <= first["company"]["cash"]
            assert delta == -2226 and sum(row["cost"] for row in rows["invoices"]) == 2226
            assert len(rows["wagons"]) == 2 and all(row["invoice"] == 1113 for row in rows["wagons"])
            old, new = [TABLE(path, b"VEHS") for path in (before, output)]
            ids = {body["vehicle"] for body in final["bodies"]}
            assert len(ids) == 2 and not ids & old.keys() and new.keys() == old.keys() | ids
            assert ids == {row["wagon"] for row in rows["wagons"]}
            # Zero-time native commands may change only the real farm cohort
            # and its new tails. Every other actual vehicle and packet stays exact.
            farm = {52, 50, 57, 58, 59, *range(12, 24)}
            assert all(old[key] == new[key] for key in old if key not in farm)
            assert chunks(before)[1][b"CAPA"] == chunks(output)[1][b"CAPA"]
            dates = [TABLE(path, b"DATE")[0] for path in (before, output)]
            assert {k: v for k, v in dates[0].items() if not k.startswith("random_state")} == {k: v for k, v in dates[1].items() if not k.startswith("random_state")}
            rejected = False
            try:
                OLD(output)
            except AssertionError:
                rejected = True
            assert rejected, "historical strict observer must reject this genuine new mixed state"
            operation.update(actual_paid_goods_ids=sorted(ids), old_default_rejected=True)
    else:
        selected, = rows["selection"]
        refused = not selected["found"]
        assert all(row["reserve"] == 40000 and row["unpaid"] == 0 and row["future_income_counted"] == 0
                   and row["target"] == row["operating"]+40000 for row in rows["admission"])
        if refused:
            assert elapsed == 0 and not rows["earned"] and chunks(before)[1] == chunks(output)[1]
        else:
            service, = rows["service"]
            earned, = rows["earned"]
            assert 0 < elapsed == service["ticks"] == earned["earned"] <= selected["ticks"] <= ceiling
            assert rows["admission"][-1]["funded"] and rows["admission"][-1]["target"] <= first["company"]["cash"]
            boarded, accepted, posted = service["boarded"], service["accepted"], service["payment_posted"]
            assert initial["quantity"]+boarded == accepted+final["quantity"]
            assert service["settlement"] == settlement
            if posted:
                assert accepted_before+accepted > 0 and final["quantity"] == 0 and service["observed_whole_unload_fare"] > 0
                assert not service["payment_pending"]
        assert sum(expenses.values()) == -delta
    assert last["company"]["cash"] >= 40000
    total = total_before+elapsed
    reason = ("funding_refused" if refused else "goods_payment_posted" if posted else "working_floor"
              if last["company"]["cash"] >= 100000 else "tick_cap" if MAX_TICKS-total < 64 else
              "window_cap" if window == MAX_WINDOWS else None)
    operation.update(earned_ticks=elapsed, startup_reserve=40000, cumulative_boarded=boarded_before+boarded,
        cumulative_accepted=accepted_before+accepted, actual_goods=final, historical_versions=[identity(path) for path in VERSIONS],
        measured_cash_flow=dict(ticks=elapsed, actual_fares=-expenses.get(7, 0)-expenses.get(8, 0),
            interest=expenses.get(11, 0), other_operating_cost=sum(value for kind, value in expenses.items() if kind not in (7, 8, 11)),
            gross_purchases=2226 if phase == "pay" else 0, cash_delta=delta),
        backhaul_scope=dict(window=window, total_earned_ticks=total, maximum_windows=MAX_WINDOWS,
            tick_cap=MAX_TICKS, ended=reason is not None, end_reason=reason))
    return operation


def main():
    """Retain one original boundary; successful native work is always recovered rather than repeated."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("query", "pay", "work"), required=True)
    parser.add_argument("--window", type=int, default=0)
    parser.add_argument("--recover-run")
    parser.add_argument("--retry-failed-query", action="store_true")
    args = parser.parse_args()
    name = args.phase if args.phase != "work" else f"work-{args.window:03d}"
    output = ROOT / f"logs/surface-entry-farm-backhaul-{name}-{args.configuration}.json"
    assert not output.exists()
    operation = execute(args.configuration, args.phase, args.window, args.recover_run, args.retry_failed_query)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
        phase="farm-backhaul-"+args.phase, operation=operation), indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), operation["backhaul_scope"], operation["measured_cash_flow"], flush=True)


if __name__ == "__main__":
    main()
