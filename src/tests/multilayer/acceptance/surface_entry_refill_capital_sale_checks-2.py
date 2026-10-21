"""Bind actual redundant-refill retirement, banked cash and exact ordinary originals.

@file surface_entry_refill_capital_sale_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZF.
Only the actual sale changes ownership and cash. No tick or forecast fare
is earned, and all historical default ownership contracts remain strict.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind,restore

CALLER = Path(__file__).with_name("surface_entry_refill_capital_sale_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain actual retirement originals, both ordinary reconstructions and banked-funding proof."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase",choices=("restore","bind"),required=True)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"))
    parser.add_argument("--receipt",type=Path)
    parser.add_argument("--retain",action="store_true")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT/args.output
    assert output.resolve().is_relative_to(ROOT/"logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration and args.receipt
        result = restore(args.configuration,ROOT/args.receipt,observe,"refill-capital-sale-static",__doc__)
    else:
        result = bind(ROOT/"logs/surface-entry-refill-capital-verification.json",
            [ROOT/"test-cache/surface-entry/salt-window-source-versions/before-refill-capital-sale/retained.json",
             ROOT/"test-cache/surface-entry/salt-window-source-versions/before-refill-order-observer/retained.json"],
            ("refill-capital-sell",),"refill-capital-sale-static",observe,[Path(__file__),CALLER],
            ROOT/"test-cache/surface-entry/refill-capital-sale-products" if args.retain else None)
        result.update(_documentation=__doc__,actual_sale_complete=True,actual_refund_banked=2120,earned_ticks=0,
            actual_reserve_restored=False,module_capital_complete=False)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
