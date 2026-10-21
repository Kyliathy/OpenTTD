"""Bind pure native productive quotes and both exact ordinary paused reloads.

@file surface_entry_rail_productivity_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZV.
Original native financial applications and all paid owners remain immutable;
pure affordability never substitutes for a real productive-service consumer.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_rail_productivity_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))


def main():
    """Restore one unchanged native original or bind both pure quotes and protected products."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("restore", "bind"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration
        receipt = ROOT / f"logs/surface-entry-rail-productivity-{args.configuration}.json"
        result = restore(args.configuration, receipt, CURRENT["observe"], "rail-productivity-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-productive-admission-verification.json", [CURRENT["VERSIONS"], ROOT / "logs/surface-entry-rail-productivity-recorder-versions.json"],
            ("rail-productivity",), "rail-productivity-static", CURRENT["observe"],
            [Path(__file__), CALLER, CALLER.with_name("surface_rail_productivity_native.py")],
            ROOT / "test-cache/surface-entry/rail-productivity-products" if args.retain else None)
        result.update(_documentation=__doc__, native_ticks_earned=0, purchases=0, startup_reserve=40000,
            final_cash=45080, working_gap=54920, incomplete_module_base_gap=464920,
            productive_consumer_admitted=False, module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
