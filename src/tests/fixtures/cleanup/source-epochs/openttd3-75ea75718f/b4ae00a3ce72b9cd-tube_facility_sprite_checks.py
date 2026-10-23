"""Run isolated actual tube facility geometry, service, stock and affected owner checks.

@file tube_facility_sprite_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups13CC/13DCC.
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

FOCUSES = {"geometry": "[tube-facility-art-geometry]", "service": "[tube-facility-art-service]",
           "stock": "[tube-facility-art-stock]", "readiness": "[transport-tube-readiness]",
           "interchange": "[transport-tube-interchange-geometry]", "guideway": "[tube-guideway-art-external]",
           "capsule": "[tube-capsule-art-body]", "meter": "[transport-tube-meter]", "ui": "[underground-toolbar]"}
SOURCES = ("src/underground_gui.h", "src/underground_gui.cpp", "src/tests/underground_window.cpp", "src/tile_cmd.h",
           "src/depot_gui.cpp", "src/station_gui.cpp", "src/vehicle.cpp",
           "src/transport_tube_sprite.h", "src/transport_tube_sprite.cpp", "src/transport_tube_facility_sprite.cpp",
           "src/multilayer_sprite.cpp", "src/transport_tube.h", "src/transport_tube_readiness.cpp",
           "src/rail_cmd.cpp", "src/station_cmd.cpp", "src/animated_tile.cpp", "src/animated_tile_map.h",
           "src/tests/transport_tube_facility_sprite.cpp", "src/tests/transport_tube_facility_sprite_load.cpp", "src/tests/excavation_fixture.h",
           "src/tests/transport_tube_fixture.h", "src/tests/transport_tube_meter_fixture.h",
           "src/tests/geothermal_tube_switch_fixture.h", "src/tests/geothermal_tube_work_fixture.h",
           "src/tests/geothermal_tube_capsule_fixture.h", "src/tests/geothermal_tube_power_fixture.h",
           "src/tests/geothermal_generator_operating_fixture.h", "src/tests/layer_save_fixture.h", "src/tests/native_art_render_fixture.h",
           "src/table/multilayer_sprite_assets.h", "src/CMakeLists.txt", "src/tests/CMakeLists.txt")


def write_scene(context, configuration, scene):
    """Build one genuine ordinary native scene through existing physical and electrical owners.

    @param context Reserved native runtime, save, report and log owners.
    @param configuration Actual compiled writer configuration.
    @param scene Declared depot-ready0/service1, loading2, interchange3, full-queue4 or genuinely source-closed depot5.
    @return Original native file identity, complete scalar observations and actual native process evidence.
    """
    output, proof = native(context, configuration, "[tube-facility-art-write]", f"write-{scene}",
                           {"OTTD_TUBE_FACILITY_SCENE": str(scene)})
    match = re.search(r"TUBE_FACILITY_SAVE ((?:\w+=\d+ )+)path=([^\r\n]+)", output)
    assert match is not None
    fields = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+)", match[1])}
    assert fields["scene"] == scene
    assert fields["state"] == (2 if scene in (1, 2) else 3 if scene in (4, 5) else 1)
    proof.update(input=identity(match[2]), fields=fields)
    return proof


def match_facility_pixels(context, fields):
    """Require the complete original status-glass component at its actual native building anchor.

    @param context Actual loaded native viewport frames.
    @param fields Independently retained original scene geometry/status/frame fields.
    @return Exact source/palette/status-component identities and native pixel counts at both zooms.
    """
    scene = fields["scene"]
    name = "tube-service" if scene in (0, 1, 5) else "tube-interchange" if scene == 3 else "tube-terminal"
    manifest_path = ROOT / f"logs/proofs/mlm/native-art/ground/{name}-atlas-manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    atlas = manifest["atlases"][0] if "atlases" in manifest else manifest
    atlas_path = ROOT / atlas["path"]
    assert identity(atlas_path)["sha256"] == atlas["sha256"]
    state = ("construction", "ready", "working", "blocked")[fields["state"]]
    direction = ("NE", "SE", "SW", "NW")[fields["view"]]
    row = next(row for row in atlas["sprites"] if (row["state"], row["frame"], row["direction"], row["tile_variant"], row["part"])
               == (state, fields["frame"], direction, fields["role"], "back"))
    x, y, width, height = row["box"]
    piece = Image.open(atlas_path).crop((x, y, x + width, y + height))
    component_record = None
    if scene in (0, 1, 5):
        component_record = ROOT / "media/multilayer/source/m1/facility-generation-record.json"
        record = json.loads(component_record.read_text(encoding="utf-8"))
        family = next(row for row in record["families"] if row["asset_id"] == "facility.rail-service")
        target = family["cue_targets"][fields["view"]]
    else:
        target = row["indicator"]["target"]
    palette_path = ROOT / "src/table/palettes.h"
    palette_text = palette_path.read_text().split("static const Palette _palette = {")[1].split("\n};")[0]
    palette = [(0, 0, 0)] + [tuple(map(int, rgb)) for rgb in re.findall(r"M\(\s*(\d+),\s*(\d+),\s*(\d+)\)", palette_text)]
    assert len(palette) == 256
    cx, cy, cw, ch = target
    assert (cw, ch) == (3, 3)
    points = [(px, py, palette[piece.getpixel((px, py))]) for py in range(cy, cy + ch) for px in range(cx, cx + cw)]
    assert all(piece.getpixel((px, py)) != 0 for px, py, _ in points)
    matches = []
    # Tile-centred native capture puts its north ground origin16px above the
    # middle. The original building(-31,-33) anchor then places all nine glass
    # pixels independently of image searching or similarity tolerances.
    for zoom, scale in ((2, 1), (1, 2)):
        frame_path = context.images / f"tube-facility-zoom-{zoom}-0.png"
        image = Image.open(frame_path).convert("RGB")
        left, top = 320 + row["anchor"][0] * scale, 180 + (row["anchor"][1] - 16) * scale
        for px, py, rgb in points:
            for dy in range(scale):
                for dx in range(scale):
                    actual = image.getpixel((left + px * scale + dx, top + py * scale + dy))
                    assert actual == rgb, (scene, zoom, px, py, actual, rgb)
        matches.append({"frame": identity(frame_path), "zoom": zoom, "rgb_pixels_exact": len(points) * scale * scale})
    return {"manifest": identity(manifest_path), "atlas": identity(atlas_path), "palette": identity(palette_path),
            "component_record": identity(component_record) if component_record else None,
            "state": state, "frame": fields["frame"], "direction": direction, "role": fields["role"], "source_target": target, "matches": matches}


def raster(context, configuration, writer_report, scene):
    """Load an exact opposite-build native facility scene and observe its original owners and pixels.

    @param context Reserved native runtime, image, report and log owners.
    @param configuration Actual compiled reader configuration.
    @param writer_report Exact accepted native scene writer report for this scene.
    @param scene Declared native scene0..5, including genuine source-closed depot5, independently pinned by the writer.
    @return Original input, native fields, exact complete chunks and repeated actual frame identities.
    """
    writer_path = ROOT / writer_report
    writer = json.loads(writer_path.read_text(encoding="utf-8"))
    assert writer["focus"] == "write" and writer["scene"] == scene
    fields = writer["native"]["fields"]
    source = ROOT / writer["native"]["input"]["path"]
    assert identity(source) == writer["native"]["input"]
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md13CC.\ntube_facility_art\n", encoding="utf-8")
    environment = {f"OTTD_TUBE_FACILITY_{key.upper()}": str(fields[key]) for key in
                   ("scene", "tile", "state", "tick", "packet", "vehicle", "utility", "invoices", "binding", "geothermal", "generator")}
    environment["OTTD_TUBE_FACILITY_INPUT"] = str(source)
    output, proof = native(context, configuration, "[.tube-facility-art-runtime]", f"raster-{scene}", environment)
    match = re.search(r"TUBE_FACILITY_ART ((?:\w+=\d+ )+)before=([^\r\n]+) after=([^\r\n]+)", output)
    assert match is not None
    selection = re.search(rf"TUBE_FACILITY_PICK scene={scene} native_window=true body_hidden=(true|false) correct_layer=true wrong_layer_refused=true", output)
    assert selection is not None and (selection[1] == "true") == (scene != 2)
    observed = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+)", match[1])}
    assert observed == {key: fields[key] for key in observed}
    proof.update(writer=identity(writer_path), input=identity(source), fields=fields, before=identity(match[2]), after=identity(match[3]))
    proof["complete_chunks_exact"] = compare_chunks(Path(match[2]), Path(match[3]), None)
    frames = []
    # Preserve every native RGB byte and require repeated complete frames at
    # both real viewport zooms; PNG conversion changes only the file container.
    for zoom in (1, 2):
        previous = None
        for repeat in range(2):
            ppm = context.images / f"tube-facility-zoom-{zoom}-{repeat}.ppm"
            frame = Image.open(ppm).convert("RGB")
            assert frame.size == (640, 360) and len(set(frame.get_flattened_data())) > 8
            pixels = frame.tobytes()
            if previous is not None:
                assert pixels == previous
            previous = pixels
            target = ppm.with_suffix(".png")
            save_native_capture(context, frame, ppm, target)
            assert Image.open(target).convert("RGB").tobytes() == pixels
            frames.append(identity(target))
    proof["frames"] = frames
    proof["selection"] = {"native_window": True, "body_hidden": selection[1] == "true", "correct_layer": True, "wrong_layer_refused": True}
    proof["packed_pixels"] = match_facility_pixels(context, fields)
    proof["fixture_classification"] = "Original ordinary command-built native scene; declared geological floor/materials, optional wear/stock and actual driven loading precede writing. The loaded observer earns no native work or cargo."
    assert identity(source) == writer["native"]["input"]
    return proof


def main():
    """Run one bounded native owner case and retain current binary/source/log identities.

    @return None; any native failure or empty Catch selection refuses a passing report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=(*FOCUSES, "write", "raster"), required=True)
    parser.add_argument("--scene", type=int, choices=range(6), default=0)
    parser.add_argument("--writer-report")
    args = parser.parse_args()
    context = current_run_context("tubes", "facility-art")
    context.prepare_runtime()
    if args.focus == "write":
        proof = write_scene(context, args.config, args.scene)
    elif args.focus == "raster":
        assert args.writer_report is not None
        proof = raster(context, args.config, args.writer_report, args.scene)
    else:
        _, proof = native(context, args.config, FOCUSES[args.focus], args.focus)
    report = {"_documentation": "r2mlvs17-underground-transport-tubes.md13DCC; actual native facility display/window/picking owners; prepared unit-test floor/cargo is not bootstrap acceptance",
              "configuration": args.config, "focus": args.focus, "scene": args.scene, "native": proof,
              "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"), "controller": identity(__file__),
              "sources": [identity(ROOT / name) for name in SOURCES], "packed_art": identity(ROOT / "media/baseset/openttd.grf")}
    path = context.output("reports", f"tube-facility-art-{args.config}-{args.focus}-{args.scene}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
