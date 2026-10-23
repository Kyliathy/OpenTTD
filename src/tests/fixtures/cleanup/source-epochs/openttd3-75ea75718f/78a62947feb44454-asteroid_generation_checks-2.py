"""Verify frozen input through real37-tick continuations, legacy migration and independent replay.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group2
This bounded second file consumes an explicitly accepted new-world producer.
Original inputs are immutable; all native outputs belong to their invocation.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context, run_native

FIELD_RECORD = runpy.run_path(str(Path(__file__).with_name("asteroid_generation_checks-1.py")))["field_record"]
EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]


def continuation(context, configuration, source):
    """Advance one real loaded field and preserve native before/after save ownership.

    @param context Explicit native invocation owner.
    @param configuration Completed build configuration.
    @param source Exact genuine source path, never rewritten by this producer.
    @return Before/after native save paths and the executed log identity.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("asteroid_starter_test\n", encoding="utf-8")
    log = context.logs / "asteroid-starter-native.log"
    with log.open("wb") as sink:
        run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.asteroid-generation-native]"],
                   cwd=ROOT / "build", env=dict(os.environ, OTTD_ASTEROID_STARTER_SOURCE=str(source)),
                   stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence and "no_reroll=true" in evidence, log
    return context.saves / "asteroid-starter-before-424.sav", context.saves / "asteroid-starter-after-424.sav", identity(log)


def verify(configuration, predecessor):
    """Check new and legacy fields through native motion, ordinary reload and exact independent replay.

    @param configuration Build configuration matching the explicitly accepted predecessor.
    @param predecessor Accepted file1 report; no directory recency heuristic selects an input.
    @return Accepted continuation report path.
    """
    proof = json.loads(predecessor.read_text(encoding="utf-8"))
    assert proof["configuration"] == configuration
    normal = next(case for case in proof["cases"] if case["profile"] == "multilayer_industry" and case["density"] == 1)
    context = current_run_context("space", "asteroid-generation-continuation-verify")
    results, owners = [], []
    # A real new field and an immutable404 field exercise both recorded history and explicit legacy absence.
    for name, source in (("new", ROOT / normal["source"]["path"]),
                         ("legacy", ROOT / "src/tests/fixtures/demos/space/space-survey-approach-1990-404.sav")):
        original = identity(source)
        if name == "new":
            assert original == normal["source"]
        primary = context.for_feature("space", "asteroid-starter-native-" + name)
        before, after, log = continuation(primary, configuration, source)
        initial, complete = FIELD_RECORD(before), FIELD_RECORD(after)
        assert complete["ticks"] == initial["ticks"] + 37
        assert initial["input"] == complete["input"]
        if name == "legacy":
            assert initial["input"]["kind"] == 0 and initial["input"]["distance_squared"] == 0
        else:
            assert initial["input"]["kind"] in (2, 3)
        replay = context.for_feature("space", "asteroid-starter-replay-" + name)
        repeated_before, repeated_after, replay_log = continuation(replay, configuration, source)
        # Independent processes start from the same immutable source; every saved gameplay owner must converge exactly.
        for first, second in ((before, repeated_before), (after, repeated_after)):
            left, right = chunks(first)[1], chunks(second)[1]
            for family in EXACT:
                assert left[family] == right[family], (name, family)
        retained = context.for_feature("space", "asteroid-starter-ordinary-" + name)
        ordinary(retained, {"name": name, "snapshot": identity(after)}, configuration)
        resaved = retained.saves / "ordinary-resave.sav"
        # An ordinary paused load retains the current motion and the original historical input independently.
        for family in EXACT:
            assert chunks(after)[1][family] == chunks(resaved)[1][family], (name, family)
        assert identity(source) == original
        results.append({"name": name, "source": original, "before": identity(before), "after": identity(after),
                        "replay_before": identity(repeated_before), "replay_after": identity(repeated_after),
                        "ordinary": identity(resaved), "input": initial["input"], "native_log": log, "replay_log": replay_log})
        owners += [primary, replay, retained]
    report = context.reports / "asteroid-generation-continuation.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs10 Group2; actual motion, legacy history absence and exact owner-family continuation.",
        "configuration": configuration, "predecessor": identity(predecessor), "cases": results,
        "exact_owner_families": [family.decode("ascii") for family in EXACT],
        "native_reports": [identity(path) for owner in owners for path in sorted(owner.reports.glob("native-*.json"))]}, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Parse the exact predecessor and print the completed bounded continuation report path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--predecessor", type=Path, required=True)
    args = parser.parse_args()
    print(verify(args.configuration, args.predecessor.resolve()).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
