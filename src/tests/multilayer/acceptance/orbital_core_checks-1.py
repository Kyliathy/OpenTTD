"""Verify native Core activation and physical inventory in one focused burst.

@file orbital_core_checks-1.py Paid Core lifecycle and exact interrupted continuation.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1.
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
    """Run the native observer against an isolated immutable predecessor.

    @param context Isolated runtime and evidence owner.
    @param configuration Built executable configuration.
    @param action Prepare activation, continue activation, or continue stored inventory.
    @param source Existing paid frame or emitted native checkpoint.
    @param corrupt Optional malformed preview input; the active world remains source.
    @param reject Whether startup must reject source as malformed.
    @return Emitted completed native save, or None for a rejection check.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("orbital_core_test\n", encoding="utf-8")
    env = dict(os.environ, OTTD_ORBITAL_CORE_ACTION=action, OTTD_ORBITAL_CORE_SOURCE=str(source))
    env.pop("OTTD_ORBITAL_CORE_CORRUPT", None)
    if corrupt is not None:
        env["OTTD_ORBITAL_CORE_CORRUPT"] = str(corrupt)
    log = context.logs / "native-core.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.orbital-core]"],
                            cwd=ROOT / "build", env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=120)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        assert result.returncode != 0 and "All tests passed" not in evidence, log
        assert "orbital" in evidence.lower(), log
        return None
    assert result.returncode == 0 and "All tests passed" in evidence and "ORBITAL_CORE" in evidence, log
    if corrupt is not None:
        assert "preview_rejected=true authority_unchanged=true" in evidence, log
        return None
    # Export the real software-rendered window without redrawing or substituting an illustrated UI.
    for ppm in context.images.glob("*.ppm"):
        with Image.open(ppm) as raster:
            save_native_capture(context, raster, ppm, ppm.with_suffix(".png"))
    return context.saves / "core-complete-418.sav"


def main():
    """Bind a genuine paid two-launch frame to activation, storage and exact owner-family replay."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--source", required=True, type=Path, help="Immutable native paid construction-frame save")
    args = parser.parse_args()
    source = args.source.resolve()
    assert source.is_relative_to(ROOT)
    original = identity(source)
    context = current_run_context("space", "core-verify")
    prepare = context.for_feature("space", "core-prepare")
    complete = native(prepare, args.configuration, "prepare", source)
    version, expected = chunks(complete)
    assert version == 418
    checkpoints = []
    # Compare both activation and actual stored-packet interruptions, rather than an empty facility-only snapshot.
    for phase, action, name in (("activating", "continue", "core-activating-418.sav"),
                                ("stored", "stored", "core-stored-418.sav")):
        partial = prepare.saves / name
        partial_identity = identity(partial)
        result = native(context.for_feature("space", "core-resume-" + phase), args.configuration, action, partial)
        actual = chunks(result)[1]
        # Every paid craft, material receipt, packet owner and company balance must continue identically.
        for family in EXACT:
            assert expected[family] == actual[family], (phase, family)
        resave = context.for_feature("space", "core-ordinary-" + phase)
        ordinary(resave, {"name": phase, "snapshot": partial_identity}, args.configuration)
        # Ordinary loading while paused must neither advance activation nor renew/move module stock.
        for family in EXACT:
            assert chunks(partial)[1][family] == chunks(resave.saves / "ordinary-resave.sav")[1][family], ("ordinary", phase, family)
        assert identity(partial) == partial_identity
        checkpoints.append({"phase": phase, "checkpoint": partial_identity, "continued": identity(result)})
    assert identity(source) == original
    report = context.reports / "orbital-core-verification.json"
    report.write_text(json.dumps({"configuration": args.configuration, "save_version": version,
        "input": original, "completed": identity(complete), "checkpoints": checkpoints,
        "stored_checkpoint": identity(prepare.saves / "core-stored-418.sav"),
        "exact_owner_families": [family.decode() for family in EXACT],
        "operational_core_inventory": True, "rocket_module_delivery": False,
        "remaining_scope": "precise berth/service work, shared utility publication, later module functions and rocket module delivery"}, indent=2) + "\n", encoding="utf-8")
    print(f"Native Core activation and stock verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
