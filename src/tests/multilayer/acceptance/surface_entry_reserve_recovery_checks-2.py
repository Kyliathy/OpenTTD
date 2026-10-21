"""Bind genuine restored working reserve without claiming the larger module base.

@file surface_entry_reserve_recovery_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CS.
Every retained partial interval contributes its original exact work and books.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_reserve_recovery_checks-1.py")
observe = runpy.run_path(str(CALLER))["observe"]


def main():
    """Publish focused ordinary reload or actual working-reserve restoration acceptance."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("restore", "bind"), required=True)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--windows", type=int, choices=range(1, 9), default=1)
    parser.add_argument("--retain", action="store_true")
    parser.add_argument("--partial", action="store_true", help="Bind actual unfinished recovery without claiming the100000goal.")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration and args.receipt
        scenario = "reserve-recovery-partial-static" if args.partial else "reserve-recovery-static"
        result = restore(args.configuration, ROOT / args.receipt, observe, scenario, __doc__)
    else:
        scenario = "reserve-recovery-partial-static" if args.partial else "reserve-recovery-static"
        directory = "reserve-recovery-partial-products" if args.partial else "reserve-recovery-products"
        result = bind(ROOT / "logs/surface-entry-capital-recovery-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-capital-recovery-work/retained.json"],
            tuple(f"reserve-recovery-work-{index:02d}" for index in range(1, args.windows + 1)),
            scenario, observe, [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry" / directory if args.retain else None)
        # A qualified partial proof preserves actual loss and unfinished cash
        # recovery; it cannot authorize the default restored-reserve claim.
        if args.partial:
            assert all(row["observation"]["company"]["cash"] == 80157 for row in result["configurations"])
        else:
            assert all(row["observation"]["company"]["cash"] >= 100000 for row in result["configurations"])
        result.update(_documentation=__doc__, working_reserve_goal=100000, actual_reserve_restored=not args.partial, module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
