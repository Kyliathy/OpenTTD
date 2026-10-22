"""Restore genuine additional components stored in the original paid launch center.

@file surface_entry_component_delivery_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CG.4.
Only the independently continued original branch supplies later fuel/capital work.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_gateway_component_delivery_ledger import observe_retained as observe
from surface_gateway_replay import replay

DRIVER = runpy.run_path(str(Path(__file__).with_name("surface_entry_component_delivery_checks-1.py")))["execute"]


def main():
    """Ordinarily restore one original world and compare matching actual3000tick continuations."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--receipt", type=Path, required=True); parser.add_argument("--output", type=Path, required=True); args = parser.parse_args()
    output = ROOT / args.output; assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    result = replay(args.configuration, ROOT / args.receipt, "surface-entry-component-delivery", "component-delivery-service", DRIVER, observe)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
