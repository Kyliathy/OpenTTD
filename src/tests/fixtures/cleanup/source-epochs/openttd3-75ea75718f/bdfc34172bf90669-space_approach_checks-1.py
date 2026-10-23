"""Verify paid native approach contention, queued reload and actual target cancellation.

@file space_approach_checks-1.py Focused422 shared approach producer acceptance.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group11.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group4B.
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


def native(context, configuration, action, source, corrupt=None, reject=False):
    """Run the ordinary loaded-world observer without injecting hulls, positions or materials.

    @param context Explicit native runtime and evidence destination.
    @param configuration Built game/test configuration.
    @param action Prepare, continue or inspect a malformed copied preview.
    @param source Genuine paid predecessor or this run's queued checkpoint.
    @param corrupt Optional independently malformed preview input.
    @param reject Require actual full-loader refusal before the observer runs.
    @return Native completed checkpoint, or None for a verified loader/preview refusal.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("space_approach_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_SPACE_APPROACH_SOURCE=str(source), OTTD_SPACE_APPROACH_ACTION=action)
    environment.pop("OTTD_SPACE_APPROACH_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_SPACE_APPROACH_CORRUPT"] = str(corrupt)
    log = context.logs / "native-space-approaches.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.space-approaches]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        # Require the native decoder's refusal; test failures or application startup errors are never acceptance evidence.
        assert result.returncode != 0 and "All tests passed" not in evidence and "Broken savegame" in evidence, log
        assert "approach" in evidence.lower() or "space order" in evidence.lower(), log
        return None
    assert result.returncode == 0 and "All tests passed" in evidence and "SPACE_APPROACH native=true" in evidence, log
    if corrupt is not None:
        assert "preview_rejected=true authority_unchanged=true" in evidence, log
        return None
    return context.saves / ("approach-body-complete-422.sav" if action.startswith("body-") else "approach-complete-422.sav")


def main():
    """Compare exact owner families after a genuine queued interruption and an ordinary game resave."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    args = parser.parse_args()
    source = ROOT / "src/tests/fixtures/demos/space/space-module-frame-418.sav"
    original = identity(source)
    context = current_run_context("space", "approach-verify")
    preparation = context.for_feature("space", "approach-prepare")
    complete = native(preparation, args.configuration, "prepare", source)
    partial = preparation.saves / "approach-queued-422.sav"
    continued = native(context.for_feature("space", "approach-resume"), args.configuration, "continue", partial)
    version, expected = chunks(complete)
    assert version == 422
    actual = chunks(continued)[1]
    # The queue owns reservations alongside actual cargo, utility, cost and construction records; interruption must preserve them all.
    for family in EXACT:
        assert expected[family] == actual[family], family
    ordinary_context = context.for_feature("space", "approach-ordinary")
    ordinary(ordinary_context, {"name": "approach-complete", "snapshot": identity(complete)}, args.configuration)
    resaved = ordinary_context.saves / "ordinary-resave.sav"
    resaved_chunks = chunks(resaved)[1]
    # Ordinary loading cannot re-admit a cancelled target or substitute a new slot/priority outside native arbitration.
    for family in EXACT:
        assert expected[family] == resaved_chunks[family], family
    assert identity(source) == original
    report = context.reports / "space-approaches.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs8-space-world-and-construction.md Group11; shared native approach producer.",
        "configuration": args.configuration, "save_version": version, "source": original, "partial": identity(partial),
        "complete": identity(complete), "continued": identity(continued), "ordinary_resave": identity(resaved),
        "exact_owner_families": len(EXACT), "stable_simultaneous_admission": True,
        "actual_departure_required": True, "target_cancellation_releases_only_affected_requests": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Native shared approaches verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
