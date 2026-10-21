"""Bind actual finite Goods rail-cohort delivery/payment originals and ordinary reloads.

@file surface_entry_goods_rail_delivery_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZH.
Actual first13acceptance and native whole unload payment remain separate from
other fares, virtual shares and the original still-unmet capital/working gates.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind,restore

CALLER = Path(__file__).with_name("surface_entry_goods_rail_delivery_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain only actual original rail windows with exact ordinary and cash evidence."""
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
        result = restore(args.configuration,ROOT/args.receipt,observe,"goods-rail-delivery-static",__doc__)
    else:
        names = tuple(f"goods-rail-delivery-{i:02d}" for i in range(1,args.windows+1))
        # A real posted new-cohort unload, actual cash goal or full-funding
        # refusal ends this scope. No later window or unrelated payment may
        # silently extend the finite original sequence.
        for configuration in ("RelWithDebInfo","Debug"):
            for name in names[:-1]:
                value = json.loads((ROOT/f"logs/surface-entry-{name}-{configuration}.json").read_text(encoding="utf-8"))["operation"]
                assert not value["work_refused"] and not value["rail_payment_posted"] and value["final"]["company"]["cash"] < 100000
        result = bind(ROOT/"logs/surface-entry-goods-rail-transfer-verification.json",
            [ROOT/"test-cache/surface-entry/salt-window-source-versions/before-goods-rail-delivery/retained.json"],
            names,"goods-rail-delivery-static",observe,[Path(__file__),CALLER],
            ROOT/"test-cache/surface-entry/goods-rail-delivery-products" if args.retain else None)
        finals = [json.loads((ROOT/f"logs/surface-entry-{names[-1]}-{c}.json").read_text(encoding="utf-8"))["operation"]
                  for c in ("RelWithDebInfo","Debug")]
        assert finals[0]["rail_payment_posted"] == finals[1]["rail_payment_posted"]
        result.update(_documentation=__doc__,rail_payment_posted=finals[0]["rail_payment_posted"],
            actual_reserve_restored=all(v["final"]["company"]["cash"] >= 100000 for v in finals),module_capital_complete=False)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
