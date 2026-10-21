"""Bind actual second-feeder commands, first-transfer windows and ordinary reloads.

@file surface_entry_second_goods_feeder_work_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZL.
Only actual original chronological windows are accepted; physical transfer
and virtual share cannot establish final rail payment or restore cash reserves.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind,restore

CALLER = Path(__file__).with_name("surface_entry_second_goods_feeder_work_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain only the actual finite second-feeder sequence and both ordinary reconstructions."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase",choices=("restore","bind"),required=True)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"))
    parser.add_argument("--receipt",type=Path)
    parser.add_argument("--windows",type=int,default=1)
    parser.add_argument("--retain",action="store_true")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT/args.output
    assert output.resolve().is_relative_to(ROOT/"logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration and args.receipt
        result = restore(args.configuration,ROOT/args.receipt,observe,"second-goods-feeder-work-static",__doc__)
    else:
        assert 1 <= args.windows <= CURRENT["MAX_WINDOWS"]
        names = ("second-goods-feeder-orders",)+tuple(f"second-goods-feeder-work-{i:02d}" for i in range(1,args.windows+1))
        finals = []
        # Every previous window must remain open and strictly consecutive;
        # only an actual goal, transfer, refusal or hard12cap closes the last.
        for configuration in ("RelWithDebInfo","Debug"):
            for i,name in enumerate(names[1:],1):
                value = json.loads((ROOT/f"logs/surface-entry-{name}-{configuration}.json").read_text(encoding="utf-8"))["operation"]
                assert value["work_scope"]["window"] == i and value["work_scope"]["ended"] == (i == args.windows)
            finals.append(value)
        result = bind(ROOT/"logs/surface-entry-second-goods-feeder-verification.json",
            [ROOT/"test-cache/surface-entry/salt-window-source-versions/before-second-goods-feeder-work/retained.json"],
            names,"second-goods-feeder-work-static",observe,[Path(__file__),CALLER],
            ROOT/"test-cache/surface-entry/second-goods-feeder-work-products" if args.retain else None)
        assert finals[0]["work_scope"] == finals[1]["work_scope"]
        result.update(_documentation=__doc__,first_25_transfer_complete=finals[0]["first_25_transfer_complete"],
            work_scope=finals[0]["work_scope"],actual_reserve_restored=all(v["final"]["company"]["cash"] >= 100000 for v in finals),
            module_capital_complete=False,working_gap=max(0,100000-finals[0]["final"]["company"]["cash"]),
            incomplete_module_base_gap=max(0,510000-finals[0]["final"]["company"]["cash"]))
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
