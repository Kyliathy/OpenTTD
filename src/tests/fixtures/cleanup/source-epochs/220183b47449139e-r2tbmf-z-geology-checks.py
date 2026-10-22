"""Bounded serial Group10 checks against one completed optimized build.

@see r2tbmf-tbm-fixes.md Group10; architecture/savegame/archi-savegame.md.
Retain supplied files/configuration; only fresh repository evidence is written.
"""

import argparse
import hashlib
import lzma
import os
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[4]
BUILD = ROOT / "build"
FIXTURES = ROOT / "src/tests/fixtures/haulage"
REVIEWS = ROOT / "docs/zz-reach2/upgrades/2026-10/assets/reviews"
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "src/tests"))
from geology_save_fixture import chunks, encode_chunk, read_stream


def digest(path):
    """@param path Retained file. @return SHA256 of exact bytes without mutation."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fresh(case):
    """@param case Evidence label. @return Fresh log and fixture prefix, preserving all earlier attempts."""
    serial = 1
    while (ROOT / f"logs/r2tbmf-z-{case}-{serial}.log").exists():
        serial += 1
    return ROOT / f"logs/r2tbmf-z-{case}-{serial}.log", FIXTURES / f"tbm-geology-{case}-{serial}"


def run(case, source=None, bad=None, expected=None):
    """Run one bounded native process and restore exact startup/config/input bytes.

    @param case Focused, journey, boundary, native, preview or refusal-load mode.
    @param source Actual retained native input for rendering/resave.
    @param bad Malformed file for preview/full-load refusal.
    @param expected Exact diagnostic required for a malformed file.
    @return Fresh output prefix and complete passing log text.
    """
    log, output = fresh(case)
    startup = BUILD / "scripts/game_start.scr"
    original = startup.read_bytes()
    protected = [BUILD / "native-art.cfg", BUILD / "play.cfg", FIXTURES / "underground-demo-390.sav"]
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
        source = Path(source).resolve()
        assert source.parent == FIXTURES and source.is_file()
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
            result = subprocess.run([str(BUILD / "RelWithDebInfo/openttd_test.exe"), filters], cwd=BUILD, env=env,
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
        for ppm in REVIEWS.glob(output.name + "*.ppm"):
            with Image.open(ppm) as picture:
                picture.save(ppm.with_suffix(".png"))
    return output, evidence


def malformed(source):
    """Check regional header corruptions through both real preview and full loader.

    @param source Actual version399 file; preserve all unrelated chunk bytes.
    @return None; each malformed candidate and exact refusal log is retained.
    """
    source = Path(source).resolve()
    version, stream = read_stream(source)
    assert version == 399 and source.parent == FIXTURES and source.read_bytes()[:4] == b"OTTX"
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
        while (FIXTURES / f"tbm-geology-bad-{name}-{serial}.sav").exists(): serial += 1
        bad = FIXTURES / f"tbm-geology-bad-{name}-{serial}.sav"
        bad.write_bytes(source.read_bytes()[:8] + lzma.compress(body))
        run("preview", source, bad, expected)
        run("refusal-load", source, bad, expected)


def survey():
    """Reuse the delivered native survey/gallery consumer against current art and language.

    @return None; retain fresh material/marker/window captures and restore historical raster outputs exactly.
    """
    from PIL import Image
    log, output = fresh("survey")
    source = FIXTURES / "portal-native-deep-stopped.sav"
    startup = BUILD / "scripts/game_start.scr"
    protected = {path: path.read_bytes() for path in (source, startup, BUILD / "native-art.cfg")}
    names = ["m1-controls-layer-1.ppm", "m1-controls-layer-3.ppm", "m1-survey-layer-1.ppm", "m1-survey-layer-3.ppm",
             "m1-controls-portal.ppm", "m1-purchase-cargo.ppm"]
    for resource in ("COAL", "IORE", "CORE", "BAUX", "SALT", "LITH", "NORE", "COBT", "RARE", "URAN"):
        names += [f"m1-survey-{resource}-{suffix}.ppm" for suffix in ("controls", "zoom-1", "zoom-2")]
    rasters = {REVIEWS / name: (REVIEWS / name).read_bytes() if (REVIEWS / name).exists() else None for name in names}
    env = {key: value for key, value in os.environ.items() if not key.startswith(("OTTD_TBM_", "OTTD_M1_", "OTTD_GEOLOGY_"))}
    env["OTTD_M1_SURVEY_GALLERY"] = "1"
    try:
        startup.write_text("m1_control_art\n", encoding="utf-8")
        with log.open("w", encoding="utf-8") as stream:
            result = subprocess.run([str(BUILD / "RelWithDebInfo/openttd_test.exe"), "[.m1-control-art]"], cwd=BUILD, env=env,
                                    stdout=stream, stderr=subprocess.STDOUT, timeout=120)
        result.check_returncode()
        for path in rasters:
            assert path.exists(), path
            target = REVIEWS / (output.name + "-" + path.stem + ".png")
            with Image.open(path) as picture: picture.save(target)
        print(log.read_text(encoding="utf-8"), flush=True)
    finally:
        for path, original in rasters.items():
            if original is None: path.unlink(missing_ok=True)
            else: path.write_bytes(original)
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
