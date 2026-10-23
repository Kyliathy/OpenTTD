"""Run bounded actual capsule art checks with native packet/work and viewport evidence.

@file tube_capsule_sprite_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13CB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
"""
import argparse
import json
from pathlib import Path
import re
import sys

from PIL import Image

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from runner import current_run_context, save_native_capture
from local_electrical_feed_checks import identity, native
from geothermal_tube_work_files import compare_chunks

FOCUSES = {"body": "[tube-capsule-art-body]", "purchase": "[tube-capsule-art-purchase]",
           "prior-purchase": "[m1-control-selection]", "prior-body": "[m1-train-art]", "meter": "[transport-tube-meter]"}
SOURCES = ("src/transport_tube_sprite.h", "src/transport_tube_vehicle_sprite.cpp", "src/multilayer_sprite.h",
           "src/multilayer_sprite.cpp", "src/train_cmd.cpp", "src/transport_tube.h", "src/transport_tube_model.h",
           "src/transport_tube_engine.cpp", "src/transport_tube_runtime.cpp", "src/transport_tube_power.cpp",
           "src/tests/transport_tube_vehicle_sprite.cpp", "src/tests/transport_tube_sprite_load.cpp",
           "src/tests/transport_tube_meter_fixture.h", "src/tests/transport_tube_fixture.h", "src/tests/native_art_render_fixture.h",
           "src/table/multilayer_sprite_assets.h", "src/CMakeLists.txt", "src/tests/CMakeLists.txt")


