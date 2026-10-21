"""Bind the finite first return-pickup followthrough without supplying another window.

@file surface_entry_return_passenger_follow_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZ.
Only actual ordered original windows are retained. Actual first pickup ends
this boundary; pending fares and accepted cargo never count as admission cash.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind,restore

CALLER = Path(__file__).with_name("surface_entry_return_passenger_work_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain the actual first-pickup followthrough and both ordinary reconstructions."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase",choices=("restore","bind"),required=True)
    parser.add_argument("--configuration",choices=("RelWithDebInfo","Debug"))
    parser.add_argument("--receipt",type=Path)
    parser.add_argument("--windows",type=int,choices=(1,2),default=1)
    parser.add_argument("--retain",action="store_true")
    parser.add_argument("--output",type=Path,required=True)
    args = parser.parse_args()
    output = ROOT/args.output
    assert output.resolve().is_relative_to(ROOT/"logs") and not output.exists()
    if args.phase == "restore":
        assert args.configuration and args.receipt
        result = restore(args.configuration,ROOT/args.receipt,observe,"return-passenger-follow-static",__doc__)
    else:
        names = tuple(f"return-passenger-follow-{i:02d}" for i in range(1,args.windows+1))
        # Once pickup or the actual cash goal ends an original window, no
        # subsequent window belongs to this explicitly bounded first cycle.
        for configuration in ("RelWithDebInfo","Debug"):
            for name in names[:-1]:
                value = json.loads((ROOT/f"logs/surface-entry-{name}-{configuration}.json").read_text(encoding="utf-8"))
                assert value["operation"]["actual_return_cargo"]["quantity"] == 0
                assert value["operation"]["final"]["company"]["cash"] < 100000
        result = bind(ROOT/"logs/surface-entry-return-passenger-work-verification.json",
            [ROOT/"test-cache/surface-entry/salt-window-source-versions/before-return-passenger-follow/retained.json"],
            names,"return-passenger-follow-static",observe,[Path(__file__),CALLER],
            ROOT/"test-cache/surface-entry/return-passenger-follow-products" if args.retain else None)
        result.update(_documentation=__doc__,actual_reserve_restored=False,module_capital_complete=False)
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
