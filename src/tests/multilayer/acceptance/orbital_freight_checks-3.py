"""Replay one saved module-flight phase in a focused bounded burst.

@file orbital_freight_checks-3.py Exact26-family continuations from preburn/ascent/admission/unload/turnaround.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context

shared = runpy.run_path(str(Path(__file__).with_name("orbital_freight_checks-1.py")))
native, EXACT = shared["native"], shared["EXACT"]


def main():
    """Resume one declared native phase and compare every affected owner with uninterrupted completion."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--completion", type=Path, required=True, help="Exact preceding orbital-module-flight-complete.json")
    parser.add_argument("--phase", required=True, choices=("preburn", "ascent", "approach", "unloaded", "turnaround"))
    args = parser.parse_args()
    predecessor = args.completion.resolve()
    assert predecessor.is_relative_to(ROOT)
    completed = json.loads(predecessor.read_text(encoding="utf-8"))
    assert completed["configuration"] == args.configuration
    preparation = ROOT / completed["preparation"]["path"]
    assert identity(preparation) == completed["preparation"]
    prepared = json.loads(preparation.read_text(encoding="utf-8"))
    checkpoint = (prepared if args.phase in ("preburn", "ascent") else completed)[args.phase]
    source, complete = ROOT / checkpoint["path"], ROOT / completed["complete"]["path"]
    assert identity(source) == checkpoint and identity(complete) == completed["complete"]
    context = current_run_context("space", "module-flight-replay-" + args.phase)
    resumed = native(context.for_feature("space", "module-flight-resume-" + args.phase), args.configuration, "continue", source)
    expected, actual = chunks(complete)[1], chunks(resumed)[1]
    # Exact native continuation checks cover every packet/age/share, paid hull, queue/flight receipt, company and original utility owner.
    for family in EXACT:
        assert expected[family] == actual[family], family
    paused = context.for_feature("space", "module-flight-paused-" + args.phase)
    ordinary(paused, {"name": "module-flight-" + args.phase, "snapshot": checkpoint}, args.configuration)
    resaved = paused.saves / "ordinary-resave.sav"
    expected, actual = chunks(source)[1], chunks(resaved)[1]
    # Saving the unresolved arrival preserves the existing in-transit hull's sole request and original freight; no loading callback may advance it.
    for family in EXACT:
        assert expected[family] == actual[family], family
    assert identity(source) == checkpoint and identity(complete) == completed["complete"]
    report = context.reports / "orbital-module-flight-replay.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group11; bounded exact interrupted and paused module-flight persistence.",
        "configuration": args.configuration, "phase": args.phase, "completion": identity(predecessor), "source": checkpoint,
        "continued": identity(resumed), "ordinary_resave": identity(resaved), "exact_owner_families": len(EXACT)}, indent=2) + "\n", encoding="utf-8")
    print(f"Replayed module flight/{args.phase}: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
