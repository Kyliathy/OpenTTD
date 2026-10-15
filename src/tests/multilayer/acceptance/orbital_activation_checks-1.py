"""Verify genuine paid Storage activation and interrupted native continuation.

@file orbital_activation_checks-1.py Focused422 activation, old-frame migration and ordinary resave.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group4.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md prerequisite checklist.
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
    """Run one ordinary loaded-world activation observer with isolated native owners.

    @param context Explicit runtime and evidence destination.
    @param configuration Built game/test configuration.
    @param action Prepare/continue activation, move/continue real freight, or inspect a malformed preview.
    @param source Immutable genuine frame or emitted partial activation save.
    @param corrupt Optional independently malformed copied-preview input.
    @param reject Require an actual full-loader refusal of the malformed source.
    @return Actual complete422 save emitted by the native writer, or None for a verified loader/preview refusal.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("orbital_activation_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_ORBITAL_ACTIVATION_SOURCE=str(source), OTTD_ORBITAL_ACTIVATION_ACTION=action)
    environment.pop("OTTD_ORBITAL_ACTIVATION_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_ORBITAL_ACTIVATION_CORRUPT"] = str(corrupt)
    log = context.logs / "native-module-activation.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.orbital-module-activation]"],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        # Require the native orbital decoder's refusal, so unrelated test/startup failures cannot count as corrupt-save acceptance.
        assert result.returncode != 0 and "All tests passed" not in evidence and "Broken savegame" in evidence and "Invalid orbital" in evidence, log
        return None
    # A normal native pass and the actual loaded-world observer receipt must both succeed; startup failure is never completion evidence.
    assert result.returncode == 0 and "All tests passed" in evidence and "ORBITAL_ACTIVATION native=true" in evidence, log
    if corrupt is not None:
        assert "preview_rejected=true authority_unchanged=true" in evidence, log
        return None
    return context.saves / ("module-freight-422.sav" if action in ("freight", "stored") else "module-activated-422.sav")


def main():
    """Compare actual continued and uninterrupted module owners and the ordinary game's paused resave."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    args = parser.parse_args()
    source = ROOT / "src/tests/fixtures/demos/space/space-module-frame-418.sav"
    original = identity(source)
    context = current_run_context("space", "module-activation-verify")
    preparation = context.for_feature("space", "module-activation-prepare")
    complete = native(preparation, args.configuration, "prepare", source)
    version, expected = chunks(complete)
    assert version == 422
    partial = preparation.saves / "module-activating-422.sav"
    continued = native(context.for_feature("space", "module-activation-resume"), args.configuration, "continue", partial)
    actual = chunks(continued)[1]
    # All existing physical owners, utility history and command receipts must survive the interruption exactly.
    for family in EXACT:
        assert expected[family] == actual[family], family
    ordinary_context = context.for_feature("space", "module-activation-ordinary")
    ordinary(ordinary_context, {"name": "module-activated", "snapshot": identity(complete)}, args.configuration)
    resaved = ordinary_context.saves / "ordinary-resave.sav"
    resaved_chunks = chunks(resaved)[1]
    # Ordinary loading must retain the same contained facility and packets without rerunning activation or its recipe.
    for family in EXACT:
        assert expected[family] == resaved_chunks[family], family
    assert identity(source) == original
    report = context.reports / "orbital-module-activation.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group4; native paid Storage activation.",
        "configuration": args.configuration, "save_version": version, "source": original, "partial": identity(partial),
        "complete": identity(complete), "continued": identity(continued), "ordinary_resave": identity(resaved),
        "exact_owner_families": len(EXACT), "one_activation": True, "no_second_material_debit": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Native module activation verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
