"""Bind actual joint feeder rail transport, whole settlement and ordinary originals.

@file surface_entry_joint_goods_rail_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZM.
Both real truck cohorts may merge; quantities, virtual share and whole native
posted payment remain distinct, with every incomplete capital gate explicit.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind,restore

CALLER = Path(__file__).with_name("surface_entry_joint_goods_rail_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain only a legitimately ended actual joint rail sequence and both ordinary reconstructions."""
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
        result = restore(args.configuration,ROOT/args.receipt,observe,"joint-goods-rail-static",__doc__)
    else:
        assert 1 <= args.windows <= CURRENT["MAX_WINDOWS"]
        names = tuple(f"joint-goods-rail-{i:02d}" for i in range(1,args.windows+1))
        finals = []
        # Only a consecutive original sequence may reach the actual posted
        # payment, cash, refusal or eight-window stop. Earlier endpoints must
        # still be open, so a closed scope cannot silently continue.
        for configuration in ("RelWithDebInfo","Debug"):
            for i,name in enumerate(names,1):
                value = json.loads((ROOT/f"logs/surface-entry-{name}-{configuration}.json").read_text(encoding="utf-8"))["operation"]
                assert value["rail_scope"]["window"] == i and value["rail_scope"]["ended"] == (i == args.windows)
            finals.append(value)
        result = bind(ROOT/"logs/surface-entry-second-goods-feeder-work-verification.json",
            [ROOT/"test-cache/surface-entry/salt-window-source-versions/before-joint-goods-rail/retained.json"],
            names,"joint-goods-rail-static",observe,[Path(__file__),CALLER],
            ROOT/"test-cache/surface-entry/joint-goods-rail-products" if args.retain else None)
        assert finals[0]["rail_scope"] == finals[1]["rail_scope"]
        result.update(_documentation=__doc__,rail_scope=finals[0]["rail_scope"],rail_payment_posted=finals[0]["rail_payment_posted"],
            actual_reserve_restored=all(v["final"]["company"]["cash"] >= 100000 for v in finals),module_capital_complete=False,
            working_gap=max(0,100000-finals[0]["final"]["company"]["cash"]),
            incomplete_module_base_gap=max(0,510000-finals[0]["final"]["company"]["cash"]))
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
