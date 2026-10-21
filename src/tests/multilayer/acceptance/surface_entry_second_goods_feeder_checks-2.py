"""Bind second paid Goods carrier admission and exact unchanged ordinary reloads.

@file surface_entry_second_goods_feeder_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZK.
This pure proposal uses no purchase, disposal, cargo grant or simulation tick.
Its loaded25cargo and all historical route/stock defaults remain intact.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind,restore

CALLER = Path(__file__).with_name("surface_entry_second_goods_feeder_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain pure second-feeder quotes and both ordinary reconstructions."""
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
        result = restore(args.configuration,ROOT/args.receipt,observe,"second-goods-feeder-static",__doc__)
    else:
        result = bind(ROOT/"logs/surface-entry-constructor-capital-verification.json",
            [ROOT/"test-cache/surface-entry/salt-window-source-versions/before-second-goods-feeder/retained.json"],
            ("second-goods-feeder-query",),"second-goods-feeder-static",observe,[Path(__file__),CALLER],
            ROOT/"test-cache/surface-entry/second-goods-feeder-products" if args.retain else None)
        operations = [json.loads((ROOT/f"logs/surface-entry-second-goods-feeder-query-{c}.json").read_text(encoding="utf-8"))["operation"]
                      for c in ("RelWithDebInfo","Debug")]
        assert all(op["second_feeder_admitted"] and op["funded_cap"] == 250 for op in operations)
        result.update(_documentation=__doc__,second_feeder_admitted=True,actual_order_change_complete=False,
            actual_reserve_restored=False,module_capital_complete=False,working_gap=50750,incomplete_module_base_gap=460750)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
