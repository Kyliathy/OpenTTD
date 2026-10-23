"""Verify paid native scans, company-private disclosure and exact partial-work reloads."""
import argparse
import json
import os
import struct
import subprocess
from PIL import Image
from space_preview_checks import ROOT, chunks, identity, ordinary, read_stream
from launch_center_checks import riff_location
from launch_logistics_checks import native as logistics_native
from runner import current_run_context, run_native, save_native_capture


def native(context, configuration, action, source, corrupt=None, reject=False):
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("space_survey_test\n", encoding="utf-8")
    env = dict(os.environ, OTTD_SPACE_SURVEY_ACTION=action, OTTD_SPACE_SURVEY_SOURCE=str(source))
    env.pop("OTTD_SPACE_SURVEY_CORRUPT", None)
    if corrupt is not None:
        env["OTTD_SPACE_SURVEY_CORRUPT"] = str(corrupt)
    log = context.logs / "native-survey.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.space-survey]"],
                            cwd=ROOT / "build", env=env, stdout=sink, stderr=subprocess.STDOUT, timeout=120)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    if reject:
        assert result.returncode != 0 and "All tests passed" not in evidence, log
        assert any(word in evidence.lower() for word in ("asteroid survey knowledge", "space order", "space fleet")), log
        return
    assert result.returncode == 0 and "All tests passed" in evidence and "SPACE_SURVEY native=true" in evidence, log
    if corrupt:
        assert "preview_reject=true authority_unchanged=true" in evidence, log
    for ppm in context.images.glob("*.ppm"):
        with Image.open(ppm) as raster:
            save_native_capture(context, raster, ppm, ppm.with_suffix(".png"))
    return context.saves / ("survey-partial-414.sav" if action == "prepare" else "survey-known-reloaded-414.sav" if action == "known" else "survey-complete-414.sav")


def damage(context, source, kind):
    context.prepare_runtime()
    version, stream = read_stream(source)
    assert version == 414
    family = b"SPOR" if kind.startswith("scan-") else b"SPKN"
    start, body, length, end = riff_location(stream, family)
    changed = bytearray(stream)
    if family == b"SPOR":
        pos = body + 9
        for _ in range(stream[body + 8]):
            count = stream[pos + 21]
            if struct.unpack(">H", stream[pos + 19:pos + 21])[0] == 37:
                break
            pos += 22 + 14 * count
        else:
            raise AssertionError("The partial scan must have its native order work")
        if kind == "scan-work":
            changed[pos + 19:pos + 21] = struct.pack(">H", 148)
        elif kind == "scan-started":
            changed[pos + 13] = 0
        elif kind == "scan-timer":
            changed[pos + 15:pos + 19] = bytes(4)
        elif kind == "scan-cursor":
            changed[pos + 12] = count
        elif kind == "scan-action":
            changed[pos + 22] = 2  # Mine cannot own scan work.
        elif kind == "scan-body-generation":
            changed[pos + 28:pos + 36] = struct.pack(">Q", 1)
        elif kind == "scan-missing-body":
            changed[pos + 24:pos + 28] = struct.pack(">I", 99999)
        else:
            raise ValueError(kind)
    elif kind == "missing":
        del changed[start:end]
    elif kind == "count":
        changed[body:body + 2] = struct.pack(">H", 65535)
    elif kind == "repeated-family":
        changed[end:end] = changed[start:end]
    elif kind == "body":
        changed[body + 2:body + 6] = struct.pack(">I", 99999)
    elif kind in ("empty-mask", "mask-range", "removed-company"):
        value = 0 if kind == "empty-mask" else 1 << 63 if kind == "mask-range" else 1 << 2
        changed[body + 6:body + 14] = struct.pack(">Q", value)
    else:
        raise ValueError(kind)
    target = context.saves / f"survey-{kind}.sav"
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", default="RelWithDebInfo", choices=("RelWithDebInfo", "Debug"))
    args = parser.parse_args()
    context = current_run_context("space", "survey-verify")
    retained = ROOT / "src/tests/fixtures/demos/space/space-launch-hub-1990-408.sav"
    original = identity(retained)
    ground = context.for_feature("space", "survey-ground-production")
    logistics_native(ground, args.configuration, "prepare")
    prepare = context.for_feature("space", "survey-prepare")
    partial = native(prepare, args.configuration, "prepare", ground.saves / "logistics-bootstrap-414.sav")
    complete = prepare.saves / "survey-complete-414.sav"
    version, expected = chunks(complete)
    assert version == 414 and struct.unpack(">H", expected[b"SPKN"][3:5])[0] == 1
    exact = (b"SPKN", b"SPDJ", b"SPFL", b"SPOD", b"SPSU", b"SVST", b"LCOP", b"SDEF", b"SPOR", b"ASTR", b"VEHS", b"CAPA", b"PLYR", b"STNN", b"LCTR", b"LCHB", b"LHND")
    for name in ("resume", "replay"):
        result_path = native(context.for_feature("space", "survey-" + name), args.configuration, "continue", partial)
        result = chunks(result_path)[1]
        for key in exact:
            assert expected[key] == result[key], ("native partial survey continuation", key, identity(complete), identity(result_path))
    known = chunks(native(context.for_feature("space", "survey-known-reload"), args.configuration, "known", complete))[1]
    for key in exact:
        assert expected[key] == known[key], ("completed company knowledge reload", key)
    for name, source in (("partial", partial), ("complete", complete)):
        owner = context.for_feature("space", "survey-ordinary-" + name)
        ordinary(owner, {"name": "survey-" + name, "snapshot": identity(source)}, args.configuration)
        before, after = chunks(source)[1], chunks(owner.saves / "ordinary-resave.sav")[1]
        for key in exact:
            assert before[key] == after[key], ("ordinary paused survey reload", name, key)
    kinds = ("missing", "count", "repeated-family", "body", "empty-mask", "mask-range", "removed-company",
             "scan-work", "scan-started", "scan-timer", "scan-cursor", "scan-action", "scan-body-generation", "scan-missing-body")
    for kind in kinds:
        source = partial if kind.startswith("scan-") else complete
        action = "continue" if kind.startswith("scan-") else "known"
        owner = context.for_feature("space", "survey-reject-" + kind)
        corrupt = damage(owner, source, kind)
        native(owner, args.configuration, action, corrupt, reject=True)
        native(context.for_feature("space", "survey-preview-" + kind), args.configuration, action, source, corrupt=corrupt)
    assert identity(retained) == original
    report = context.reports / "space-survey-verification.json"
    report.write_text(json.dumps({"configuration": args.configuration, "retained": original, "partial": identity(partial), "complete": identity(complete),
                                 "working_ticks": 148, "partial_work_ticks": 37, "cancelled_work_grants_no_knowledge": True,
                                 "company_private": True, "company_reuse": True, "concurrent_native_mining_lease": True,
                                 "ordinary_exact_chunks": [key.decode() for key in exact], "corrupt_rejections": len(kinds),
                                 "native_inspector_images": [identity(path) for path in sorted(prepare.images.glob("*.png"))],
                                 "remaining_scope": "construction, orbital resupply, final art and full integrated/network acceptance"}, indent=2) + "\n", encoding="utf-8")
    print(f"Native survey work and company disclosure verified: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()
