"""Verify retained zero-reserve records, actual lease exit and exact ordinary persistence.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group5
The staged depleted record is an explicit boundary fixture, not production of
its historical missing reserve or acceptance of a newly launched mission.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context, run_native

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]


def native(context, configuration, source, resume=False):
    """Execute one retained native depletion boundary with real load/tick/save ownership.

    @param context Explicit invocation-local runtime and output owner.
    @param configuration Completed test build configuration.
    @param source Immutable original or accepted before-boundary checkpoint.
    @param resume Whether to retain an already staged zero-reserve record.
    @return Actual native before/after checkpoint paths and log identity.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("asteroid_depletion_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_ASTEROID_DEPLETION_SOURCE=str(source))
    if resume:
        environment["OTTD_ASTEROID_DEPLETION_RESUME"] = "1"
    log = context.logs / "asteroid-depletion-native.log"
    with log.open("wb") as sink:
        run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.asteroid-depletion-native]"],
                   cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence and "zero=true retained=true cargo_unchanged=true" in evidence, log
    return context.saves / "asteroid-depletion-before-424.sav", context.saves / "asteroid-depletion-after-424.sav", identity(log)


def verify(configuration):
    """Verify focused body records, an existing native lease and all26 saved owner families.

    @param configuration Completed configuration; product writer remains the accepted424 build.
    @return Accepted bounded depletion report path with native provenance.
    """
    context = current_run_context("space", "asteroid-depletion-verify")
    focused = context.for_feature("space", "asteroid-depletion-focused")
    log = focused.logs / "asteroid-depletion-focused.log"
    with log.open("wb") as sink:
        run_native(focused, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[asteroid-depletion]"],
                   cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", log.read_text(encoding="utf-8"))
    assert passed, log
    source = ROOT / "src/tests/fixtures/demos/space/space-mining-working-1990-405.sav"
    original = identity(source)
    primary = context.for_feature("space", "asteroid-depletion-stage")
    before, after, native_log = native(primary, configuration, source)
    replay = context.for_feature("space", "asteroid-depletion-replay")
    repeated_before, repeated_after, replay_log = native(replay, configuration, before, True)
    # An independent ordinary load resumes the same depleted/leased boundary without recreating the field or its missing resources.
    for first, second in ((before, repeated_before), (after, repeated_after)):
        left, right = chunks(first), chunks(second)
        assert left[0] == right[0] == 424
        for family in EXACT:
            assert left[1][family] == right[1][family], family
    retained = context.for_feature("space", "asteroid-depletion-ordinary")
    ordinary(retained, {"name": "zero-reserve", "snapshot": identity(after)}, configuration)
    resaved = retained.saves / "ordinary-resave.sav"
    # Paused ordinary reload preserves zero reserve, monotonic identity, released lease and actual held packets together.
    for family in EXACT:
        assert chunks(after)[1][family] == chunks(resaved)[1][family], family
    assert identity(source) == original
    report = context.reports / "asteroid-depletion.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs10 Group5; zero-reserve boundary fixture, no removal path and actual native lease exit.",
        "configuration": configuration, "focused_assertions": int(passed.group(1)), "focused_cases": int(passed.group(2)),
        "source": original, "before": identity(before), "after": identity(after), "replay_before": identity(repeated_before),
        "replay_after": identity(repeated_after), "ordinary": identity(resaved),
        "focused_log": identity(log), "native_log": native_log, "replay_log": replay_log,
        "exact_owner_families": [x.decode("ascii") for x in EXACT], "removal_supported": False,
        "native_reports": [identity(p) for owner in (focused, primary, replay, retained) for p in sorted(owner.reports.glob("native-*.json"))]},
        indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Parse the completed configuration and print its accepted bounded report path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    args = parser.parse_args()
    print(verify(args.configuration).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
