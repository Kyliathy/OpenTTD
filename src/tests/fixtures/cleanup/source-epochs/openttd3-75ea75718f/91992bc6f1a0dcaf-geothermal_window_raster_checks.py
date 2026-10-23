"""Capture actual geothermal controls and natural survey from retained paid-world originals.

@file geothermal_window_raster_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15CDC.
@see docs/zz-reach2/architecture/gui/archi-gui.md and archi-viewport.md.
Every image comes from the ordinary application renderer; PNG conversion preserves
the exact native RGB pixels and never synthesizes widgets, labels or world art.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context, run_native, save_native_capture
sys.path.insert(0, str(ROOT / "src/tests"))
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks
from PIL import Image, ImageChops

CONTEXT = current_run_context("geothermal", "window-raster")


def identity(path):
    """Identify one exact native input or produced artifact.

    @param path Existing repository-owned file.
    @return Relative path, byte length and SHA256.
    """
    path = Path(path).resolve()
    return {"path": path.relative_to(ROOT).as_posix(), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def complete_chunks(path):
    """Inspect every complete native owner after ordinary load/save.

    @param path Actual application-written before/after checkpoint.
    @return Native version and every complete raw chunk hash, without a whitelist of skipped quantities.
    """
    version, stream = read_stream(path)
    rows = chunks(stream)
    return {"version": version, "chunks": {row["name"].decode(): hashlib.sha256(row["raw"]).hexdigest() for row in rows}}


def main():
    """Run one isolated ordinary loaded-world raster observation and retain exact proof.

    @return None; missing frames, mutation, clipping, stale originals or failed native tests abort acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--scene", choices=("solid", "funded", "partial", "vent-tap", "magma-loop"), required=True)
    parser.add_argument("--input", type=Path, required=True)
    args = parser.parse_args()
    original = identity(args.input)
    config = CONTEXT.prepare_runtime()
    startup = CONTEXT.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# @see r2mlvs18-geothermal-energy-and-industry.md Group15CDC.\ngeothermal_window_raster\n", encoding="utf-8")
    environment = os.environ.copy()
    environment.update(OTTD_GEO_RASTER_INPUT=str(args.input.resolve()), OTTD_GEO_RASTER_SCENE=args.scene,
                       OTTD_TEST_CONFIGURATION=args.config)
    binary = ROOT / f"build/{args.config}/openttd_test.exe"
    result = run_native(CONTEXT, [binary, "[.geothermal-window-raster]"], cwd=ROOT / "build", env=environment,
                        capture_output=True, timeout=110, check=False)
    output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    log = CONTEXT.logs / f"{args.config}-{args.scene}.log"
    log.write_text(output + f"\nNATIVE_EXIT_CODE={result.returncode}\n", encoding="utf-8")
    assert result.returncode == 0 and "All tests passed" in output, log
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", output)
    assert counts and int(counts[2]) == 1, log
    assert f"GEO_RASTER_READY scene={args.scene} native_owners_unchanged=true rng_unchanged=true" in output, log
    assert identity(args.input) == original, "Rendering replaced or modified its retained paid-world input"
    before, after = CONTEXT.saves / "world-before.sav", CONTEXT.saves / "world-after.sav"
    initial, final = complete_chunks(before), complete_chunks(after)
    assert initial == final, {"changed_chunks": sorted(key for key in initial["chunks"].keys() | final["chunks"].keys()
                                                      if initial["chunks"].get(key) != final["chunks"].get(key))}
    frames = []
    # Convert only this process's recorded native PPMs. The shared conversion
    # helper validates exact pixels and links each PNG to its actual producer.
    for source in sorted(CONTEXT.images.glob("*.ppm")):
        with Image.open(source) as native:
            raster = native.copy()
        target = source.with_suffix(".png")
        save_native_capture(CONTEXT, raster, source, target)
        frames.append(dict(identity(target), native_container=identity(source), size=list(raster.size)))
    assert len(frames) >= 9, "Missing actual window/viewport frames"
    with Image.open(CONTEXT.images / "vent-inactive.png") as inactive, Image.open(CONTEXT.images / "vent-active.png") as active, Image.open(CONTEXT.images / "vent-cancelled.png") as cancelled:
        assert inactive.size == active.size == cancelled.size
        assert inactive.tobytes() == cancelled.tobytes(), "Cancelled cursor left actual world pixels behind"
        delta = ImageChops.difference(inactive, active).getbbox()
        assert delta is not None, "Active original survey produced no actual vent pixels"
    windows = [json.loads(line.removeprefix("GEO_RASTER_WINDOW ")) for line in output.splitlines() if line.startswith("GEO_RASTER_WINDOW ")]
    assert windows and all(window["status_text_height"] <= window["status_height"] for window in windows)
    report = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md Group15CDC; actual ordinary loaded paid-world window/survey raster",
              "configuration": args.config, "scene": args.scene, "input": original, "binary": identity(binary), "log": identity(log),
              "case_runs": 1, "assertions": int(counts[1]), "frames": frames, "windows": windows,
              "before": identity(before), "after": identity(after), "all_complete_native_chunks_equal": True,
              "native_chunks": initial, "source_unchanged": True, "cancelled_pixels_equal_inactive": True,
              "active_overlay_difference_bbox": list(delta), "actual_pixels": True,
              "sources": [identity(ROOT / name) for name in ("src/tests/geothermal_window_raster.cpp", "src/tests/native_art_render_fixture.h",
                  "src/geothermal_gui.h", "src/geothermal_gui.cpp", "src/underground_gui.cpp", "src/underground_gui.h", "src/viewport.cpp",
                  "src/utility_power.cpp", "src/utility_power.h", "src/geothermal_generator.cpp", "src/geothermal_generator.h",
                  "src/lang/english.txt", "src/widgets/underground_widget.h", "src/tests/CMakeLists.txt",
                  "src/tests/multilayer/utilities/geothermal_window_raster_checks.py")]}
    path = CONTEXT.output("reports", f"geothermal-window-raster-{args.config}-{args.scene}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"{args.config}-{args.scene}: {len(frames)} actual native frames; every native chunk/RNG/source unchanged\n{path}")


if __name__ == "__main__":
    main()
