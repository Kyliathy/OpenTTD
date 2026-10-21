"""Bind pure redundant-refill capital quotes and exact ordinary originals.

@file surface_entry_refill_capital_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZE.
Actual39remains owned and its refund remains unbanked. Neither conditional
admission nor a retained quote grants work or closes the original funding gap.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind,restore

CALLER = Path(__file__).with_name("surface_entry_refill_capital_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain unchanged actual pure originals and both ordinary reconstructions."""
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
        result = restore(args.configuration,ROOT/args.receipt,observe,"refill-capital-static",__doc__)
    else:
        result = bind(ROOT/"logs/surface-entry-goods-rail-feeder-follow-verification.json",
            [ROOT/"test-cache/surface-entry/salt-window-source-versions/before-refill-capital/retained.json"],
            ("refill-capital-query",),"refill-capital-static",observe,[Path(__file__),CALLER],
            ROOT/"test-cache/surface-entry/refill-capital-products" if args.retain else None)
        admitted = [json.loads((ROOT/f"logs/surface-entry-refill-capital-query-{c}.json").read_text(encoding="utf-8"))["operation"]["refill_retirement_admitted"]
                    for c in ("RelWithDebInfo","Debug")]
        assert admitted[0] == admitted[1]
        result.update(_documentation=__doc__,refill_retirement_admitted=admitted[0],actual_sale_complete=False,
            actual_reserve_restored=False,module_capital_complete=False)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
