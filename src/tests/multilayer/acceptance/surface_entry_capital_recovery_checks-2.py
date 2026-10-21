"""Bind unchanged current originals and genuine bounded recovery admission.

@file surface_entry_capital_recovery_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CR.
Neither the100000cash goal nor native pending freight promises future income.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_capital_recovery_checks-1.py")
observe = runpy.run_path(str(CALLER))["observe"]


def main():
    """Publish ordinary current reload or exact pure admission evidence."""
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
        result = restore(args.configuration, ROOT / args.receipt, observe, "capital-recovery-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-capital-renewal-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-capital-recovery-query/retained.json"],
            ("capital-recovery-query",), "capital-recovery-static", observe, [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry/capital-recovery-products" if args.retain else None)
        result.update(_documentation=__doc__, reserve_restored=False, module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
