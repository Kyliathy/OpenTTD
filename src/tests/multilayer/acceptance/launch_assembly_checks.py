"""Verify paid native Earth hull assembly, exact interrupted continuation and independent preview."""
import argparse
import json
import os
import struct
import subprocess
from PIL import Image
from space_preview_checks import ROOT, chunks, identity, ordinary, read_stream
from launch_center_checks import riff_location
from runner import current_run_context, run_native, save_native_capture


def native(context, configuration, action, source=None, corrupt=None, reject=False):
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("launch_assembly_test\n", encoding="utf-8")
    env = dict(os.environ, OTTD_LAUNCH_ASSEMBLY_ACTION=action)
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
    for ppm in context.images.glob("*.ppm"):
        with Image.open(ppm) as raster:
            save_native_capture(context, raster, ppm, ppm.with_suffix(".png"))
    return context.saves / ("assembly-partial-414.sav" if action == "prepare" else "assembly-complete-414.sav")


def damage(context, source, kind):
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 414
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    args = parser.parse_args()
    context = current_run_context("space", "assembly-verify")
    retained = ROOT / "src/tests/fixtures/demos/space/space-launch-hub-1990-408.sav"
    original = identity(retained)
    prepare = context.for_feature("space", "assembly-prepare")
    built = native(prepare, args.configuration, "prepare")
    version, before = chunks(built)
    assert version == 414
    exact = (b"SPKN", b"SPDJ", b"SPFL", b"SPOD", b"SPSU", b"SVST", b"LCOP", b"SDEF", b"SPOR", b"ASTR", b"VEHS", b"CAPA", b"PLYR", b"STNN", b"LCTR", b"LCHB", b"LHND")
    results = []
    for name in ("assembly-resume", "assembly-replay"):
        result = native(context.for_feature("space", name), args.configuration, "continue", built)
        results.append(chunks(result)[1])
    prepared = chunks(prepare.saves / "assembly-complete-414.sav")[1]
    for key in exact:
        assert prepared[key] == results[0][key] == results[1][key], ("native interrupted continuation", key)
    owner = context.for_feature("space", "assembly-ordinary")
    ordinary(owner, {"name": "assembly", "snapshot": identity(built)}, args.configuration)
    after = chunks(owner.saves / "ordinary-resave.sav")[1]
    for key in exact:
        assert before[key] == after[key], ("ordinary paused reload", key)
    kinds = ("missing", "count", "generation", "receipt", "capital", "work", "orbital", "boolean", "home-generation",
             "origin-generation", "physical-position", "physical-model", "slot-missing", "slot-count", "slot-generation",
             "slot-duplicate", "slot-service", "slot-center")
    physical = {"physical-position", "physical-model"}
    for kind in kinds:
        owner = context.for_feature("space", "assembly-reject-" + kind)
        corrupt = damage(owner, built, kind)
        native(owner, args.configuration, "continue", corrupt, reject=True)
        if kind not in physical:
            native(context.for_feature("space", "assembly-preview-" + kind), args.configuration, "continue", built, corrupt)
    assert identity(retained) == original
    report = context.reports / "launch-assembly-verification.json"
    report.write_text(json.dumps({"configuration": args.configuration, "retained": original, "interrupted": identity(built),
                                 "paid_native_models": [14, 15, 19, 20], "capital": 660000, "consumed_steel": 22, "consumed_goods": 36,
                                 "same_site_recovery_fee": 0, "exact_prepared_and_reloaded_results": 3,
                                 "ordinary_exact_chunks": [key.decode() for key in exact], "native_corrupt_rejections": len(kinds),
                                 "independent_preview_rejections": len(kinds) - len(physical),
                                 "native_workshop_images": [identity(path) for path in sorted(prepare.images.glob("*.png"))],
                                 "remaining_scope": "full launch/transition/deployment, supplies/pods, paid service, role producers, art and acceptance"}, indent=2) + "\n", encoding="utf-8")
    print(f"Paid native Earth hull assembly verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
