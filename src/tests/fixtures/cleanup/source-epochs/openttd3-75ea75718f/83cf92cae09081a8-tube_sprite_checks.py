"""Run isolated native tube guideway presentation checks with exact evidence.

@file tube_sprite_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13B.
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

FOCUSES = {"geometry": "[tube-guideway-art-geometry]", "ends": "[tube-guideway-art-ends]",
           "external": "[tube-guideway-art-external]", "geothermal": "[tube-guideway-art-geothermal]"}
SOURCES = ("src/transport_tube_sprite.h", "src/transport_tube_sprite.cpp", "src/rail_cmd.cpp",
           "src/multilayer_sprite.h", "src/multilayer_sprite.cpp", "src/table/multilayer_sprite_assets.h",
           "src/transport_tube.h", "src/transport_tube_topology.cpp", "src/transport_tube_power.cpp",
           "src/transport_tube_runtime.cpp", "src/pbs.h", "src/pbs.cpp", "src/rail_map.h",
           "src/transport_tube_geothermal.h", "src/transport_tube_geothermal.cpp", "src/utility_power.h", "src/utility_power.cpp",
           "src/multilayer_state.cpp", "src/saveload/afterload.cpp", "src/saveload/transport_tube_vehicle_sl.cpp",
           "src/saveload/transport_tube_geothermal_sl.cpp",
           "src/saveload/map_sl.cpp", "src/saveload/saveload.h",
           "src/geothermal_generator.cpp", "src/tests/transport_tube_sprite.cpp",
           "src/tests/transport_tube_sprite_load.cpp",
           "src/table/palettes.h", "src/tests/native_art_render_fixture.h",
           "src/tests/transport_tube_fixture.h", "src/tests/transport_tube_meter_fixture.h",
           "src/tests/geothermal_tube_work_fixture.h", "src/CMakeLists.txt", "src/tests/CMakeLists.txt")


def match_guideway_pixels(context, energized):
    """Require exact authored indexed pixels at their actual native viewport ground anchor.

    @param context This ordinary loaded world's actual renderer frames.
    @param energized Native source/path result, independently constrained by the selected retained scene.
    @return Hash-bound atlas/palette/frame inputs and exact unobscured casing pixel counts at both zooms.
    """
    manifest_path = ROOT / "logs/proofs/mlm/native-art/ground/tube-atlas-manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    atlas_path = ROOT / manifest["path"]
    assert identity(atlas_path)["sha256"] == manifest["sha256"]
    state = "powered" if energized else "unpowered"
    row = next(row for row in manifest["sprites"] if (row["state"], row["direction"], row["part"]) == (state, "SE", "front"))
    x, y, width, height = row["box"]
    piece = Image.open(atlas_path).crop((x, y, x + width, y + height))
    # The native DOS palette deliberately differs from the quantizer's legacy
    # grey RGB approximations. Resolve actual packed indices through the exact
    # engine table instead of tolerating pixel differences or normalizing RGB.
    palette_path = ROOT / "src/table/palettes.h"
    palette_text = palette_path.read_text().split("static const Palette _palette = {")[1].split("\n};")[0]
    palette = [(0, 0, 0)] + [tuple(map(int, rgb)) for rgb in re.findall(r"M\(\s*(\d+),\s*(\d+),\s*(\d+)\)", palette_text)]
    assert len(palette) == 256
    # This fixed left near-rim window is unobscured by the adjacent native
    # depot. Its complete authored foreground must remain exact above actual
    # capsules; the whole back may legitimately be covered by a moving body.
    # Complete repeated captures still compare every capsule/roof/ground pixel.
    points = [(offset % width, offset // width, palette[index]) for offset, index in enumerate(piece.get_flattened_data())
              if index != 0 and 10 <= offset % width < 36]
    assert len(points) > 50
    matches = []
    # Native64x31 ground uses its original(-31,0) anchor. The viewport centres
    # the tile's physical midpoint, so north origin is16 pixels above it at1x.
    for zoom, scale in ((2, 1), (1, 2)):
        frame_path = context.images / f"tube-guideway-zoom-{zoom}-0.png"
        frame = Image.open(frame_path).convert("RGB")
        left, top = 320 - 31 * scale, 180 - 16 * scale
        for px, py, rgb in points:
            for dy in range(scale):
                for dx in range(scale):
                    assert frame.getpixel((left + px * scale + dx, top + py * scale + dy)) == rgb, (zoom, px, py)
        matches.append({"frame": identity(frame_path), "zoom": zoom, "source_points": len(points), "rgb_pixels_exact": len(points) * scale * scale})
    return {"manifest": identity(manifest_path), "atlas": identity(atlas_path), "palette": identity(palette_path),
            "state": state, "direction": "SE", "part": "front", "unobscured_source_window": [10, 0, 36, 31], "matches": matches}


def raster(context, configuration, writer_report, case, reservation=False):
    """Prepare declared native art fixtures, then observe original work without advancing it.

    @param context This invocation's native runtime, log, report and review owners.
    @param configuration Actual compiled native reader configuration.
    @param writer_report Immutable accepted six-scene current work writer report.
    @param case Native travel or cable-wait checkpoint zero/three.
    @param reservation Prepare one declared native reservation boundary fixture on the original track before read-only capture.
    @return Actual process evidence, exact render-boundary owners and repeated native frame identities.
    """
    writer_path = ROOT / writer_report
    writer = json.loads(writer_path.read_text(encoding="utf-8"))
    # Use the original physical scene, rather than its uninterrupted next-step
    # baseline, so any optional boundary setup starts from its retained owners.
    checkpoint = next(row for row in writer["checkpoints"] if row["phase"] == case and row["step"] == 0)
    source = ROOT / checkpoint["input"]["path"]
    assert identity(source) == checkpoint["input"], "The original native writer input changed."
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md13B.\ntube_guideway_art\n", encoding="utf-8")
    output, proof = native(context, configuration, "[.tube-guideway-art-runtime]", f"raster-{case}",
                           {"OTTD_TUBE_ART_INPUT": str(source), "OTTD_TUBE_ART_CLOSE_EXTERNAL": "1" if case == 3 else "0",
                            "OTTD_TUBE_ART_RESERVATION": "1" if reservation else "0"})
    match = re.search(r"TUBE_GUIDEWAY_ART tile=(\d+) energized=(true|false) count=(\d+) back=(\d+) front=(\d+) before=([^\r\n]+) after=([^\r\n]+)", output)
    assert match is not None, "The actual loaded viewport observer did not finish."
    proof.update(writer=identity(writer_path), input=identity(source), before=identity(match[6]), after=identity(match[7]),
                 tile=int(match[1]), energized=match[2] == "true", count=int(match[3]), back=int(match[4]), front=int(match[5]))
    proof["complete_chunks_exact"] = compare_chunks(Path(match[6]), Path(match[7]), None)
    frames = []
    # Both native zooms must repeat pixel-for-pixel while complete saved owners
    # remain exact. Container conversion retains every renderer-produced RGB byte.
    for zoom in (1, 2):
        previous = None
        for repeat in range(2):
            ppm = context.images / f"tube-guideway-zoom-{zoom}-{repeat}.ppm"
            frame = Image.open(ppm)
            assert frame.size == (640, 360)
            assert len(set(frame.get_flattened_data())) > 8
            pixels = frame.tobytes()
            if previous is not None:
                assert pixels == previous, "Repeated rendering changed native viewport pixels."
            previous = pixels
            target = ppm.with_suffix(".png")
            save_native_capture(context, frame, ppm, target)
            assert Image.open(target).tobytes() == pixels
            frames.append(identity(target))
    assert identity(source) == checkpoint["input"]
    proof["frames"] = frames
    if case == 3:
        assert "TUBE_GUIDEWAY_ACTION close_external=true" in output
        proof["native_action_before_read_only_capture"] = "Close original still-open paid external service; retain its used history and the selected native capsule's existing topology wait."
    if reservation:
        reserved = re.search(r"TUBE_GUIDEWAY_RESERVATION tracks=(\d+) native_reservation_fixture=true", output)
        assert reserved is not None
        assert int(reserved[1]) != 0
        proof["actual_reserved_tracks"] = int(reserved[1])
        proof["native_reservation_fixture_before_read_only_capture"] = "TryReserveRailTrack reserves the existing original Track::Y; the retained waiting input had no reservation here. No vehicle, source, packet or clock advances."
    assert proof["energized"] == (case == 0), "Retained native travel/cable-wait scene has the wrong supply indication."
    proof["packed_pixels"] = match_guideway_pixels(context, proof["energized"])
    return proof


def main():
    """Execute one bounded native case and retain its actual binary, source and log owners.

    @return None; a native failure or empty test selection refuses the report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=(*FOCUSES, "raster", "reservation"), required=True)
    parser.add_argument("--writer-report", help="Exact retained six-scene work writer, required for raster.")
    parser.add_argument("--case", choices=(0, 3), type=int, default=0)
    args = parser.parse_args()
    context = current_run_context("tubes", "guideway-art")
    context.prepare_runtime()
    if args.focus in ("raster", "reservation"):
        assert args.writer_report is not None
        proof = raster(context, args.config, args.writer_report, args.case, args.focus == "reservation")
    else:
        _, proof = native(context, args.config, FOCUSES[args.focus], args.focus)
    report = {"_documentation": "r2mlvs17-underground-transport-tubes.md Group13B; actual native guideway presentation; prepared fixture floor is not paid bootstrap",
              "configuration": args.config, "focus": args.focus, "case": args.case, "native": proof,
              "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
              "controller": identity(__file__), "sources": [identity(ROOT / name) for name in SOURCES],
              "packed_art": identity(ROOT / "media/baseset/openttd.grf")}
    path = context.output("reports", f"tube-guideway-art-{args.config}-{args.focus}-{args.case}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
