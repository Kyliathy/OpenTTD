"""Bind real Goods trade, strict original supply custody and ordinary reloads.

@file surface_entry_goods_trade_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGB.
Actual working-reserve and delivery outcomes remain independent observations.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind, restore

CALLER = Path(__file__).with_name("surface_entry_goods_trade_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Publish exact Goods trade receipts, physical stock and ordinary native reloads."""
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
        result = restore(args.configuration, ROOT / args.receipt, observe, "goods-trade-static", __doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-goods-income-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-goods-trade/retained.json"],
            ("goods-trade-start", "goods-trade-work-01"), "goods-trade-static", observe, [Path(__file__), CALLER],
            ROOT / "test-cache/surface-entry/goods-trade-products" if args.retain else None)
        restored = []
        for row in result["configurations"]:
            assert CURRENT["strict_default_rejects"](ROOT / row["output"]["path"])
            restored.append(row["observation"]["company"]["cash"] >= 100000)
        assert restored[0] == restored[1]
        result.update(_documentation=__doc__, actual_reserve_restored=restored[0], strict_default_rejected=True, module_capital_complete=False)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
