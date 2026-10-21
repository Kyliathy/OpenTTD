"""Return the actual original idle builder home through fully funded native work.

@file surface_entry_constructor_home_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEC.
No hypothetical refund, avoided upkeep, cargo or movement funds this boundary.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_original_evidence import verify
from surface_regional_ledger import TABLE
from surface_surplus_recovery_ledger import observe as old_queue
from surface_constructor_home_ledger import observe
from surface_constructor_home_native import execute as native

HERE = Path(__file__).resolve().parent
PREVIOUS = runpy.run_path(str(HERE / "surface_entry_surplus_recovery_checks-1.py"))
VERSIONS = PREVIOUS["VERSIONS"] + [ROOT / "logs/surface-entry-constructor-home-before-edit.json"]
MAX_WINDOWS, MAX_TICKS = 4, 500
OWNERS = [HERE / name for name in ("surface_gateway_core_ledger.py", "surface_gateway_assembly_ledger.py",
    "surface_gateway_material_ledger.py", "surface_gateway_resupply_ledger.py", "surface_gateway_construction_ledger.py",
    "surface_gateway_component_delivery_ledger.py", "surface_farm_backhaul_ledger.py", "surface_surplus_recovery_ledger.py")]


def execute(configuration, phase, window=0, recover_run=None):
    """Run or recover precisely one ordered original home boundary.

    @param configuration Independently linked isolated native application.
    @param phase Zero-time public order or finite freshly funded native work.
    @param window Strict consecutive work ordinal; zero for the public order.
    @param recover_run Exact successful invocation to inspect without repeating any command or tick.
    @return Native order/navigation/stop proof and real whole-company costs, with sale value unbanked.
    """
    assert phase in ("order", "work")
    total_before = 0
    if phase == "order":
        assert window == 0
        receipt = ROOT / f"logs/surface-entry-surplus-recovery-pay-{configuration}.json"
    else:
        assert 1 <= window <= MAX_WINDOWS
        name = "order" if window == 1 else f"work-{window-1:03d}"
        receipt = ROOT / f"logs/surface-entry-constructor-home-{name}-{configuration}.json"
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration
    prior = previous["operation"]
    for reference in prior["sources"] + [prior["application"]]:
        verify(reference, VERSIONS)
    source = ROOT / prior["output"]["path"]
    assert identity(source) == prior["output"] and json.loads(json.dumps(observe(source))) == prior["final"]
    if phase == "order":
        assert prior["actual_sale_complete"] and prior["refund_banked"] == 1504 and prior["actual_cash"] == 41666
    else:
        assert not prior["home_scope"]["ended"] and prior["home_scope"]["window"] == window-1
        total_before = prior["home_scope"]["total_earned_ticks"]
    ceiling = min(250, MAX_TICKS-total_before)
    assert ceiling >= 64
    name = phase if phase != "work" else f"work-{window:03d}"
    attempt = ROOT / f"logs/surface-entry-constructor-home-{name}-{configuration}-attempt.json"
    if recover_run is None:
        assert not attempt.exists(), "recover the successful original invocation instead of repeating orders or work"
        attempt.write_text(json.dumps(dict(_documentation=__doc__, receipt=identity(receipt), reserve=40000,
            phase=phase, window=window, ceiling=ceiling), indent=2)+"\n", encoding="utf-8")
    else:
        assert json.loads(attempt.read_text(encoding="utf-8"))["receipt"] == identity(receipt)
    files = [ROOT / reference["path"] for reference in prior["sources"] if verify(reference, VERSIONS)] + OWNERS + [
        Path(__file__), HERE / "surface_constructor_home_ledger.py", ROOT / "src/tests/surface_entry_constructor_home.h",
        ROOT / "src/space_orders.cpp", ROOT / "src/space.cpp", ROOT / "src/space_replacement.cpp",
        ROOT / "src/space_vehicle.cpp", ROOT / "src/autoreplace_cmd.cpp"] + VERSIONS
    operation = native(configuration, "constructor-home-"+phase, receipt, observe,
        dict(admission="CONSTRUCTOR_HOME_ADMISSION", ordered="CONSTRUCTOR_HOME_ORDERED", selection="CONSTRUCTOR_HOME_SELECTION",
            service="CONSTRUCTOR_HOME_SERVICE", sale="CONSTRUCTOR_HOME_SALE", invoices="INVOICE", earned="EARNED", expenses="EXPENSE"),
        files, recover_run=recover_run, work_limit=ceiling)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    before, output = [ROOT / operation[name]["path"] for name in ("before", "output")]
    assert json.loads(json.dumps(first)) == prior["final"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"]-TABLE(before, b"DATE")[0]["tick_counter"]
    delta = last["company"]["cash"]-first["company"]["cash"]
    assert all(row["reserve"] == 40000 and row["target"] == row["operating"]+40000 and row["idle_per_day"] == 100
        and row["refund_counted"] == row["avoided_cost_counted"] == row["future_income_counted"] == 0
        and not row["automatic_replacement"] for row in rows["admission"])
    expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
    arrived = refused = False
    if phase == "order":
        assert elapsed == delta == 0 and not rows["earned"] and rows["admission"][-1]["funded"]
        assert rows["admission"][-1]["target"] <= first["company"]["cash"]
        a, b = [TABLE(path, b"VEHS") for path in (before, output)]
        assert a.keys() == b.keys() and all(a[key] == b[key] for key in a if key != 38)
        assert chunks(before)[1][b"CAPA"] == chunks(output)[1][b"CAPA"]
        assert TABLE(before, b"DATE") == TABLE(output, b"DATE")
        ordered, = rows["ordered"]
        assert ordered["invoice"] == ordered["elapsed"] == ordered["refund_banked"] == 0
    else:
        selected, = rows["selection"]
        refused = not selected["found"]
        if refused:
            assert elapsed == delta == 0 and not rows["earned"] and chunks(before)[1] == chunks(output)[1]
        else:
            service, = rows["service"]
            earned, = rows["earned"]
            assert 0 < elapsed == service["ticks"] == earned["earned"] <= selected["ticks"] <= ceiling
            assert rows["admission"][-1]["funded"] and rows["admission"][-1]["target"] <= first["company"]["cash"]
            arrived = service["arrived"]
            assert arrived == earned["reached"] and service["refund_banked"] == 0 and service["idle_per_day"] == 100
            if arrived:
                assert service["stopped"] and service["cursor"] == service["entries"] == 1
                assert service["x"] == service["home_x"] and service["y"] == service["home_y"]
                sale, = rows["sale"]
                assert sale["available"] and sale["invoice"] == -sale["unbanked_refund"]
                assert sale["refund_banked"] == sale["avoided_cost_realized"] == 0
            else:
                assert not rows["sale"]
        assert sum(expenses.values()) == -delta
    queue = last["space_orders"]["queues"][38]
    assert queue["generation"] == 2 and queue["entries"] == [(0, 3, 21, 1)]
    rejected = False
    try:
        old_queue(output)
    except AssertionError:
        rejected = True
    assert rejected, "old queue defaults must reject the genuine new home Visit"
    assert sum(row["cost"] for row in rows["invoices"]) == 0 and last["company"]["cash"] >= 40000
    total = total_before+elapsed
    reason = ("home_arrived_stopped" if arrived else "funding_refused" if refused else "tick_cap"
              if MAX_TICKS-total < 64 else "window_cap" if window == MAX_WINDOWS else None)
    operation.update(earned_ticks=elapsed, startup_reserve=40000, arrived_stopped=arrived, builder_refund_banked=0,
        avoided_cost_realized=0, old_queue_default_rejected=True, historical_versions=[identity(path) for path in VERSIONS],
        measured_cash_flow=dict(ticks=elapsed, actual_fares=-expenses.get(7, 0)-expenses.get(8, 0), interest=expenses.get(11, 0),
            other_operating_cost=sum(value for kind, value in expenses.items() if kind not in (7, 8, 11)), cash_delta=delta),
        home_scope=dict(window=window, total_earned_ticks=total, maximum_windows=MAX_WINDOWS,
            tick_cap=MAX_TICKS, ended=reason is not None, end_reason=reason))
    return operation


def main():
    """Retain one exact original home boundary without repeating successful commands or ticks."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("order", "work"), required=True)
    parser.add_argument("--window", type=int, default=0)
    parser.add_argument("--recover-run")
    args = parser.parse_args()
    name = args.phase if args.phase != "work" else f"work-{args.window:03d}"
    output = ROOT / f"logs/surface-entry-constructor-home-{name}-{args.configuration}.json"
    assert not output.exists()
    operation = execute(args.configuration, args.phase, args.window, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
        phase="constructor-home-"+args.phase, operation=operation), indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), operation["home_scope"], operation["measured_cash_flow"], flush=True)


if __name__ == "__main__":
    main()
