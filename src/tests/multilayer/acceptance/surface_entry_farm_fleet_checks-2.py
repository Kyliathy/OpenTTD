"""Bind actual extra-head refusals and the fully funded retained-wagon balance.

@file surface_entry_farm_fleet_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGG.
All current original bodies remain coupled and unchanged until a later producer.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

DIRECTORY = Path(__file__).resolve().parent
CALLER = DIRECTORY / "surface_entry_farm_balance_checks-1.py"
FLEET = DIRECTORY / "surface_entry_farm_fleet_checks-1.py"
observe = runpy.run_path(str(CALLER))["observe"]


def main():
    """Publish ordinary reload or exact refused-head/retained-wagon admission."""
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
        result = restore(args.configuration, ROOT / args.receipt, observe, "farm-fleet-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-farm-upgrade-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-farm-fleet-query/retained.json",
             ROOT / "test-cache/surface-entry/salt-window-source-versions/before-farm-balance-query/retained.json"],
            ("farm-fleet-query", "farm-balance-query"), "farm-fleet-static", observe, [Path(__file__), CALLER, FLEET],
            ROOT / "test-cache/surface-entry/farm-fleet-products" if args.retain else None)
        result.update(_documentation=__doc__, actual_reserve_restored=False, module_capital_complete=False, purchases=0)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
