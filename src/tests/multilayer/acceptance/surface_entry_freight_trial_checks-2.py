"""Bind actual available-load freight operation and retain its genuine financial result.

@file surface_entry_freight_trial_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSC.
The measured trial can remain partial; no reserve or module goal is fabricated.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_freight_trial_checks-1.py")
observe = runpy.run_path(str(CALLER))["observe"]


def main():
    """Publish focused ordinary reload or exact current available-load trial evidence."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("restore", "bind"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--windows", type=int, choices=range(1, 5), default=1)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration and args.receipt
        result = restore(args.configuration, ROOT / args.receipt, observe, "freight-trial-static", __doc__)
    else:
        names = ("freight-trial-change",) + tuple(f"freight-trial-work-{index:02d}" for index in range(1, args.windows + 1))
        result = bind(ROOT / "logs/surface-entry-freight-efficiency-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-throughput-work/retained.json",
             ROOT / "test-cache/surface-entry/salt-window-source-versions/before-short-throughput-work/retained.json"],
            names, "freight-trial-static", observe, [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry/freight-trial-products" if args.retain else None)
        restored = [row["observation"]["company"]["cash"] >= 100000 for row in result["configurations"]]
        assert restored[0] == restored[1]
        result.update(_documentation=__doc__, changed_pickup_orders=3, actual_reserve_restored=restored[0], module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
