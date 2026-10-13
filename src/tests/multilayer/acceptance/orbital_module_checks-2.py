"""Prepare genuine powered assembly milestones in one bounded native burst.

@file orbital_module_checks-2.py Paid resupply, real810J competition and exact final construction use.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group3.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess

from space_preview_checks import ROOT, chunks, identity
from runner import current_run_context, run_native


def native(context, configuration, action, source, corrupt=None, reject=False):
    """Run one actual-world assembly observer without fabricating cargo, source output or positions.

    @param context Isolated native runtime/evidence owner.
    @param configuration Built executable configuration.
    @param action Prepare, work, partial, power, last, cancel or old first-Core bootstrap continuation.
    @param source Immutable genuine reservation or actual assembly checkpoint.
    @param corrupt Optional independently malformed copied-preview input.
    @param reject Whether startup must refuse the malformed source before the observer runs.
    @return Saved completed frame/cancellation checkpoint, or None for refusal checks.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("orbital_assembly_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_ORBITAL_ASSEMBLY_ACTION=action, OTTD_ORBITAL_ASSEMBLY_SOURCE=str(source))
    environment.pop("OTTD_ORBITAL_ASSEMBLY_CORRUPT", None)
    if corrupt is not None:
        environment["OTTD_ORBITAL_ASSEMBLY_CORRUPT"] = str(corrupt)
    log = context.logs / "native-module-assembly.log"
    with log.open("wb") as sink:
        tag = "[.orbital-bootstrap-receipt]" if action == "bootstrap" else "[.orbital-module-assembly]"
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tag],
                            cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=115)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        # Require the native decoder's specific rejection, so an unrelated startup or observer failure cannot count as corrupt-save coverage.
        assert (result.returncode != 0 and "All tests passed" not in evidence
                and "Game load failed" in evidence and "Broken savegame" in evidence and "Invalid orbital" in evidence), log
        return None
    assert result.returncode == 0 and "All tests passed" in evidence and "ORBITAL_ASSEMBLY" in evidence, log
    if corrupt is not None:
        assert "preview_rejected=true authority_unchanged=true" in evidence, log
        return None
    filename = {"cancel": "module-assembly-cancelled-418.sav", "bootstrap": "bootstrap-frame-418.sav"}.get(action, "module-assembly-frame-418.sav")
    return context.saves / filename


def main():
    """Create real fractional/zero-power/final-use checkpoints while preserving the paid417 input."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--source", type=Path, required=True, help="Immutable genuine module-reserved-417.sav from Group2")
    args = parser.parse_args()
    source = args.source.resolve()
    assert source.is_relative_to(ROOT)
    before = identity(source)
    context = current_run_context("space", "module-assembly-prepare")
    complete = native(context, args.configuration, "prepare", source)
    assert chunks(complete)[0] == 418
    checkpoints = {phase: identity(context.saves / ("module-assembly-" + phase + "-418.sav"))
                   for phase in ("work", "partial", "power", "last")}
    assert identity(source) == before
    report = context.reports / "orbital-module-assembly-preparation.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group3; native paid assembly milestones.",
        "configuration": args.configuration, "save_version": 418, "input": before, "checkpoints": checkpoints, "frame": identity(complete),
        "panel_j_per_tick": 810, "service_grant_j": 750, "partial_assembly_grant_j": 60, "partial_work_ticks": 39,
        "partial_fraction_q16": 62914, "partial_division_remainder": 420, "final_useful_grant_j": 30,
        "construction_settlement_j": 222000, "service_settlement_j": 27750, "prior_original_source_use_j": 111000,
        "final_original_source_use_j": 360750, "shared_single_construction_slot": True,
        "frame_inert_pending_functional_activation": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Native assembly milestones prepared: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
