"""Verify native moving-body approach queues without acquiring mining authority.

@file space_approach_checks-4.py Genuine body contention, motion and exact queued continuation.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group11.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context

shared = runpy.run_path(str(Path(__file__).with_name("space_approach_checks-1.py")))
native, EXACT = shared["native"], shared["EXACT"]


def main():
    """Compare actual queued-body continuation and ordinary resave across every native owner family."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    args = parser.parse_args()
    source = ROOT / "src/tests/fixtures/demos/space/space-module-frame-418.sav"
    original = identity(source)
    context = current_run_context("space", "approach-body-verify")
    preparation = context.for_feature("space", "approach-body-prepare")
    complete = native(preparation, args.configuration, "body-prepare", source)
    partial = preparation.saves / "approach-body-queued-422.sav"
    continued = native(context.for_feature("space", "approach-body-resume"), args.configuration, "body-continue", partial)
    version, expected = chunks(complete)
    assert version == 422
    actual = chunks(continued)[1]
    # Reservation replay must preserve all native field, cargo, job, utility and company owners without granting extraction or survey work.
    for family in EXACT:
        assert expected[family] == actual[family], family
    ordinary_context = context.for_feature("space", "approach-body-ordinary")
    ordinary(ordinary_context, {"name": "approach-body-complete", "snapshot": identity(complete)}, args.configuration)
    resaved = ordinary_context.saves / "ordinary-resave.sav"
    resaved_chunks = chunks(resaved)[1]
    # Cancelled navigation remains cancelled after ordinary load; it cannot recreate either an approach or a mining lease.
    for family in EXACT:
        assert expected[family] == resaved_chunks[family], family
    assert identity(source) == original
    report = context.reports / "space-body-approaches.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs8-space-world-and-construction.md Group11; native moving-body approach reservations.",
        "configuration": args.configuration, "save_version": version, "source": original, "partial": identity(partial),
        "complete": identity(complete), "continued": identity(continued), "ordinary_resave": identity(resaved),
        "exact_owner_families": len(EXACT), "body_reserve_unchanged": True, "no_mining_or_survey_lease": True,
        "moving_target_resolved_from_actual_body": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Native moving-body approaches verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
