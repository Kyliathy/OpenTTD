"""Run bounded original tube conduit checks and retain exact native file/pixel identities.

@file tube_conduit_sprite_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13CD.
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
from tube_facility_sprite_checks import SOURCES as FACILITY_SOURCES

FOCUSES = {"path": "[tube-conduit-art-path]", "purity": "[tube-conduit-art-purity]", "native": "[tube-conduit-art-native]",
           "guideway": "[tube-guideway-art-external]", "facility": "[tube-facility-art-service]", "meter": "[transport-tube-meter]"}
SOURCES = tuple(dict.fromkeys((*FACILITY_SOURCES, "src/transport_tube_conduit_sprite.cpp", "src/transport_tube_power.cpp",
                             "src/transport_tube_topology.cpp", "src/viewport.cpp", "src/road_cmd.cpp", "src/clear_cmd.cpp",
                             "src/tests/transport_tube_conduit_sprite.cpp", "src/tests/transport_tube_conduit_sprite_load.cpp")))


def match_conduit_pixels(context, scene):
    """Require original DOS branch pixels at their actual common ground anchor.

    @param context Actual native image/review owner.
    @param scene Dark0, published1 or retired2 original service.
    @return Complete palette/atlas identities and exact selected pixel counts; retired history has no claimed source pixels.
    """
    manifest_path = ROOT / "logs/proofs/mlm/native-art/ground/tube-conduit-atlas-manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    atlas_path = ROOT / manifest["path"]
    assert identity(atlas_path)["sha256"] == manifest["sha256"]
    palette_path = ROOT / "src/table/palettes.h"
    palette_text = palette_path.read_text().split("static const Palette _palette = {")[1].split("\n};")[0]
    palette = [(0, 0, 0)] + [tuple(map(int, rgb)) for rgb in re.findall(r"M\(\s*(\d+),\s*(\d+),\s*(\d+)\)", palette_text)]
    assert len(palette) == 256
    atlas = Image.open(atlas_path)
    checks = []
    path = [(x, 23) for x in range(32, 26, -1)] + [(27, 24)] + [(x, 25) for x in range(27, 33)] + [(32, 24)]
    directions = {(-1, 0): 0, (0, 1): 1, (1, 0): 2, (0, -1): 3}
    cells = {}
    for index, cell in enumerate(path):
        adjacent = path[max(0, index - 1):index] + path[index + 1:index + 2]
        cells[cell] = sorted(directions[(other[0] - cell[0], other[1] - cell[1])] for other in adjacent)
    # Native ground draws far-to-near rows and left-to-right columns. Adjacent
    # authored halves overlap at exact shared edge pixels, so compare their
    # original composition, never a masked/tolerated isolated half at a seam.
    for y in (23, 24, 25):
        piece = Image.new("P", (64, 31), 0)
        if scene != 2:
            for _, _, cx, cy in sorted((cx + cy, cy - cx, cx, cy) for cx, cy in cells):
                ports = cells[cx, cy]
                for port in ports:
                    row = next(row for row in manifest["sprites"] if (row["state"], row["direction"]) ==
                               ("powered" if scene == 1 else "unpowered", ("NE", "SE", "SW", "NW")[port]))
                    x, top, width, height = row["box"]
                    branch = atlas.crop((x, top, x + width, top + height))
                    offset = ((cy - y - cx + 27) * 32, (cy - y + cx - 27) * 16)
                    piece.paste(branch, offset, branch.point([0] + [255] * 255, mode="L"))
        points = [(offset % 64, offset // 64, palette[index]) for offset, index in enumerate(piece.get_flattened_data()) if index]
        for zoom, scale in ((2, 1), (1, 2)):
            frame = Image.open(context.images / f"tube-conduit-{y}-zoom-{zoom}-0.png").convert("RGB")
            for px, py, rgb in points:
                for dy in range(scale):
                    for dx in range(scale):
                        actual = frame.getpixel((320 + (px - 31) * scale + dx, 180 + (py - 16) * scale + dy))
                        assert actual == rgb, (scene, y, zoom, px, py, rgb, actual)
            checks.append({"y": y, "zoom": zoom, "pixels": len(points) * scale * scale})
    return {"manifest": identity(manifest_path), "atlas": identity(atlas_path), "palette": identity(palette_path), "checks": checks}


def write_scene(context, configuration, scene):
    """Write one independent ordinary original-path file through real owners.

    @param context Reserved native artifacts/runtime/logs.
    @param configuration Actual compiled native writer.
    @param scene Dark0, published1 or retired2 original external service.
    @return Native process counts, complete original file and scalar identities.
    """
    output, proof = native(context, configuration, "[tube-conduit-art-write]", f"write-{scene}", {"OTTD_TUBE_CONDUIT_SCENE": str(scene)})
    match = re.search(r"TUBE_CONDUIT_SAVE ((?:\w+=\d+ )+)path=([^\r\n]+)", output)
    assert match is not None
    fields = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+)", match[1])}
    assert fields["scene"] == scene
    proof.update(input=identity(match[2]), fields=fields)
    return proof


def raster(context, configuration, scene, writer_report):
    """Load an exact original opposite-build writer and preserve complete native owners during actual drawing.

    @param context Reserved native reader artifacts/runtime/logs.
    @param configuration Actual compiled reader.
    @param scene Exact writer scene classification.
    @param writer_report Original successful writer report, never a substituted normalized save.
    @return Exact before/after chunks, repeated whole RGB and original packed source pixel evidence.
    """
    writer_path = ROOT / writer_report
    writer = json.loads(writer_path.read_text(encoding="utf-8"))
    assert writer["focus"] == "write" and writer["scene"] == scene
    original = writer["native"]
    source = ROOT / original["input"]["path"]
    assert identity(source) == original["input"]
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md13CD.\ntube_conduit_art\n", encoding="utf-8")
    environment = {f"OTTD_TUBE_CONDUIT_{key.upper()}": str(value) for key, value in original["fields"].items()}
    environment["OTTD_TUBE_CONDUIT_INPUT"] = str(source)
    output, proof = native(context, configuration, "[.tube-conduit-art-runtime]", f"raster-{scene}", environment)
    match = re.search(r"TUBE_CONDUIT_ART scene=(\d+) before=([^\r\n]+) after=([^\r\n]+)", output)
    assert match is not None and int(match[1]) == scene
    proof.update(writer=identity(writer_path), input=identity(source), fields=original["fields"], before=identity(match[2]), after=identity(match[3]),
                 complete_chunks_exact=compare_chunks(Path(match[2]), Path(match[3]), None))
    frames = []
    # Complete repeated native frames retain every RGB byte, including scenery
    # outside the source-pixel checks. Lossless containers never normalize art.
    for y in (23, 24, 25):
        for zoom in (1, 2):
            previous = None
            for repeat in range(2):
                ppm = context.images / f"tube-conduit-{y}-zoom-{zoom}-{repeat}.ppm"
                frame = Image.open(ppm)
                assert frame.size == (640, 360) and len(set(frame.get_flattened_data())) > 8
                pixels = frame.tobytes()
                if previous is not None:
                    assert previous == pixels, (y, zoom)
                previous = pixels
                png = ppm.with_suffix(".png")
                save_native_capture(context, frame, ppm, png)
                frames.append({"y": y, "zoom": zoom, "repeat": repeat, "file": identity(png)})
    proof["frames"] = frames
    proof["packed_pixels"] = match_conduit_pixels(context, scene)
    return proof


def main():
    """Execute one bounded isolated native scope and retain its complete actual identities.

    @return None; failed/empty native selections refuse evidence.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=(*FOCUSES, "write", "raster"), required=True)
    parser.add_argument("--scene", type=int, choices=(0, 1, 2), default=0)
    parser.add_argument("--writer-report")
    args = parser.parse_args()
    context = current_run_context("tubes", "conduit-art"); context.prepare_runtime()
    if args.focus == "write":
        proof = write_scene(context, args.config, args.scene)
    elif args.focus == "raster":
        assert args.writer_report is not None
        proof = raster(context, args.config, args.scene, args.writer_report)
    else:
        _, proof = native(context, args.config, FOCUSES[args.focus], args.focus)
    report = {"_documentation": "r2mlvs17-underground-transport-tubes.md13CD; actual original paid path rendering; prepared floor/portal are boundary inputs",
              "configuration": args.config, "focus": args.focus, "scene": args.scene, "native": proof,
              "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"), "controller": identity(__file__),
              "sources": [identity(ROOT / name) for name in SOURCES], "packed_art": identity(ROOT / "media/baseset/openttd.grf")}
    path = context.output("reports", f"tube-conduit-art-{args.config}-{args.focus}-{args.scene}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
