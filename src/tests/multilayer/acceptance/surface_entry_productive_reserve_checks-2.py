"""Bind the actual finite productive-recovery sequence and its enforced cash-growth stop.

@file surface_entry_productive_reserve_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGW.
Only completed native intervals are evidence. An unsuccessful burst cannot
authorize the remaining conditional windows or count unpaid cargo as cash.
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
    """Retain exact completed recovery/ordinary products and the literal stop/gap outcome."""
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
        result = restore(args.configuration,ROOT / args.receipt,observe,"productive-reserve-static",__doc__)
    else:
        names = tuple(f"productive-reserve-work-{i:02d}" for i in range(1,5))
        result = bind(ROOT / "logs/surface-entry-factory-goods-liquid-verification.json",[],names,
            "productive-reserve-static",observe,[Path(__file__),CALLER],
            ROOT / "test-cache/surface-entry/productive-reserve-products" if args.retain else None)
        totals = []
        # Four actual windows lost money, closing the next-burst gate. The
        # absence of05..12is deliberate scope enforcement, never missing work
        # or permission to replay a paid/earned earlier interval.
        for configuration in ("RelWithDebInfo","Debug"):
            operations = [json.loads((ROOT / f"logs/surface-entry-{name}-{configuration}.json").read_text(encoding="utf-8"))["operation"] for name in names]
            assert operations[0]["initial"]["company"]["cash"] == 60252
            assert operations[-1]["final"]["company"]["cash"] == 54619
            assert all(not op["work_refused"] for op in operations)
            flow = dict(ticks=sum(op["measured_cash_flow"]["ticks"] for op in operations),
                fares=sum(op["measured_cash_flow"]["actual_fares"] for op in operations),
                interest=sum(op["measured_cash_flow"]["interest"] for op in operations),
                other=sum(op["measured_cash_flow"]["other_operating_cost"] for op in operations),
                cash_delta=sum(op["measured_cash_flow"]["cash_delta"] for op in operations))
            assert flow == dict(ticks=7000,fares=34476,interest=25000,other=15109,cash_delta=-5633)
            assert not any((ROOT / f"logs/surface-entry-productive-reserve-work-{i:02d}-{configuration}.json").exists() for i in range(5,13))
            totals.append(flow)
        assert totals[0] == totals[1]
        result.update(_documentation=__doc__,measurement=totals[0],completed_windows=4,
                      stopped_for_non_growing_burst=True,next_burst_authorized=False,actual_reserve_restored=False,module_capital_complete=False)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
