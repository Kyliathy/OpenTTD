"""Verify one explicit edge/corner geometry fixture through native ticks, replay and ordinary reload.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group3
Each selected case is a bounded file2 invocation. Staged motion in an isolated
copy proves geometry and pre-decision lease translation, not fresh deployment.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context, run_native

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]
CASES = ("preview-edge", "preview-corner", "native-edge", "native-corner")


def boundary(context, configuration, source, case, resume=False):
    """Execute one isolated native boundary with real save ownership.

    @param context Explicit runtime and artifact owner.
    @param configuration Completed configuration to execute.
    @param source Immutable original or invocation-owned before checkpoint.
    @param case Declared geometry/lease fixture identity.
    @param resume Whether to retain already staged motion without editing it again.
    @return Actual before/after checkpoint paths and native log identity.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("asteroid_motion_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_ASTEROID_MOTION_SOURCE=str(source), OTTD_ASTEROID_MOTION_CASE=case)
    if resume:
        environment["OTTD_ASTEROID_MOTION_RESUME"] = "1"
    log = context.logs / "asteroid-motion-native.log"
    with log.open("wb") as sink:
        run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.asteroid-motion-native]"],
                   cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence and "hidden=true predecision=true" in evidence, log
    return context.saves / "asteroid-motion-before-424.sav", context.saves / "asteroid-motion-after-424.sav", identity(log)


def verify(configuration, case):
    """Check one boundary and all saved owners from independent replay and ordinary resave.

    @param configuration Completed configuration matching the native sources.
    @param case One of the four explicit boundary scenarios.
    @return Accepted case report path; never chooses inputs by directory recency.
    """
    context = current_run_context("space", "asteroid-motion-boundary-" + case)
    filename = "space-mining-working-1990-405.sav" if case.startswith("native-") else "space-survey-attached-1990-404.sav"
    source = ROOT / "src/tests/fixtures/demos/space" / filename
    original = identity(source)
    primary = context.for_feature("space", "asteroid-motion-stage-" + case)
    before, after, log = boundary(primary, configuration, source, case)
    replay = context.for_feature("space", "asteroid-motion-replay-" + case)
    repeated_before, repeated_after, replay_log = boundary(replay, configuration, before, case, True)
    # Exact independent continuation retains motion, phase, lease, packets and every unrelated gameplay owner.
    for first, second in ((before, repeated_before), (after, repeated_after)):
        left, right = chunks(first), chunks(second)
        assert left[0] == right[0] == 424
        for family in EXACT:
            assert left[1][family] == right[1][family], (case, family)
    retained = context.for_feature("space", "asteroid-motion-ordinary-" + case)
    ordinary(retained, {"name": case, "snapshot": identity(after)}, configuration)
    resaved = retained.saves / "ordinary-resave.sav"
    # Ordinary loading must retain the reflected direction and fractional phase rather than generating a new motion record.
    for family in EXACT:
        assert chunks(after)[1][family] == chunks(resaved)[1][family], (case, family)
    assert identity(source) == original
    report = context.reports / "asteroid-motion-boundary.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs10 Group3; isolated boundary geometry and actual pre-decision lease translation.",
        "configuration": configuration, "case": case, "source": original,
        "before": identity(before), "after": identity(after), "replay_before": identity(repeated_before),
        "replay_after": identity(repeated_after), "ordinary": identity(resaved),
        "native_log": log, "replay_log": replay_log, "steps": 38, "hidden": True,
        "exact_owner_families": [x.decode("ascii") for x in EXACT],
        "native_reports": [identity(p) for owner in (primary, replay, retained) for p in sorted(owner.reports.glob("native-*.json"))]},
        indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Parse one bounded case and print the completed native proof path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--case", choices=CASES, required=True)
    args = parser.parse_args()
    print(verify(args.configuration, args.case).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
