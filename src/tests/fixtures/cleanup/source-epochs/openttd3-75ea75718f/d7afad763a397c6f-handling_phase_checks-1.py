"""Verify current handling phase custody and its repeated native financial ledger.

@file handling_phase_checks-1.py Explicit seeded-packet boundary fixture, distinct from material-paid flight provenance.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group17.
"""
import argparse
import json
from pathlib import Path
import runpy

from launch_center_checks import native
from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]


def main():
    """Replay the existing explicit handling packet fixture, checking sole custody and bounded expense without claiming new gameplay cargo production."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    args = parser.parse_args()
    source = ROOT / "src/tests/fixtures/demos/space/space-survey-approach-1990-404.sav"
    original = identity(source)
    context = current_run_context("space", "handling-phase-verify")
    partial = native(context.for_feature("space", "handling-phase-prepare"), args.configuration, handling="prepare")
    complete = native(context.for_feature("space", "handling-phase-continue"), args.configuration, partial, handling="continue")
    replay = native(context.for_feature("space", "handling-phase-replay"), args.configuration, partial, handling="continue")
    assert chunks(partial)[0] == 423 and chunks(complete)[0] == 423 and chunks(replay)[0] == 423
    expected, actual = chunks(complete)[1], chunks(replay)[1]
    # Identical native commands must settle the same remaining five packets, preserve route/age/share and charge the same two-unit handling expense exactly once.
    for family in EXACT:
        assert expected[family] == actual[family], family
    paused_saves = {}
    # Check both unresolved and just-completed handling; ordinary loading cannot advance the timer, charge work or move cargo a second time.
    for edge, snapshot in (("before", partial), ("after", complete)):
        paused = context.for_feature("space", "handling-phase-paused-" + edge)
        ordinary(paused, {"name": "handling-phase-" + edge, "snapshot": identity(snapshot)}, args.configuration)
        resaved = paused.saves / "ordinary-resave.sav"
        expected, actual = chunks(snapshot)[1], chunks(resaved)[1]
        for family in EXACT:
            assert expected[family] == actual[family], (edge, family)
        paused_saves[edge] = identity(resaved)
    assert identity(source) == original
    report = context.reports / "handling-phase.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group17; seeded thirteen-unit packet/near-aging-boundary characterization with actual native handling commands. This supplies no material-paid cargo producer claim. Existing observer checks fee2, retained feeder share130 and one canonical packet age increment.",
        "configuration": args.configuration, "source": original, "fixture_kind": "seeded_packet_handling_boundary", "before": identity(partial), "after": identity(complete),
        "replay": identity(replay), "ordinary_resaves": paused_saves, "exact_owner_families": len(EXACT),
        "transferred_before": 8, "transferred_after": 13, "completion_expense": 2, "no_cargo_income": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Handling phase: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
