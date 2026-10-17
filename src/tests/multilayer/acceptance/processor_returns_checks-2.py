"""Bounded sequential scopes for actual paid product returns; no synthetic stock or poses.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12B
"""
import argparse
import importlib.util
import json
from pathlib import Path

from space_preview_checks import ROOT, identity

SPEC = importlib.util.spec_from_file_location("processor_returns_leaf", Path(__file__).with_name("processor_returns_checks-1.py"))
LEAF = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(LEAF)
SCOPES = {"native-a": ("native", LEAF.PHASES[:5]), "native-b": ("native", LEAF.PHASES[5:]),
          "own-a": ("resave", LEAF.PHASES[1:6]), "own-b": ("resave", LEAF.PHASES[6:]),
          "resume-a": ("native", LEAF.PHASES[2:7]), "resume-b": ("native", LEAF.PHASES[7:]),
          "focused": ("focused", ("inspected",))}


def main():
    """Execute at most six ordered native children using explicitly named predecessor selectors."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=SCOPES, required=True)
    parser.add_argument("--parents", type=Path, nargs="*", default=[])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists()
    parents = {}
    # A scope explicitly names its complete prior membership. Reject duplicate phase owners rather than selecting a newest or convenient save.
    for selector in args.parents:
        selected = LEAF.read(selector)
        assert selected["configuration"] == args.configuration and selected["complete"]
        for relative in selected["reports"]:
            path = ROOT / relative; proof = LEAF.read(path)
            assert proof["phase"] not in parents
            parents[proof["phase"]] = path
    operation, phases = SCOPES[args.scope]; reports = []
    for phase in phases:
        parent_phase = phase if operation == "resave" else LEAF.PARENTS.get(phase)
        parent = parents.get(parent_phase)
        assert operation == "focused" or (parent_phase is None) == (parent is None), phase
        report = LEAF.execute(args.configuration, operation, phase, parent)
        reports.append(report.relative_to(ROOT).as_posix())
        if args.scope.startswith("native"):
            parents[phase] = report
        print(f"passed {args.configuration} {args.scope} {phase}", flush=True)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, configuration=args.configuration, scope=args.scope,
                                          complete=True, parents=[identity(p.resolve()) for p in args.parents], reports=reports), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
