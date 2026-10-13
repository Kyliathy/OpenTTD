"""Verify paid native Earth hull assembly, exact continuation and copied preview refusal.

@file launch_assembly_checks.py Focused current-schema paid hull/slot acceptance.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups2B/8B.
"""
import argparse
import json
import os
from pathlib import Path
import struct
import subprocess
from PIL import Image
from space_preview_checks import ROOT, chunks, identity, ordinary, read_stream
from launch_center_checks import riff_location
from runner import current_run_context, run_native, save_native_capture


def native(context, configuration, action, source=None, corrupt=None, reject=False):
    """Run one actual native assembly observer with repository-owned artifacts.

    @param context Explicit feature invocation owning runtime, logs, saves and captures.
    @param configuration Compiled Debug or RelWithDebInfo executable to run.
    @param action Native preparation or interrupted continuation action.
    @param source Optional saved world used for continuation.
    @param corrupt Optional malformed preview input tested without replacing the live world.
    @param reject Whether this actual load must fail before publishing successful acceptance.
    @return Generated interrupted/completed save, or None after an expected malformed-load refusal.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("launch_assembly_test\n", encoding="utf-8")
    env = dict(os.environ, OTTD_LAUNCH_ASSEMBLY_ACTION=action)
    # Each native process receives only this invocation's explicit inputs;
    # inherited source/corruption hooks must not leak between scenarios.
    for name, value in (("OTTD_LAUNCH_ASSEMBLY_SOURCE", source), ("OTTD_LAUNCH_ASSEMBLY_CORRUPT", corrupt)):
        env.pop(name, None)
        if value is not None:
            env[name] = str(value)
    log = context.logs / "native-assembly.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.launch-assembly]"],
                            cwd=ROOT / "build", env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=120)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        assert result.returncode != 0 and "All tests passed" not in evidence, log
        assert "launch assembly" in evidence.lower(), log
        return
    assert result.returncode == 0 and "All tests passed" in evidence and "LAUNCH_ASSEMBLY native=true" in evidence, log
    if corrupt:
        assert "preview_reject=true authority_unchanged=true" in evidence, log
    # Keep exact native pixels and their producing-process identity while
    # converting the test's raster container into a reviewable PNG.
    for ppm in context.images.glob("*.ppm"):
        with Image.open(ppm) as raster:
            save_native_capture(context, raster, ppm, ppm.with_suffix(".png"))
    # The observer retains its414 output basename. The actual save header below
    # must match415; filenames are not a schema/version authority.
    return context.saves / ("assembly-partial-414.sav" if action == "prepare" else "assembly-complete-414.sav")


def damage(context, source, kind):
    """Create one malformed copy while preserving the retained source and all unrelated bytes.

    @param context Invocation owning the new corruption fixture.
    @param source Current-schema interrupted native assembly save.
    @param kind Single receipt, slot, identity or physical-placement defect to introduce.
    @return Repository-local corrupted save for actual-load and independent-preview checks.
    """
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 415
    family = b"LCOP" if kind.startswith("slot-") else b"SVST"
    start, body, length, end = riff_location(stream, family)
    changed = bytearray(stream)
    row = body + 1 + 3 * 49  # Fourth actual native Vehicle is the incomplete construction probe.
    if kind.endswith("missing"):
        del changed[start:end]
    elif kind == "count":
        changed[body] = 65
    elif kind == "generation":
        changed[row + 4:row + 12] = bytes(8)
    elif kind == "receipt":
        changed[row + 39:row + 41] = struct.pack(">H", 99)
    elif kind == "capital":
        changed[row + 43:row + 47] = bytes(4)
    elif kind == "work":
        changed[row + 37:row + 39] = struct.pack(">H", 149)
    elif kind == "orbital":
        changed[row + 12] = 0
    elif kind == "boolean":
        changed[row + 47] = 2
    elif kind == "home-generation":
        changed[row + 15:row + 23] = struct.pack(">Q", 999)
    elif kind == "origin-generation":
        changed[row + 25:row + 33] = bytes(8)
    elif kind == "physical-position":
        tile = struct.unpack(">I", changed[row + 33:row + 37])[0]
        changed[row + 33:row + 37] = struct.pack(">I", tile + 1)
    elif kind == "physical-model":
        changed[row + 41:row + 43] = struct.pack(">H", 15)  # Same cost/work, wrong real native Engine.
    elif kind == "slot-count":
        changed[body + 57] = 9
    elif kind == "slot-generation":
        changed[body + 15:body + 23] = struct.pack(">Q", 999)
    elif kind == "slot-duplicate":
        changed[body + 58:body + 70] = changed[body + 11:body + 23]
    elif kind == "slot-service":
        changed[body + 23:body + 35] = changed[body + 11:body + 23]
    elif kind == "slot-center":
        changed[body + 3:body + 11] = struct.pack(">Q", 999)
    else:
        raise ValueError(kind)
    target = context.saves / f"assembly-{kind}.sav"
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def verify_lifecycle(context, configuration):
    """Verify exact native assembly and ordinary paused continuation before malformed-input checks.

    @param context Explicit invocation sharing the preparation and continuation artifacts.
    @param configuration Compiled native configuration under test.
    @return Interrupted save and lifecycle-only evidence for the invocation report.
    """
    prepare = context.for_feature("space", "assembly-prepare")
    built = native(prepare, configuration, "prepare")
    version, before = chunks(built)
    assert version == 415
    exact = (b"SPST", b"SPKN", b"SPDJ", b"SPFL", b"SPOD", b"SPSU", b"SVST", b"LCOP", b"SDEF", b"SPOR", b"ASTR", b"VEHS", b"CAPA", b"PLYR", b"STNN", b"LCTR", b"LCHB", b"LHND")
    results = []
    # Independent continuations must produce identical native ownership/work,
    # rather than relying on a single successful reload of the paid hull.
    for name in ("assembly-resume", "assembly-replay"):
        result = native(context.for_feature("space", name), configuration, "continue", built)
        results.append(chunks(result)[1])
    prepared = chunks(prepare.saves / "assembly-complete-414.sav")[1]
    # Compare each authoritative save family with uninterrupted completion;
    # no receipt, packet, cash balance or slot may diverge after interruption.
    for key in exact:
        assert prepared[key] == results[0][key] == results[1][key], ("native interrupted continuation", key)
    owner = context.for_feature("space", "assembly-ordinary")
    ordinary(owner, {"name": "assembly", "snapshot": identity(built)}, configuration)
    after = chunks(owner.saves / "ordinary-resave.sav")[1]
    # An ordinary paused game resave must preserve every owner family exactly.
    for key in exact:
        assert before[key] == after[key], ("ordinary paused reload", key)
    return built, {"paid_native_models": [14, 15, 19, 20], "capital": 660000,
                   "consumed_steel": 22, "consumed_goods": 36, "same_site_recovery_fee": 0,
                   "exact_prepared_and_reloaded_results": 3,
                   "ordinary_exact_chunks": [key.decode() for key in exact],
                   "native_workshop_images": [identity(path) for path in sorted(prepare.images.glob("*.png"))]}


def verify_rejections(context, configuration, built, preview):
    """Verify one bounded load or preview refusal burst against a known valid interrupted save.

    @param context Explicit invocation owning malformed copies and their native reports.
    @param configuration Compiled native configuration under test.
    @param built Valid current-schema interrupted save from the lifecycle burst.
    @param preview Whether to check copied read-only previews instead of actual failed loads.
    @return Number of isolated malformed inputs actually rejected in this burst.
    """
    kinds = ("missing", "count", "generation", "receipt", "capital", "work", "orbital", "boolean", "home-generation",
             "origin-generation", "physical-position", "physical-model", "slot-missing", "slot-count", "slot-generation",
             "slot-duplicate", "slot-service", "slot-center")
    physical = {"physical-position", "physical-model"}
    count = 0
    # Each isolated corruption must fail an actual load. Defects visible from
    # copied transport also fail a read-only preview without changing live authority.
    for kind in kinds:
        # Physical position/model consistency belongs to the actual-world load
        # validator; copied previews deliberately do not inspect live Engine/tile owners.
        if preview and kind in physical:
            continue
        owner = context.for_feature("space", "assembly-reject-" + kind)
        corrupt = damage(owner, built, kind)
        if preview:
            native(context.for_feature("space", "assembly-preview-" + kind), configuration, "continue", built, corrupt)
        else:
            native(owner, configuration, "continue", corrupt, reject=True)
        count += 1
    return count


def main(part=None):
    """Run all acceptance or one bounded numbered burst and publish only its actual evidence.

    @param part None for the legacy complete suite;1 lifecycle,2 failed loads,3 copied previews.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--source", type=Path, help="Interrupted save produced by launch_assembly_checks-1.py")
    args = parser.parse_args()
    # Refusal-only bursts require the explicit successful lifecycle checkpoint;
    # silently regenerating it would repeat setup and defeat the two-minute split.
    if part in (2, 3) and args.source is None:
        parser.error("--source from the lifecycle burst is required")
    context = current_run_context("space", "assembly-verify")
    retained = ROOT / "src/tests/fixtures/demos/space/space-launch-hub-1990-408.sav"
    original = identity(retained)
    source_identity = None
    if part in (2, 3):
        built = args.source.resolve()
        source_identity = identity(built)
        assert chunks(built)[0] == 415
        # A valid control continuation must succeed before malformed copies are
        # offered, so an already-invalid source cannot make every refusal pass.
        native(context.for_feature("space", "assembly-resume"), args.configuration, "continue", built)
        evidence = {}
    else:
        built, evidence = verify_lifecycle(context, args.configuration)
    load_count = verify_rejections(context, args.configuration, built, False) if part in (None, 2) else 0
    preview_count = verify_rejections(context, args.configuration, built, True) if part in (None, 3) else 0
    assert identity(retained) == original
    if source_identity is not None:
        assert identity(built) == source_identity
    report = context.reports / "launch-assembly-verification.json"
    report.write_text(json.dumps({"configuration": args.configuration, "save_version": 415, "part": part,
                                 "retained": original, "interrupted": identity(built),
                                 **evidence, "native_corrupt_rejections": load_count,
                                 "independent_preview_rejections": preview_count,
                                 "remaining_scope": "focused paid assembly only; no new flight/descent, final art or network acceptance"}, indent=2) + "\n", encoding="utf-8")
    print(f"Paid native Earth hull assembly verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
