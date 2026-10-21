"""Bind the default-reserve loaded Goods arrival and ordinary paused reloads.

@file surface_entry_loaded_goods_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZR.
Original paid packet/financial owners and actual native preflight remain authoritative.
"""
import argparse
import json
from pathlib import Path
import runpy
from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_loaded_goods_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))


def main():
    """Restore one exact original or bind both paid default-reserve arrival products."""
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
        receipt = ROOT / ("logs/surface-entry-loaded-goods-" + args.configuration + ".json")
        result = restore(args.configuration, receipt, CURRENT["observe"], "loaded-goods-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-reserve-bridge-verification.json", [],
            ("loaded-goods",), "loaded-goods-static", CURRENT["observe"], [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry/loaded-goods-products" if args.retain else None)
        finals = [row["observation"] for row in result["configurations"]]
        assert finals[0]["company"] == finals[1]["company"]
        cash = finals[0]["company"]["cash"]
        result.update(_documentation=__doc__, startup_reserve=40000, final_cash=cash,
            working_gap=max(0,100000-cash), incomplete_module_base_gap=max(0,510000-cash),
            module_capital_complete=False)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
