"""Bind actual surplus-only capital recovery and exact ordinary reloads.

@file surface_entry_surplus_recovery_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEB.
Recovered paid capital is not freight income or an avoided running charge.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_surplus_recovery_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))


def main():
    """Restore one actual post-sale original or bind both real zero-time sale boundaries."""
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
        receipt = ROOT / f"logs/surface-entry-surplus-recovery-pay-{args.configuration}.json"
        result = restore(args.configuration, receipt, CURRENT["observe"], "surplus-recovery-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-surplus-home-verification.json", CURRENT["VERSIONS"],
            ("surplus-recovery-pay",), "surplus-recovery-static", CURRENT["observe"],
            [Path(__file__), CALLER, CALLER.with_name("surface_surplus_recovery_native.py")],
            ROOT / "test-cache/surface-entry/surplus-recovery-products" if args.retain else None)
        result.update(_documentation=__doc__, actual_cash=41666, startup_reserve=40000, original_wagons_owned=32,
            refund_banked=1504, avoided_cost_realized=0, actual_sale_complete=True, historical_mixed_rejected=True,
            working_gap=58334, incomplete_module_base_gap=468334, module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
