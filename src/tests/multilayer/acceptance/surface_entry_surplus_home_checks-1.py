"""Price only actual surplus wagons and an idle builder's genuine home prerequisites.

@file surface_entry_surplus_home_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVE.
No command executes, no refund is banked and no work is earned by this proposal.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_original_evidence import verify
from surface_farm_backhaul_ledger import observe
from surface_surplus_home_native import execute as native

HERE = Path(__file__).resolve().parent
PREVIOUS = runpy.run_path(str(HERE / "surface_entry_farm_backhaul_checks-1.py"))
VERSIONS = PREVIOUS["VERSIONS"] + [ROOT / "logs/surface-entry-surplus-home-before-edit.json"]


def execute(configuration, recover_run=None):
    """Inspect precisely one current original without spending or moving native assets.

    @param configuration Independently linked isolated native application.
    @param recover_run Exact successful pure invocation to inspect without repeating it.
    @return Unchanged original identities and explicit unbanked/funded native proposal rows.
    """
    receipt = ROOT / f"logs/surface-entry-farm-backhaul-work-004-{configuration}.json"
    previous = json.loads(receipt.read_text(encoding="utf-8"))
    assert previous["complete"] and previous["configuration"] == configuration
    prior = previous["operation"]
    assert prior["backhaul_scope"]["end_reason"] == "funding_refused" and prior["final"]["company"]["cash"] == 40162
    for reference in prior["sources"] + [prior["application"]]:
        verify(reference, VERSIONS)
    attempt = ROOT / f"logs/surface-entry-surplus-home-query-{configuration}-attempt.json"
    if recover_run is None:
        assert not attempt.exists(), "recover the exact successful invocation"
        attempt.write_text(json.dumps(dict(_documentation=__doc__, receipt=identity(receipt), paid=0, elapsed=0), indent=2)+"\n", encoding="utf-8")
    else:
        assert json.loads(attempt.read_text(encoding="utf-8"))["receipt"] == identity(receipt)
    files = [ROOT / reference["path"] for reference in prior["sources"] if verify(reference, VERSIONS)] + [
        Path(__file__), HERE / "surface_gateway_construction_ledger.py", HERE / "surface_gateway_resupply_ledger.py",
        HERE / "surface_gateway_material_ledger.py", ROOT / "src/tests/surface_entry_surplus_home.h",
        ROOT / "src/space_orders.cpp", ROOT / "src/vehicle_cmd.cpp", ROOT / "src/train_cmd.cpp"] + VERSIONS
    operation = native(configuration, "surplus-home-query", receipt, observe,
        dict(reserve="SURPLUS_HOME_RESERVE", builder="SURPLUS_HOME_BUILDER", admission="SURPLUS_HOME_ADMISSION"),
        files, recover_run=recover_run, work_limit=64)
    first, last, rows = operation["initial"], operation["final"], operation["rows"]
    assert json.loads(json.dumps(first)) == prior["final"] and first == last
    before, output = [ROOT / operation[name]["path"] for name in ("before", "output")]
    assert chunks(before)[1] == chunks(output)[1]
    reserve, = rows["reserve"]
    builder, = rows["builder"]
    assert reserve["available"] and reserve["owned"] == 2 and reserve["empty"] and reserve["invoice"] < 0
    assert reserve["unbanked_refund"] == -reserve["invoice"] and reserve["refund_banked"] == reserve["running_cost_saved"] == reserve["elapsed"] == 0
    assert builder["visit_available"] and builder["visit_invoice"] == 0 and not builder["remote_sale_available"]
    assert builder["replacement_invoice"] == 100000 and builder["replacement_steel"] == 4 and builder["replacement_goods"] == 8
    assert builder["replacement_work"] == 148 and not builder["replacement_launch_priced"]
    assert builder["refund_banked"] == builder["elapsed"] == 0 and builder["idle_per_day"] == 100
    assert all(row["reserve"] == 40000 and row["target"] == row["operating"]+40000
        and row["refund_counted"] == row["avoided_cost_counted"] == row["future_income_counted"] == 0
        and not row["work_permission"] for row in rows["admission"])
    assert not any(row["funded"] for row in rows["admission"])
    operation.update(earned_ticks=0, actual_cash=40162, refund_banked=0, avoided_cost_realized=0,
        home_work_admitted=False, actual_sale_complete=False, startup_reserve=40000,
        historical_versions=[identity(path) for path in VERSIONS],
        remaining_scope=["actual surplus-only capital recovery", "fully funded genuine builder-home lifecycle prerequisite",
            "actual first farm Goods payment", "real working/module/continuous-chain capital"])
    return operation


def main():
    """Retain the exact original pure proposal and its native process identities."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--recover-run")
    args = parser.parse_args()
    output = ROOT / f"logs/surface-entry-surplus-home-query-{args.configuration}.json"
    assert not output.exists()
    operation = execute(args.configuration, args.recover_run)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
        phase="surplus-home-query", operation=operation), indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), operation["rows"], flush=True)


if __name__ == "__main__":
    main()
