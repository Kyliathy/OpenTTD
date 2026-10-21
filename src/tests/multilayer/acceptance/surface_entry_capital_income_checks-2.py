"""Accept genuine one-bus startup and measured income without claiming module capital.

@file surface_entry_capital_income_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CP.
Exact prior authorities and current original worlds retain their dated receipts.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_capital_income_checks-1.py")
observe = runpy.run_path(str(CALLER))["observe"]


def main():
    """Publish focused ordinary reload or joined actual additional service acceptance."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("restore", "bind"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration and args.receipt
        result = restore(args.configuration, ROOT / args.receipt, observe, "capital-income-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-module-capital-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-capital-income/retained.json"],
            ("capital-income-buy", "capital-income-work-01"), "capital-income-static", observe,
            [Path(__file__), CALLER], ROOT / "test-cache/surface-entry/capital-income-products" if args.retain else None)
        assert all(row["observation"]["capital_service"] is not None for row in result["configurations"])
        result.update(_documentation=__doc__, paid_additional_buses=1, module_capital_complete=False, renewal_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