def match_capsule_pixels(context, capsule, delta):
    """Require the original authored upper hull to appear at the native vehicle anchor.

    @param context Actual loaded-world native frames and output owners.
    @param capsule Independently checked original model/load/status/direction/frame selector result.
    @param delta Native body-bounds projected offset from its tile-centred viewport, in normal-scale pixels.
    @return Hash-bound source, native palette and exact unobscured upper-hull pixel counts at both zooms.
    """
    manifest_path = ROOT / "logs/proofs/mlm/native-art/ground/mole-source-manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    atlas = manifest["atlases"][0]
    atlas_path = ROOT / atlas["path"]
    assert identity(atlas_path)["sha256"] == atlas["sha256"]
    state = "partial" if capsule["state"] == 1 else "blocked-partial"
    view = ("N", "NE", "E", "SE", "S", "SW", "W", "NW")[capsule["view"]]
    row = next(row for row in atlas["sprites"] if (row["state"], row["direction"], row["frame"]) == (state, view, capsule["frame"]))
    x, y, width, height = row["box"]
    piece = Image.open(atlas_path).crop((x, y, x + width, y + height))
    palette_path = ROOT / "src/table/palettes.h"
    palette_text = palette_path.read_text().split("static const Palette _palette = {")[1].split("\n};")[0]
    palette = [(0, 0, 0)] + [tuple(map(int, rgb)) for rgb in re.findall(r"M\(\s*(\d+),\s*(\d+),\s*(\d+)\)", palette_text)]
    assert len(palette) == 256
    top = min(offset // width for offset, index in enumerate(piece.get_flattened_data()) if index != 0)
    # The upper six source rows sit above the intended near-rim occlusion.
    # Exclude only native company-remap indices, whose company palette has a
    # different owner; every other authored pixel in this fixed window is exact.
    points = [(offset % width, offset // width, palette[index]) for offset, index in enumerate(piece.get_flattened_data())
              if index != 0 and not 198 <= index <= 205 and offset // width < top + 6]
    assert len(points) > 20
    matches = []
    # Actual body bounds and source anchor determine placement independently
    # of picture matching. Both viewport scales must retain this whole window.
    for zoom, scale in ((2, 1), (1, 2)):
        frame_path = context.images / f"tube-capsule-zoom-{zoom}-0.png"
        frame = Image.open(frame_path).convert("RGB")
        left, upper = 320 + (delta[0] + row["anchor"][0]) * scale, 180 + (delta[1] + row["anchor"][1]) * scale
        for px, py, rgb in points:
            for dy in range(scale):
                for dx in range(scale):
                    actual = frame.getpixel((left + px * scale + dx, upper + py * scale + dy))
                    assert actual == rgb, (zoom, px, py, actual, rgb)
        matches.append({"frame": identity(frame_path), "zoom": zoom, "source_points": len(points), "rgb_pixels_exact": len(points) * scale * scale})
    return {"manifest": identity(manifest_path), "atlas": identity(atlas_path), "palette": identity(palette_path),
            "state": state, "direction": view, "frame": capsule["frame"], "native_projected_delta": delta,
            "source_upper_window_y": [top, top + 6], "matches": matches}


def raster(context, configuration, writer_report, case):
    """Observe an original moving or power-wait capsule through the actual loaded viewport.

    @param context Reserved runtime, report, image and diagnostic owners for this invocation.
    @param configuration Actual compiled native reader configuration.
    @param writer_report Immutable original accepted current work writer, not a reconstructed scene.
    @param case Native moving0 or cable-wait3 checkpoint; source closure precedes read-only capture in3.
    @return Exact native selectors, complete before/after owner chunks and repeated viewport identities.
    """
    writer_path = ROOT / writer_report
    writer = json.loads(writer_path.read_text(encoding="utf-8"))
    checkpoint = next(row for row in writer["checkpoints"] if row["phase"] == case and row["step"] == 0)
    source = ROOT / checkpoint["input"]["path"]
    assert identity(source) == checkpoint["input"]
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md13CB.\ntube_guideway_art\n", encoding="utf-8")
    output, proof = native(context, configuration, "[.tube-guideway-art-runtime]", f"capsule-raster-{case}",
                           {"OTTD_TUBE_ART_INPUT": str(source), "OTTD_TUBE_ART_CLOSE_EXTERNAL": "1" if case == 3 else "0",
                            "OTTD_TUBE_ART_CAPSULE": "1"})
    body = re.search(r"TUBE_CAPSULE_ART vehicle=(\d+) model=(\d+) state=(\d+) view=(\d+) frame=(\d+) sprite=(\d+) cargo=(\d+) tile=(\d+) cache_exact=true", output)
    boundary = re.search(r"TUBE_GUIDEWAY_ART tile=\d+ energized=\w+ count=\d+ back=\d+ front=\d+ before=([^\r\n]+) after=([^\r\n]+)", output)
    assert body is not None and boundary is not None, "Native capsule selector/cache and complete raster boundary did not finish."
    proof.update(writer=identity(writer_path), input=identity(source), before=identity(boundary[1]), after=identity(boundary[2]),
                 capsule=dict(zip(("vehicle", "model", "state", "view", "frame", "sprite", "cargo", "tile"), map(int, body.groups()))))
    assert proof["capsule"]["model"] == 9
    assert proof["capsule"]["state"] == (1 if case == 0 else 7)
    assert proof["capsule"]["cargo"] == 17
    proof["complete_chunks_exact"] = compare_chunks(Path(boundary[1]), Path(boundary[2]), None)
    frames = []
    # Both repeated native zooms retain every actual hull/cargo/casing pixel;
    # PNG is only a lossless container for the renderer's original RGB bytes.
    for zoom in (1, 2):
        previous = None
        for repeat in range(2):
            ppm = context.images / f"tube-capsule-zoom-{zoom}-{repeat}.ppm"
            frame = Image.open(ppm).convert("RGB")
            assert frame.size == (640, 360) and len(set(frame.get_flattened_data())) > 8
            pixels = frame.tobytes()
            if previous is not None:
                assert pixels == previous, "Repeated native capsule capture changed RGB pixels."
            previous = pixels
            target = ppm.with_suffix(".png")
            save_native_capture(context, frame, ppm, target)
            assert Image.open(target).convert("RGB").tobytes() == pixels
            frames.append(identity(target))
    proof["frames"] = frames
    anchor = re.search(r"TUBE_CAPSULE_ANCHOR dx=(-?\d+) dy=(-?\d+) native_bounds=true", output)
    assert anchor is not None
    proof["packed_pixels"] = match_capsule_pixels(context, proof["capsule"], [int(anchor[1]), int(anchor[2])])
    proof["cache_exact"] = True
    travel = re.search(r"TUBE_CAPSULE_SETUP native_travel_ticks=(\d+) tile=\d+ speed=\d+ visible=true", output)
    assert travel is not None
    proof["native_travel_ticks_before_render_boundary"] = int(travel[1])
    proof["fixture_classification"] = "Original actual travel/cable-wait file; declared native recovery/travel clears the depot roof, then source closure/native braking prepares blackout. Pure rendering starts afterward; no coordinate edits or cargo loading."
    assert identity(source) == checkpoint["input"]
    return proof


def main():
    """Run one isolated capsule or affected prior-art case and retain hash-bound native evidence.

    @return None; any failed/empty native case refuses a passing report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=(*FOCUSES, "raster"), required=True)
    parser.add_argument("--writer-report")
    parser.add_argument("--case", choices=(0, 3), type=int, default=0)
    args = parser.parse_args()
    context = current_run_context("tubes", "capsule-art")
    context.prepare_runtime()
    if args.focus == "raster":
        assert args.writer_report is not None
        proof = raster(context, args.config, args.writer_report, args.case)
    else:
        _, proof = native(context, args.config, FOCUSES[args.focus], args.focus)
    report = {"_documentation": "r2mlvs17-underground-transport-tubes.md13CB; actual native capsule packet/service/source presentation; prepared unit-test floor/cargo is not useful-route acceptance",
              "configuration": args.config, "focus": args.focus, "case": args.case, "native": proof,
              "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"), "controller": identity(__file__),
              "sources": [identity(ROOT / name) for name in SOURCES], "packed_art": identity(ROOT / "media/baseset/openttd.grf")}
    path = context.output("reports", f"tube-capsule-art-{args.config}-{args.focus}-{args.case}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
