"""Bank only the real native refund for two empty additional surplus wagons.

@file surface_entry_surplus_recovery_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEB.
No original service, actual freight, builder, clock or reserve policy changes.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_original_evidence import verify
from surface_regional_ledger import TABLE
from surface_farm_backhaul_ledger import observe as old_mixed
from surface_surplus_recovery_ledger import observe
from surface_surplus_recovery_native import execute as native

HERE = Path(__file__).resolve().parent
PURE = runpy.run_path(str(HERE / "surface_entry_surplus_home_checks-1.py"))
VERSIONS = PURE["VERSIONS"] + [ROOT / "logs/surface-entry-surplus-recovery-before-edit.json"]


def execute(configuration, recover_run=None):
    """Perform or recover exactly one real zero-time surplus-only sale.

    @param configuration Independently linked isolated native configuration.
    @param recover_run Exact successful original invocation; never repeat a successful sale.
    @return Actual paid invoice, original packet/body conservation and fresh post-refund funding.
    """
    receipt = ROOT / f"logs/surface-entry-surplus-home-query-{configuration}.json"
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration
    prior = previous["operation"]
    assert prior["actual_cash"] == 40162 and not prior["actual_sale_complete"] and prior["refund_banked"] == 0
    for reference in prior["sources"] + [prior["application"]]:
        verify(reference, VERSIONS)
    source = ROOT / prior["output"]["path"]
    assert identity(source) == prior["output"] and json.loads(json.dumps(observe(source))) == prior["final"]
    attempt = ROOT / f"logs/surface-entry-surplus-recovery-pay-{configuration}-attempt.json"
    if recover_run is None:
        assert not attempt.exists(), "recover the precise original invocation instead of repeating a native sale"
        attempt.write_text(json.dumps(dict(_documentation=__doc__, receipt=identity(receipt), reserve=40000,
            original_cash=40162, expected_refund=1504, elapsed=0), indent=2)+"\n", encoding="utf-8")
    else:
        assert json.loads(attempt.read_text(encoding="utf-8"))["receipt"] == identity(receipt)
    files = [ROOT / reference["path"] for reference in prior["sources"] if verify(reference, VERSIONS)] + [
        Path(__file__), HERE / "surface_surplus_recovery_ledger.py", HERE / "surface_farm_backhaul_ledger.py",
        HERE / "surface_gateway_component_delivery_ledger.py", ROOT / "src/tests/surface_entry_mixed_farm_backhaul.h",
        ROOT / "src/tests/surface_entry_surplus_recovery.h"] + VERSIONS
    operation = native(configuration, "surplus-recovery-pay", receipt, observe,
        dict(paid="SURPLUS_RECOVERY_PAID", admission="SURPLUS_RECOVERY_ADMISSION", invoices="INVOICE"),
        files, recover_run=recover_run, work_limit=64)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == prior["final"]
    before, output = [ROOT / operation[name]["path"] for name in ("before", "output")]
    assert first["company"]["cash"] == 40162 and last["company"]["cash"] == 41666
    assert first["company"]["debt"] == last["company"]["debt"] == 5000000
    paid, = rows["paid"]
    invoice, = rows["invoices"]
    assert paid["refund_banked"] == 1504 and paid["invoice"] == invoice["cost"] == -1504
    assert paid["removed"] == 2 and paid["running_cost_saved"] == paid["elapsed"] == 0
    a, b = [TABLE(path, b"VEHS") for path in (before, output)]
    assert b.keys() == a.keys()-{58, 59}
    assert all(a[key] == b[key] for key in b)
    first_chunks, last_chunks = chunks(before)[1], chunks(output)[1]
    changed = {key for key in first_chunks if first_chunks[key] != last_chunks[key]}
    assert changed == {b"VEHS", b"PLYR"}, changed
    assert TABLE(before, b"DATE") == TABLE(output, b"DATE")
    balance = last["farm_balance"]
    assert balance["retired_livestock"] == [58, 59] and balance["original_wagons_owned"] == 32
    assert balance["train_bodies"] == 39 and balance["active_length"] == 112
    assert balance["livestock_capacity"] == 200 and balance["grain_capacity"] == 90 and balance["goods_capacity"] == 50
    assert first["totals"] == last["totals"] == {"STEL": 32, "GOOD": 50, "PROP": 38}
    rejected = False
    try:
        old_mixed(output)
    except AssertionError:
        rejected = True
    assert rejected, "historical mixed mode must still require its original additional reserve"
    assert all(row["cash"] == 41666 and row["reserve"] == 40000 and row["target"] == row["operating"]+40000
        and row["future_income_counted"] == row["avoided_cost_counted"] == row["unbanked_refund_counted"] == 0
        and not row["work_permission"] for row in rows["admission"])
    assert {row["ticks"] for row in rows["admission"] if row["funded"]} == {250, 64}
    operation.update(earned_ticks=0, actual_cash=41666, refund_banked=1504, actual_sale_complete=True,
        avoided_cost_realized=0, startup_reserve=40000, original_wagons_owned=32, historical_mixed_rejected=True,
        historical_versions=[identity(path) for path in VERSIONS],
        measured_cash_flow=dict(ticks=0, actual_fares=0, gross_capital_recovered=1504, cash_delta=1504),
        remaining_scope=["separately funded original builder home/lifecycle prerequisite", "actual first farm Goods payment",
            "real working/module/continuous-chain capital"])
    return operation


def main():
    """Retain the actual original paid boundary and its precise successful process."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--recover-run")
    args = parser.parse_args()
    output = ROOT / f"logs/surface-entry-surplus-recovery-pay-{args.configuration}.json"
    assert not output.exists()
    operation = execute(args.configuration, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
        phase="surplus-recovery-pay", operation=operation), indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), operation["rows"], flush=True)


if __name__ == "__main__":
    main()
