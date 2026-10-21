"""Bind unchanged originals and their zero-purchase native freight-policy proposal.

@file surface_entry_freight_efficiency_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSB.
No measured savings, restored working reserve or paid module is claimed here.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_freight_efficiency_checks-1.py")
observe = runpy.run_path(str(CALLER))["observe"]


def main():
    """Publish focused ordinary reload or exact pure available-load admission."""
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
        result = restore(args.configuration, ROOT / args.receipt, observe, "freight-efficiency-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-reserve-recovery-partial-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-throughput-query/retained.json"],
            ("freight-efficiency-query",), "freight-efficiency-static", observe, [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry/freight-efficiency-products" if args.retain else None)
        result.update(_documentation=__doc__, changed_orders=0, actual_reserve_restored=False, module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
