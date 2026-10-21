"""Bind exact unchanged originals and their native module-capital admission.

@file surface_entry_module_capital_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CO.
Read-only proposals never prove paid income, completed renewals or module capital.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT
from surface_gateway_component_delivery_ledger import observe_propellant_delivery as observe
from surface_original_evidence import bind, restore


def main():
    """Publish focused ordinary reloads or the joined current exact artifact graph."""
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
        result = restore(args.configuration, ROOT / args.receipt, observe, "module-capital-static", __doc__)
    else:
        destination = ROOT / "test-cache/surface-entry/module-capital-products" if args.retain else None
        result = bind(ROOT / "logs/surface-entry-propellant-delivery-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-module-capital-query/retained.json"],
            ("module-capital-startup-query",), "module-capital-static", observe,
            [Path(__file__), Path(__file__).with_name("surface_entry_module_capital_checks-1.py")], destination)
        result.update(_documentation=__doc__, module_capital_complete=False, paid_improvement=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
