"""Bind the finite first Goods transfer followthrough without supplying extra work.

@file surface_entry_goods_rail_feeder_follow_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZD.
Only actual ordered original windows and explicit unchanged refusals are
retained. Actual first transfer, cash goal or refusal ends this bounded scope.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind,restore

CALLER = Path(__file__).with_name("surface_entry_goods_rail_feeder_work_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain the actual first-transfer sequence and both ordinary reconstructions."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase",choices=("restore","bind"),required=True)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"))
    parser.add_argument("--receipt",type=Path)
    parser.add_argument("--windows",type=int,choices=(1,2,3,4),default=1)
    parser.add_argument("--retain",action="store_true")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT/args.output
    assert output.resolve().is_relative_to(ROOT/"logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration and args.receipt
        result = restore(args.configuration,ROOT/args.receipt,observe,"goods-rail-feeder-follow-static",__doc__)
    else:
        names = tuple(f"goods-rail-feeder-follow-{i:02d}" for i in range(1,args.windows+1))
        # A successful real transfer, cash goal or full-funding refusal ends
        # this first-cycle boundary. A later interval must have its own plan
        # admission and cannot appear in the retained finite sequence.
        for configuration in ("RelWithDebInfo","Debug"):
            for name in names[:-1]:
                value = json.loads((ROOT/f"logs/surface-entry-{name}-{configuration}.json").read_text(encoding="utf-8"))["operation"]
                assert not value["work_refused"] and not value["measured_cash_flow"]["first_transfer_complete"]
                assert value["final"]["company"]["cash"] < 100000
        result = bind(ROOT/"logs/surface-entry-goods-rail-feeder-work-verification.json",
            [ROOT/"test-cache/surface-entry/salt-window-source-versions/before-goods-rail-feeder-follow/retained.json"],
            names,"goods-rail-feeder-follow-static",observe,[Path(__file__),CALLER],
            ROOT/"test-cache/surface-entry/goods-rail-feeder-follow-products" if args.retain else None)
        result.update(_documentation=__doc__,actual_reserve_restored=False,module_capital_complete=False)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
