"""Bind unchanged startup-reserve comparisons and both ordinary native reconstructions.

@file surface_entry_joint_settlement_budget_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZN.
The proposed short settlement floor supplies no cash, stock, work or setting
change; original working/module reserves and unassisted-chain gates stay open.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind,restore

CALLER = Path(__file__).with_name("surface_entry_joint_settlement_budget_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain only current pure comparison originals and both ordinary reloads."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase",choices=("restore","bind"),required=True)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"))
    parser.add_argument("--receipt",type=Path)
    parser.add_argument("--retain",action="store_true")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT/args.output
    assert output.resolve().is_relative_to(ROOT/"logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration and args.receipt
        result = restore(args.configuration,ROOT/args.receipt,observe,"joint-settlement-budget-static",__doc__)
    else:
        result = bind(ROOT/"logs/surface-entry-joint-goods-rail-verification.json",
            [ROOT/"test-cache/surface-entry/salt-window-source-versions/before-joint-settlement-budget/retained.json"],
            ("joint-settlement-budget-query",),"joint-settlement-budget-static",observe,[Path(__file__),CALLER],
            ROOT/"test-cache/surface-entry/joint-settlement-budget-products" if args.retain else None)
        operations = [json.loads((ROOT/f"logs/surface-entry-joint-settlement-budget-query-{c}.json").read_text(encoding="utf-8"))["operation"]
                      for c in ("RelWithDebInfo","Debug")]
        assert all(op["candidate_settlement_admitted"] and op["candidate_reserve"] == 30000 for op in operations)
        assert operations[0]["complete_startup_bill"] == operations[1]["complete_startup_bill"]
        result.update(_documentation=__doc__,candidate_settlement_admitted=True,candidate_reserve=30000,
            complete_startup_bill=operations[0]["complete_startup_bill"],actual_work_complete=False,
            actual_reserve_restored=False,module_capital_complete=False,working_gap=55828,incomplete_module_base_gap=465828)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
