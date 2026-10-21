"""Settle the loaded native arrival only after a new exact user authorization.

@file surface_entry_arrival_settlement_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZS.
A new proposal-bound decision is required before the temporary reserve admits work;
physical cargo and unbanked shares cannot fund any subsequent interval.
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
observe, rail_stock, TRAIN = [RAIL[name] for name in ("observe", "rail_stock", "TRAIN")]
PHASES = dict(work="staged-chemical-capital-joint-settlement-work",
              payment="staged-chemical-capital-joint-settlement-payment")
MAX_WINDOWS, MAX_TICKS = 8, 4000
SCOPE = "arrival_settlement"


PROPOSAL = ROOT / "logs/surface-entry-arrival-settlement-proposal.json"
AUTHORIZATION = ROOT / "logs/surface-entry-arrival-settlement-authorization.json"


def check_proposal(configuration, receipt):
    """Verify this exact original and its complete first-window proposal without work.

    @param configuration Original independently linked native build configuration.
    @param receipt Exact bound loaded-arrival002 receipt for the first window.
    @return Validated read-only proposal and unchanged first-window admission row.
    @throws AssertionError If any reviewed caller, source, quote or finite limit changed.
    """
    proposal = json.loads(PROPOSAL.read_text(encoding="utf-8"))
    assert proposal["authorized"] is False and proposal["normal_ticks_executed"] == 0
    assert proposal["reserve"] == 30000 and proposal["default_reserve"] == 40000
    assert proposal["maximum_windows"] == MAX_WINDOWS and proposal["tick_cap"] == MAX_TICKS
    assert proposal["window_ceiling"] == 500 and proposal["stop_on_first_posted_payment"]
    assert identity(ROOT / proposal["foundation"]["path"]) == proposal["foundation"]
    assert all(identity(ROOT / ref["path"]) == ref for ref in proposal["reviewed_callers"])
    entry = proposal["configurations"][configuration]
    assert identity(receipt) == entry["receipt"]
    previous = json.loads(receipt.read_text(encoding="utf-8"))["operation"]
    assert previous["arrival_work_scope"]["end_reason"] == "funding_refused"
    assert all(identity(ROOT / previous[key]["path"]) == previous[key]
               for key in ("output", "application"))
    assert all(identity(ROOT / ref["path"]) == ref for ref in previous["sources"])
    quote, = [row for row in previous["rows"]["admission"] if row["ticks"] == 500]
    assert quote == entry["original_quote"] and not quote["funded"]
    assert quote["cash"] == 46064 and quote["operating"] == 9479
    assert entry["proposed_full_target"] == quote["operating"]+30000 <= quote["cash"]
    return proposal, quote


def require_authorization(configuration, root_receipt):
    """Require a new explicit proposal-bound decision before any native attempt.

    @param configuration Original independently linked native build configuration.
    @param root_receipt Exact immutable GZRBpaid source of the entire new settlement scope.
    @return Identity of the explicit new user decision, retained with every interval.
    @throws AssertionError If the new decision is absent, mismatched or outside these limits.
    """
    check_proposal(configuration, root_receipt)
    assert AUTHORIZATION.is_file(), "new explicit user authorization is required; the earlier bridge is spent"
    approval = json.loads(AUTHORIZATION.read_text(encoding="utf-8"))
    assert approval["authorized"] is True and approval["proposal"] == identity(PROPOSAL)
    assert approval["reserve"] == 30000 and approval["default_reserve"] == 40000
    assert approval["maximum_windows"] == MAX_WINDOWS and approval["tick_cap"] == MAX_TICKS
    assert approval["window_ceiling"] == 500 and approval["stop_on_first_posted_payment"]
    assert approval["configurations"] == ["RelWithDebInfo", "Debug"] and approval["user_decision"]
    return identity(AUTHORIZATION)


def execute(configuration, receipt, window, recover_run=None):
    """Earn one consecutive interval from the exact loaded paid predecessor.

    @param configuration Original independently linked native build configuration.
    @param receipt Exact closed GZRB endpoint or this scope's immediately preceding receipt.
    @param window Consecutive interval ordinal, bounded by the eight-window scope.
    @param recover_run Exact original native invocation to inspect without rerunning work.
    @return Selected native phase and reconciled physical, financial and stop evidence.
    @throws AssertionError On a changed authority, closed scope or duplicated invocation.
    """
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration
    assert 1 <= window <= MAX_WINDOWS
    prior = previous["operation"]
    accepted_before = total_before = 0
    if window == 1:
        anchor = ROOT / f"logs/surface-entry-loaded-arrival-002-{configuration}.json"
        assert receipt.resolve() == anchor.resolve()
        assert prior["arrival_work_scope"]["ended"] and not prior["rail_payment_posted"]
        assert prior["startup_reserve"] == 40000 and prior["final"]["company"]["cash"] == 46064
    else:
        expected = ROOT / f"logs/surface-entry-arrival-settlement-{window-1:03d}-{configuration}.json"
        assert receipt.resolve() == expected.resolve()
        scope = prior["settlement_scope"]
        assert scope["kind"] == SCOPE and scope["window"] == window-1 and not scope["ended"]
        anchor = ROOT / prior["paid_root_receipt"]["path"]
        assert identity(anchor) == prior["paid_root_receipt"]
        total_before = scope["total_earned_ticks"]
        accepted_before = prior["cumulative_rail_accepted"]
    authorization = require_authorization(configuration, anchor)
    root = json.loads(anchor.read_text(encoding="utf-8"))
    assert root["complete"] and root["configuration"] == configuration
    assert root["operation"]["arrival_work_scope"]["ended"]
    assert all(identity(ROOT / ref["path"]) == ref for ref in prior["sources"])
    assert identity(ROOT / prior["application"]["path"]) == prior["application"]
    source = ROOT / prior["output"]["path"]
    assert identity(source) == prior["output"]
    assert json.loads(json.dumps(observe(source))) == prior["final"]
    assert prior["final"]["company"]["cash"] < 100000
    assert 0 <= total_before <= MAX_TICKS-64
    ceiling = min(500, MAX_TICKS-total_before)
    initial = rail_stock(source)
    pending_before = [row for row in TABLE(source, b"CAPY").values() if row["front"] == 50]
    # Only this arrival's actual accepted cargo and outstanding positive native
    # payment qualify settlement. An unrelated older unload cannot close it.
    settlement = bool(accepted_before > 0 and initial["quantity"] == 0 and pending_before
                      and pending_before[0]["route_profit"] > 0)
    phase = PHASES["payment"] if settlement else PHASES["work"]
    attempt = ROOT / f"logs/surface-entry-arrival-settlement-{window:03d}-{configuration}-attempt.json"
    if recover_run is None:
        assert not attempt.exists(), "recover the recorded original invocation instead of repeating work"
        attempt.write_text(json.dumps(dict(receipt=identity(receipt), paid_root=identity(anchor),
            window=window, ceiling=ceiling, reserve=30000, maximum_native_invocations=1), indent=2)+"\n",
            encoding="utf-8")
    else:
        marker = json.loads(attempt.read_text(encoding="utf-8"))
        assert marker["receipt"] == identity(receipt) and marker["paid_root"] == identity(anchor)
    files = [ROOT / ref["path"] for ref in prior["sources"]] + [Path(__file__), PROPOSAL, AUTHORIZATION]
    operation = native(configuration, phase, receipt, observe,
        dict(admission="GOODS_RAIL_FEEDER_WORK_ADMISSION", selection="GOODS_RAIL_DELIVERY_SELECTION",
             service="GOODS_RAIL_DELIVERY_SERVICE", earned="EARNED", expenses="EXPENSE"),
        files, recover_run=recover_run, chemical_limit=ceiling)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == prior["final"]
    assert operation["application"] == prior["application"]
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    before, output = [ROOT / operation[key]["path"] for key in ("before", "output")]
    elapsed = TABLE(output, b"DATE")[0]["tick_counter"] - TABLE(before, b"DATE")[0]["tick_counter"]
    initial, final = [rail_stock(path) for path in (before, output)]
    selected, = rows["selection"]
    cases = rows["admission"]
    assert selected["ceiling"] == ceiling and selected["settlement"] == settlement
    assert selected["initial"] == initial["quantity"]
    assert all(row["reserve"] == 30000 and row["ticks"] <= ceiling and row["unpaid"] == 0
               and row["target"] == row["operating"]+30000 and row["future_income_counted"] == 0 for row in cases)
    refused, posted, cumulative = not selected["found"], False, accepted_before
    service = None
    if refused:
        assert not any(row["funded"] for row in cases)
        assert elapsed == 0 and not rows["service"] and not rows["earned"]
        assert chunks(before)[1] == chunks(output)[1]
    else:
        assert cases[-1]["funded"] and not any(row["funded"] for row in cases[:-1])
        assert cases[-1]["ticks"] == selected["ticks"] and cases[-1]["target"] <= first["company"]["cash"]
        service, = rows["service"]
        earned, = rows["earned"]
        assert 0 < elapsed == service["ticks"] == earned["earned"] <= selected["ticks"] <= ceiling
        assert service["initial"] == initial["quantity"] and service["aboard"] == final["quantity"]
        assert service["share"] == final["share"]
        assert initial["quantity"]+service["boarded"] == service["accepted"]+final["quantity"]
        assert service["settlement"] == settlement and service["stop_on_payment"]
        assert not service["cohort_only_fare_claimed"] and not service["feeder_credit_is_cash"]
        pending = [row for row in TABLE(output, b"CAPY").values() if row["front"] == 50]
        assert service["payment_pending"] == bool(pending)
        cumulative, posted = accepted_before+service["accepted"], service["payment_posted"]
        if posted:
            assert cumulative > 0 and service["observed_whole_unload_fare"] > 0
            assert not pending and final["quantity"] == 0
            assert TABLE(output, b"VEHS")[49][TRAIN+"last_station_visited"] == 11
        assert earned["reached"] == (posted or last["company"]["cash"] >= 100000)
        assert earned["reached"] or elapsed == selected["ticks"]
    expenses = {row["type"]: row["amount"] for row in rows["expenses"]}
    assert sum(expenses.values()) == first["company"]["cash"]-last["company"]["cash"]
    assert last["company"]["cash"] >= 30000
    total = total_before+elapsed
    assert total <= MAX_TICKS
    # A real posted payment, cash goal, funding refusal or finite cap ends this
    # consumer. Remaining cargo cannot authorize work beyond any such stop.
    reason = ("posted_payment" if posted else "cash_goal" if last["company"]["cash"] >= 100000 else
              "funding_refused" if refused else "tick_cap" if MAX_TICKS-total < 64 else
              "window_cap" if window == MAX_WINDOWS else None)
    operation.update(authorization=authorization, paid_root_receipt=identity(anchor), startup_reserve=30000,
        work_refused=refused, rail_payment_posted=posted, cumulative_rail_accepted=cumulative,
        measured_cash_flow=dict(ticks=elapsed, actual_fares=-expenses.get(7, 0)-expenses.get(8, 0),
            interest=expenses.get(11, 0), other_operating_cost=sum(value for key, value in expenses.items()
                if key not in (7, 8, 11)), cash_delta=last["company"]["cash"]-first["company"]["cash"],
            rail_boarded=service["boarded"] if service else 0, rail_accepted=service["accepted"] if service else 0,
            rail_aboard=final["quantity"], rail_share=final["share"],
            observed_whole_unload_fare=service["observed_whole_unload_fare"] if service else 0),
        settlement_scope=dict(kind=SCOPE, window=window, maximum_windows=MAX_WINDOWS,
            tick_cap=MAX_TICKS, total_earned_ticks=total, ended=reason is not None, end_reason=reason))
    return phase, operation


def main():
    """Retain exactly one new native interval or inspect its original recorded invocation."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--window", type=int, required=True)
    parser.add_argument("--recover-run")
    parser.add_argument("--check-proposal", action="store_true")
    args = parser.parse_args()
    if args.check_proposal:
        assert args.window == 1 and args.recover_run is None
        check_proposal(args.configuration, ROOT / args.receipt)
        print("Exact proposal/source/callers verified; no native invocation or work.", flush=True)
        return
    output = ROOT / f"logs/surface-entry-arrival-settlement-{args.window:03d}-{args.configuration}.json"
    assert not output.exists()
    phase, operation = execute(args.configuration, ROOT / args.receipt, args.window, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
        phase=phase, operation=operation), indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), operation["settlement_scope"],
          operation["measured_cash_flow"], flush=True)


if __name__ == "__main__":
    main()
