"""Verify actual pad contention and whole over-capacity hold refusal with a second paid rocket.

@file orbital_freight_checks-8.py Real commissioning, retained active pad and independent paused native owners.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11C.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context

shared = runpy.run_path(str(Path(__file__).with_name("orbital_freight_checks-4.py")))
native, EXACT = shared["native"], shared["EXACT"]


def main():
    """Commission from real Surface inputs while the original rocket owns the pad, then refuse a second fee and an over-capacity hold."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--preparation", type=Path, required=True, help="Exact preceding positive preparation report")
    args = parser.parse_args()
    predecessor = args.preparation.resolve()
    assert predecessor.is_relative_to(ROOT)
    prepared = json.loads(predecessor.read_text(encoding="utf-8"))
    assert prepared["configuration"] == args.configuration
    source = ROOT / prepared["preburn"]["path"]
    assert identity(source) == prepared["preburn"]
    context = current_run_context("space", "module-pad-contention-verify")
    complete = native(context.for_feature("space", "module-pad-contention"), args.configuration, "pad-prepare", source)
    paused = context.for_feature("space", "module-pad-contention-ordinary")
    ordinary(paused, {"name": "module-pad-contention", "snapshot": identity(complete)}, args.configuration)
    resaved = paused.saves / "ordinary-resave.sav"
    expected, actual = chunks(complete)[1], chunks(resaved)[1]
    # Paid second-hull/material history, actual pad/queue custody and both original packet owners must survive ordinary loading exactly.
    for family in EXACT:
        assert expected[family] == actual[family], family
    assert identity(source) == prepared["preburn"]
    report = context.reports / "orbital-module-pad-contention.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group11C; actual second paid rocket, pad and whole hold refusal.",
        "configuration": args.configuration, "preparation": identity(predecessor), "source": identity(source),
        "complete": identity(complete), "ordinary_resave": identity(resaved), "exact_owner_families": len(EXACT),
        "paid_second_rocket": True, "pad_refusal": True, "hold_refusal": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Module pad contention: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
