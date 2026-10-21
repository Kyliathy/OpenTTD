"""Bind one genuinely funded retained farm-to-factory cycle and ordinary originals.

@file surface_entry_farm_cycle_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGI.
Actual cash alone can close the reserve; original materials/assets stay owned.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_farm_cycle_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain exact cycle/ordinary evidence and the truthful reserve endpoint."""
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
        result = restore(args.configuration, ROOT / args.receipt, observe, "farm-cycle-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-farm-rebalance-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-farm-cycle/retained.json"],
            ("farm-cycle-query", "farm-cycle-work-01", "farm-cycle-work-02"), "farm-cycle-static", observe,
            [Path(__file__), CALLER], ROOT / "test-cache/surface-entry/farm-cycle-products" if args.retain else None)
        restored = [row["observation"]["company"]["cash"] >= 100000 for row in result["configurations"]]
        assert restored[0] == restored[1]
        result.update(_documentation=__doc__, actual_reserve_restored=restored[0], module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
