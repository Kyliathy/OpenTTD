"""Bind pure actual Goods rail-feeder admission and exact ordinary originals.

@file surface_entry_goods_rail_feeder_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZB.
The unchanged original packets, native command estimates and complete
operating prices decide whether a later separate paid consumer is admitted.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind,restore

CALLER = Path(__file__).with_name("surface_entry_goods_rail_feeder_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain the unchanged pure originals and ordinary reloads with literal admission."""
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
        result = restore(args.configuration,ROOT/args.receipt,observe,"goods-rail-feeder-static",__doc__)
    else:
        result = bind(ROOT/"logs/surface-entry-return-passenger-delivery-verification.json",
            [ROOT/"test-cache/surface-entry/salt-window-source-versions/before-goods-rail-feeder/retained.json"],
            ("goods-rail-feeder-query",),"goods-rail-feeder-static",observe,[Path(__file__),CALLER],
            ROOT/"test-cache/surface-entry/goods-rail-feeder-products" if args.retain else None)
        admitted = [json.loads((ROOT/f"logs/surface-entry-goods-rail-feeder-query-{c}.json").read_text(encoding="utf-8"))["operation"]["goods_rail_feeder_admitted"]
                    for c in ("RelWithDebInfo","Debug")]
        assert admitted[0] == admitted[1]
        result.update(_documentation=__doc__,goods_rail_feeder_admitted=admitted[0],actual_reserve_restored=False,module_capital_complete=False)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
