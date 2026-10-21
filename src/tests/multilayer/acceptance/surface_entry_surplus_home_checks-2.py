"""Bind unchanged native surplus/home proposals and two ordinary-game reloads.

@file surface_entry_surplus_home_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVE.
Quoted resale values remain separate from actual cash and funded work.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_surplus_home_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))


def main():
    """Restore an actual unchanged original or bind both independently linked pure proposals."""
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
        receipt = ROOT / f"logs/surface-entry-surplus-home-query-{args.configuration}.json"
        result = restore(args.configuration, receipt, CURRENT["observe"], "surplus-home-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-farm-backhaul-verification.json", CURRENT["VERSIONS"],
            ("surplus-home-query",), "surplus-home-static", CURRENT["observe"],
            [Path(__file__), CALLER, CALLER.with_name("surface_surplus_home_native.py")],
            ROOT / "test-cache/surface-entry/surplus-home-products" if args.retain else None)
        result.update(_documentation=__doc__, actual_cash=40162, startup_reserve=40000,
            refund_banked=0, avoided_cost_realized=0, home_work_admitted=False, actual_sale_complete=False,
            working_gap=59838, incomplete_module_base_gap=469838, module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
