"""Bind pure native productive quotes and both exact ordinary paused reloads.

@file surface_entry_productive_admission_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZU.
Original native financial applications and all paid owners remain immutable;
pure affordability never substitutes for a real productive-service consumer.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_productive_admission_checks-1.py")
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
        receipt = ROOT / f"logs/surface-entry-productive-admission-{args.configuration}.json"
        result = restore(args.configuration, receipt, CURRENT["observe"], "productive-admission-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-arrival-settlement-verification.json", [],
            ("productive-admission",), "productive-admission-static", CURRENT["observe"],
            [Path(__file__), CALLER, CALLER.with_name("surface_productive_native.py")],
            ROOT / "test-cache/surface-entry/productive-admission-products" if args.retain else None)
        result.update(_documentation=__doc__, native_ticks_earned=0, purchases=0, startup_reserve=40000,
            final_cash=45080, working_gap=54920, incomplete_module_base_gap=464920,
            productive_consumer_admitted=False, module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
