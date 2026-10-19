"""Restore the original collected components and earn matching short continuations.

@file surface_entry_components_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CF.3.
Only the original branch is selected for subsequent real gateway deliveries.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_gateway_components_ledger import observe
from surface_gateway_replay import replay

DRIVER = runpy.run_path(str(Path(__file__).with_name("surface_entry_components_checks-1.py")))["execute"]


def main():
    """Restore and continue one actual build without overwriting prior original receipts."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--receipt", type=Path, required=True); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    result = replay(args.configuration, ROOT / args.receipt, "surface-entry-components", "components-service", DRIVER, observe)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
