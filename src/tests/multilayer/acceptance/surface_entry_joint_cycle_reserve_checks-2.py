"""Bind original cycle-based reserve work and exact ordinary native reloads.

@file surface_entry_joint_cycle_reserve_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZP.
Actual posted-cycle company cash growth alone admits another cycle. Native
code, historical floor defaults and all original physical owners stay intact.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT
from surface_original_evidence import bind,restore

CALLER = Path(__file__).with_name("surface_entry_joint_cycle_reserve_checks-1.py")
CURRENT = runpy.run_path(str(CALLER))
observe = CURRENT["observe"]


def main():
    """Retain only a closed actual cycle-measurement sequence and both ordinary reconstructions."""
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
        result = restore(args.configuration,ROOT/args.receipt,observe,"joint-cycle-reserve-static",__doc__)
    else:
        assert 1 <= args.windows <= CURRENT["MAX_WINDOWS"]
        names = tuple(f"joint-cycle-reserve-{i:03d}" for i in range(1,args.windows+1))
        finals = []
        # Exact predecessor identities and namespace prevent reused native
        # phase names from joining foreign or already closed saving scopes.
        # Only the actual final goal/refusal/cycle/tick/window stop may end it.
        for configuration in ("RelWithDebInfo","Debug"):
            for i,name in enumerate(names,1):
                value = json.loads((ROOT/f"logs/surface-entry-{name}-{configuration}.json").read_text(encoding="utf-8"))["operation"]
                scope = value["working_scope"]
                assert scope["kind"] == CURRENT["SCOPE"] and scope["window"] == i and scope["ended"] == (i == args.windows)
            finals.append(value)
        result = bind(ROOT/"logs/surface-entry-joint-settlement-work-verification.json",[],
            names,"joint-cycle-reserve-static",observe,[Path(__file__),CALLER],
            ROOT/"test-cache/surface-entry/joint-cycle-reserve-products" if args.retain else None)
        assert finals[0]["working_scope"] == finals[1]["working_scope"]
        result.update(_documentation=__doc__,working_scope=finals[0]["working_scope"],startup_reserve=40000,
            actual_reserve_restored=all(v["final"]["company"]["cash"] >= 100000 for v in finals),module_capital_complete=False,
            working_gap=max(0,100000-finals[0]["final"]["company"]["cash"]),
            incomplete_module_base_gap=max(0,510000-finals[0]["final"]["company"]["cash"]))
    output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(output.relative_to(ROOT).as_posix(),flush=True)


if __name__ == "__main__":
    main()
