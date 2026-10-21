"""Bind one fully funded short continuation and exact ordinary native originals.

@file surface_entry_factory_goods_liquidity_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGVC.
Actual selected-cap work, cargo ownership, payment settlement and company cash
remain separate literal facts, with no forecast money or implicit next window.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind,restore

CALLER = Path(__file__).with_name("surface_entry_factory_goods_liquidity_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain actual selected work/ordinary owners and literal cash/rail-payment results."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase",choices=("restore","bind"),required=True)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"))
    parser.add_argument("--receipt",type=Path)
    parser.add_argument("--retain",action="store_true")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT / args.output
    assert output.resolve().is_relative_to(ROOT / "logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration and args.receipt
        result = restore(args.configuration,ROOT / args.receipt,observe,"factory-goods-liquid-static",__doc__)
    else:
        result = bind(ROOT / "logs/surface-entry-factory-goods-settlement-verification.json",
            [ROOT / "test-cache/surface-entry/salt-window-source-versions/before-factory-goods-liquidity/retained.json"],
            ("factory-goods-liquid-01",),"factory-goods-liquid-static",observe,[Path(__file__),CALLER],
            ROOT / "test-cache/surface-entry/factory-goods-liquid-products" if args.retain else None)
        flows = []
        for configuration in ("RelWithDebInfo","Debug"):
            value = json.loads((ROOT / f"logs/surface-entry-factory-goods-liquid-01-{configuration}.json").read_text(encoding="utf-8"))["operation"]
            assert not value["work_refused"] and value["measured_cash_flow"]["ceiling"] == 1000
            flows.append(value["measured_cash_flow"])
        assert flows[0] == flows[1]
        result.update(_documentation=__doc__,actual_rail_payment_posted=flows[0]["actual_rail_payment_posted"],
                      actual_reserve_restored=all(r["observation"]["company"]["cash"] >= 100000 for r in result["configurations"]),module_capital_complete=False)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
