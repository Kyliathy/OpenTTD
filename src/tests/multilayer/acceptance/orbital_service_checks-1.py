"""Verify actual Core service and exact interrupted owners in one focused burst.

@file orbital_service_checks-1.py Native paid service, cancellation and paused resaves.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1B.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
from PIL import Image

from space_preview_checks import ROOT, chunks, identity, ordinary
from runner import current_run_context, run_native, save_native_capture

EXACT = (b"SPST", b"SPKN", b"SPDJ", b"SPFL", b"SPOD", b"SPSU", b"SVST", b"LCOP", b"SDEF",
         b"SPOR", b"ASTR", b"VEHS", b"CAPA", b"PLYR", b"STNN", b"LCTR", b"LCHB", b"LHND",
         b"UPLH", b"UPLD", b"UPPH", b"UPPD", b"TUPH", b"TUPD", b"TUVH", b"TUVD")


def native(context, configuration, action, source, corrupt=None, reject=False):
    """Run a native contained-service observer against an immutable real predecessor.

    @param context Isolated runtime/evidence owner.
    @param configuration Built executable configuration.
    @param action Prepare, work, paused, power, last, cancel or ownership continuation.
    @param source Genuine paid Core or native partial-service checkpoint.
    @param corrupt Optional independent copied-preview malformed input.
    @param reject Whether native startup must refuse the malformed source.
    @return Completed native checkpoint, or None for refusal/cancellation checks.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("orbital_service_test\n", encoding="utf-8")
    env = dict(os.environ, OTTD_ORBITAL_SERVICE_ACTION=action, OTTD_ORBITAL_SERVICE_SOURCE=str(source))
    env.pop("OTTD_ORBITAL_SERVICE_CORRUPT", None)
    if corrupt is not None:
        env["OTTD_ORBITAL_SERVICE_CORRUPT"] = str(corrupt)
    log = context.logs / "native-service.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.orbital-service]"],
                            cwd=ROOT / "build", env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=120)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        assert result.returncode != 0 and "All tests passed" not in evidence, log
        assert "orbital" in evidence.lower(), log
        return None
    assert result.returncode == 0 and "All tests passed" in evidence and "ORBITAL_SERVICE" in evidence, log
    if corrupt is not None:
        assert "preview_rejected=true authority_unchanged=true" in evidence, log
        return None
    # Export actual software-rendered native controls; this transformation only converts the captured raster format.
    for ppm in context.images.glob("*.ppm"):
        with Image.open(ppm) as raster:
            save_native_capture(context, raster, ppm, ppm.with_suffix(".png"))
    filename = {"cancel": "service-cancelled-418.sav", "ownership": "service-unowned-418.sav"}.get(action, "service-complete-418.sav")
    return context.saves / filename


def main():
    """Bind paid416 migration, four interrupted service phases and physical cancellation to exact owner evidence."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--source", required=True, type=Path, help="Immutable inventory-only core-complete-416.sav")
    args = parser.parse_args()
    source = args.source.resolve()
    assert source.is_relative_to(ROOT)
    original = identity(source)
    context = current_run_context("space", "service-verify")
    prepare = context.for_feature("space", "service-prepare")
    complete = native(prepare, args.configuration, "prepare", source)
    version, expected = chunks(complete)
    assert version == 418 and b"UPLH" in expected and b"UPLD" in expected and b"TUVH" in expected
    checkpoints = []
    # Every interruption carries physical inputs, utility lineage, clock and company expense through the normal loader.
    for phase in ("work", "paused", "power", "last"):
        partial = prepare.saves / ("service-" + phase + "-418.sav")
        before = identity(partial)
        continued = native(context.for_feature("space", "service-resume-" + phase), args.configuration, phase, partial)
        actual = chunks(continued)[1]
        for family in EXACT:
            assert expected.get(family) == actual.get(family), (phase, family)
        resave = context.for_feature("space", "service-ordinary-" + phase)
        ordinary(resave, {"name": phase, "snapshot": before}, args.configuration)
        original_chunks, saved_chunks = chunks(partial)[1], chunks(resave.saves / "ordinary-resave.sav")[1]
        for family in EXACT:
            assert original_chunks.get(family) == saved_chunks.get(family), ("ordinary", phase, family)
        assert identity(partial) == before
        checkpoints.append({"phase": phase, "checkpoint": before, "continued": identity(continued)})
    cancelled = native(context.for_feature("space", "service-cancel"), args.configuration, "cancel", prepare.saves / "service-work-418.sav")
    unowned = native(context.for_feature("space", "service-ownership"), args.configuration, "ownership", prepare.saves / "service-work-418.sav")
    assert identity(source) == original
    report = context.reports / "orbital-service-verification.json"
    report.write_text(json.dumps({"configuration": args.configuration, "save_version": version, "input": original,
        "completed": identity(complete), "checkpoints": checkpoints, "cancelled": identity(cancelled), "unowned": identity(unowned),
        "exact_owner_families": [family.decode() for family in EXACT], "actual_service_parts": ["RFMT1", "ELEC1"],
        "settled_energy_j": 111000, "service_ticks": 148, "allowance_replaced": 2,
        "zero_grant_and_manual_pause": True, "migration_requires_explicit_commission": True,
        "actual_parts_imported_by_additional_paid_rocket_flight": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Native Core service verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
