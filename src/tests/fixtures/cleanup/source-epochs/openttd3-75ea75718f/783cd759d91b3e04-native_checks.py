"""Bounded serial Group10 checks against one completed optimized build.

@see r2tbmf-tbm-fixes.md Group10; architecture/savegame/archi-savegame.md.
Retain supplied files/configuration; only fresh repository evidence is written.
"""

import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import subprocess
import sys
import time

# Repository markers identify the source checkout after tools move; neither
# documentation depth nor the caller's working directory owns runtime output.
ROOT = next(parent for parent in Path(__file__).resolve().parents if (parent / "CMakeLists.txt").is_file() and (parent / "src").is_dir())
import sys
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import resolve_input_path
from runner import save_native_capture, report_provenance, current_run_context, run_native, start_native

CONTEXT = current_run_context("geology", "terrain-hardness")
RUNTIME = CONTEXT.runtime_dir
CONTEXT.prepare_runtime()

BUILD = ROOT / "build"
FIXTURES = ROOT / "src/tests/fixtures/haulage"
REVIEWS = CONTEXT.images
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "src/tests"))
from geology_save_fixture import chunks, encode_chunk, read_stream


def digest(path):
    """@param path Retained file. @return SHA256 of exact bytes without mutation."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fresh(case):
    """@param case Evidence label. @return Fresh log and fixture prefix, preserving all earlier attempts."""
    serial = 1
    while (CONTEXT.logs / f"r2tbmf-z-{case}-{serial}.log").exists():
        serial += 1
    return CONTEXT.logs / f"r2tbmf-z-{case}-{serial}.log", CONTEXT.saves / f"tbm-geology-{case}-{serial}"


def run(case, source=None, bad=None, expected=None):
    """Run one bounded native process and restore exact startup/config/input bytes.

    @param case Focused, journey, boundary, native, preview or refusal-load mode.
    @param source Actual retained native input for rendering/resave.
    @param bad Malformed file for preview/full-load refusal.
    @param expected Exact diagnostic required for a malformed file.
    @return Fresh output prefix and complete passing log text.
    """
    log, output = fresh(case)
    startup = RUNTIME / "scripts/game_start.scr"
    original = startup.read_bytes()
    protected = [BUILD / "native-art.cfg", BUILD / "play.cfg", resolve_input_path(FIXTURES / "underground-demo-390.sav")]
    env = {key: value for key, value in os.environ.items() if not key.startswith(("OTTD_TBM_", "OTTD_GEOLOGY_"))}
    command = None
    if case in ("focused", "journey", "boundary"):
        filters = {"focused": "[geology-regional],[geology-generation],[geology-generation-boundary],[geology-state],[geology-state-refusal],[geology-state-legacy],[m1-terrain-portal]",
                   "journey": "[tbm-geology]", "boundary": "[tbm-geology-boundary]"}[case]
        env["OTTD_TBM_LOGISTICS_RECORD"] = output.name
    elif case == "refusal-load":
        filters = "[.tbm-motion-reload]"
        env["OTTD_TBM_PACE_BAD_LOAD"] = str(bad)
        protected.append(bad)
        command = "tbm_motion\n"
    else:
        source = resolve_input_path(source)
        assert resolve_input_path(source).is_file(), source
        protected.append(source)
        env.update(OTTD_GEOLOGY_SOURCE=str(source), OTTD_GEOLOGY_OUTPUT=str(output))
        filters = "[.m1-terrain-art]"
        command = "m1_terrain_art\n"
        if bad:
            env["OTTD_GEOLOGY_BAD_PREVIEW"] = str(bad)
            protected.append(bad)
    before = {path: digest(path) for path in protected}
    started = time.monotonic()
    try:
        if command:
            startup.write_text(command, encoding="utf-8")
        with log.open("w", encoding="utf-8") as stream:
            result = run_native(CONTEXT, [str(BUILD / "RelWithDebInfo/openttd_test.exe"), filters], cwd=BUILD, env=env,
                                    stdout=stream, stderr=subprocess.STDOUT, timeout=120)
    finally:
        startup.write_bytes(original)
        for path, value in before.items():
            assert digest(path) == value, path
    evidence = log.read_text(encoding="utf-8")
    print(f"Geology {case}: result={result.returncode}, seconds={time.monotonic()-started:.2f}, log={log.relative_to(ROOT)}", flush=True)
    for line in evidence.splitlines():
        if line.startswith(("ROCK_REGIONS", "REGIONAL_", "M1_TERRAIN", "PACE_REFUSED", "All tests passed", "test cases:", "assertions:")):
            print(line, flush=True)
    result.check_returncode()
    assert "All tests passed" in evidence, log
    if expected:
        assert expected in evidence, log
    if case == "native":
        from PIL import Image
        frames = []
        # Regional terrain evidence retains every native frame at its explicit
        # feature path; conversion cannot alter scene dimensions or RGB bytes.
        for ppm in CONTEXT.capture_paths(output.name + "*.ppm"):
            with Image.open(ppm) as picture:
                target = ppm.with_suffix(".png")
                save_native_capture(CONTEXT, picture, ppm, target)
                assert Image.open(target).tobytes() == picture.tobytes()
            frames.append({"path": target.relative_to(ROOT).as_posix(), "sha256": digest(target)})
        assert len(frames) == 8, "regional observer did not emit all native/picker frames"
        assert "M1_TERRAIN_RUNTIME frames=8 picker_views=16 actual_viewport_render=true native_and_2x=true owners_unchanged=true" in evidence
        report = {"_documentation": "Owner: r2fc-file-cleanup.md and r2tbmf-tbm-fixes.md; actual regional terrain capture",
                  "source": source.relative_to(ROOT).as_posix(), "source_sha256": digest(source),
                  "log": log.relative_to(ROOT).as_posix(), "frames": frames,
                  "simulation_owners_unchanged_during_render": True, "user_review": "deferred_until_integration"}
        report_provenance(CONTEXT, report, __file__, "RelWithDebInfo")
        (CONTEXT.reports / "geology-runtime.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return output, evidence


def malformed(source):
    """Check regional header corruptions through both real preview and full loader.

    @param source Actual version399 file; preserve all unrelated chunk bytes.
    @return None; each malformed candidate and exact refusal log is retained.
    """
    source = resolve_input_path(source)
    version, stream = read_stream(source)
    assert version == 399 and source.read_bytes()[:4] == b"OTTX"
    definitions = (("algorithm", "Invalid geology hardness state"), ("legacy-seed", "Invalid geology hardness state"),
                   ("inactive", "Invalid geology hardness state"), ("wire", "Unsupported geology wire version"),
                   ("truncated", "Truncated geology header"))
    for name, expected in definitions:
        rows = chunks(stream)
        header = next(row for row in rows if row["name"] == b"GEOS")
        payload = bytearray(header["payload"])
        assert int.from_bytes(payload[29:31], "big") == 1, "Malformed regional cases require a real new regional world"
        if name == "algorithm": payload[29:31] = (2).to_bytes(2, "big")
        if name == "legacy-seed": payload[29:39] = bytes(2) + (1).to_bytes(8, "big")
        if name == "inactive": payload[2:4] = bytes(2)
        if name == "wire": payload[0:2] = (1).to_bytes(2, "big")
        if name == "truncated": payload = payload[:38]
        header["payload"] = bytes(payload)
        header["raw"] = encode_chunk(header)
        body = b"".join(row["raw"] for row in rows) + bytes(4)
        serial = 1
        while (CONTEXT.saves / f"tbm-geology-bad-{name}-{serial}.sav").exists(): serial += 1
        bad = CONTEXT.saves / f"tbm-geology-bad-{name}-{serial}.sav"
        bad.write_bytes(source.read_bytes()[:8] + lzma.compress(body))
        run("preview", source, bad, expected)
        run("refusal-load", source, bad, expected)


def survey():
    """Reuse the delivered native survey/gallery consumer against current art and language.

    @return None; retain this invocation's material/marker/window captures and preserve inputs.
    """
    from PIL import Image
    log, output = fresh("survey")
    source = resolve_input_path(FIXTURES / "portal-native-deep-stopped.sav")
    startup = RUNTIME / "scripts/game_start.scr"
    protected = {path: path.read_bytes() for path in (source, startup, BUILD / "native-art.cfg")}
    names = ["m1-controls-layer-1.ppm", "m1-controls-layer-3.ppm", "m1-survey-layer-1.ppm", "m1-survey-layer-3.ppm",
             "m1-controls-portal.ppm", "m1-purchase-cargo.ppm"]
    for resource in ("COAL", "IORE", "CORE", "BAUX", "SALT", "LITH", "NORE", "COBT", "RARE", "URAN"):
        names += [f"m1-survey-{resource}-{suffix}.ppm" for suffix in ("controls", "zoom-1", "zoom-2")]
    env = {key: value for key, value in os.environ.items() if not key.startswith(("OTTD_TBM_", "OTTD_M1_", "OTTD_GEOLOGY_"))}
    env["OTTD_M1_SURVEY_GALLERY"] = "1"
    try:
        startup.write_text("m1_control_art\n", encoding="utf-8")
        with log.open("w", encoding="utf-8") as stream:
            result = run_native(CONTEXT, [str(BUILD / "RelWithDebInfo/openttd_test.exe"), "[.m1-control-art]"], cwd=BUILD, env=env,
                                    stdout=stream, stderr=subprocess.STDOUT, timeout=120)
        result.check_returncode()
        from runner import capture_path
        captures = []
        # The observer emits several features. Resolve each expected frame in
        # this invocation and keep the PNG beside its actual native producer.
        for name in names:
            path = capture_path(CONTEXT, name)
            target = path.with_suffix(".png")
            with Image.open(path) as picture: save_native_capture(CONTEXT, picture, path, target)
            captures.append(target)
        CONTEXT.write_report("survey-provenance.json", Path(__file__).relative_to(ROOT).as_posix(),
                             captures, [source], "RelWithDebInfo", "[.m1-control-art]")
        print(log.read_text(encoding="utf-8"), flush=True)
    finally:
        startup.write_bytes(protected[startup])
        for path, original in protected.items(): assert path.read_bytes() == original, path


def main():
    """@return None; execute only the requested unverified or failed slice against the existing build."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--case", choices=("focused", "journey", "boundary", "native", "malformed", "survey"), required=True)
    parser.add_argument("--source")
    args = parser.parse_args()
    if args.case == "survey": survey()
    elif args.case == "malformed": malformed(args.source)
    else: run(args.case, args.source)


if __name__ == "__main__":
    main()
