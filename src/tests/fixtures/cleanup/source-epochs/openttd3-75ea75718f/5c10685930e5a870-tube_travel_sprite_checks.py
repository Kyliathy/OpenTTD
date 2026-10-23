"""Run genuine paid capsule directional travel, exact native pixels and original layer picking.

@file tube_travel_sprite_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups13DB/13DCB/15EDE.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
"""
import argparse
import json
from pathlib import Path
import re
import sys

from PIL import Image, ImageFilter

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from runner import current_run_context, save_native_capture
from local_electrical_feed_checks import identity, native
from geothermal_tube_work_files import compare_chunks
from tube_conduit_sprite_checks import SOURCES as CONDUIT_SOURCES
from tube_capsule_sprite_checks import SOURCES as CAPSULE_SOURCES

SOURCES = tuple(dict.fromkeys((*CONDUIT_SOURCES, *CAPSULE_SOURCES, "src/tests/transport_tube_travel_sprite_load.cpp",
                             "src/tests/transport_tube_sprite.cpp", "src/vehicle.cpp", "src/vehicle_gui.h", "src/track_func.h", "src/train_cmd.cpp")))
MODELS = {9: "mole", 10: "crate", 11: "cistern", 12: "shield", 13: "forge"}


def match_travel_pixels(context, model, view):
    """Account for every original hull pixel at the independently projected real body anchor.

    @param context Actual native output owner.
    @param model Original native capability model9..13.
    @param view Actual earned native view/frame and projected physical anchor; optional native state0..8 selects the observed load/service pose, while older partial-load fixtures retain state1.
    @return Original palette/atlas identities and complete hull/foreground pixel counts at both zooms.
    """
    manifest_path = ROOT / f"logs/proofs/mlm/native-art/ground/{MODELS[model]}-source-manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8")); atlas = manifest["atlases"][0]
    atlas_path = ROOT / atlas["path"]; assert identity(atlas_path)["sha256"] == atlas["sha256"]
    # Existing directional fixtures carry a partial load. A useful route may
    # genuinely fill its native capacity, so its observed state must select the
    # original full-load pose instead of testing unrelated partial-load pixels.
    states = ("empty", "partial", "full", "service-empty", "service-partial", "service-full", "blocked-empty", "blocked-partial", "blocked-full")
    state = view.get("state", 1)
    assert state in range(len(states))
    row = next(row for row in atlas["sprites"] if (row["state"], row["direction"], row["frame"]) ==
               (states[state], ("N", "NE", "E", "SE", "S", "SW", "W", "NW")[view["view"]], view["frame"]))
    x, y, width, height = row["box"]; piece = Image.open(atlas_path).crop((x, y, x + width, y + height))
    palette_path = ROOT / "src/table/palettes.h"
    palette_text = palette_path.read_text().split("static const Palette _palette = {")[1].split("\n};")[0]
    palette = [(0, 0, 0)] + [tuple(map(int, rgb)) for rgb in re.findall(r"M\(\s*(\d+),\s*(\d+),\s*(\d+)\)", palette_text)]
    assert len(palette) == 256
    # Every non-company hull pixel must either remain exact or be covered by
    # an exact original foreground pixel from an actual neighbouring native
    # guideway. No fixed straight-track window or image-derived mask can hide
    # a missing/shifted hull at a curve. Whole repeated frames retain company
    # remaps and every other pixel independently.
    interior = piece.point([0] + [255] * 255, mode="L").filter(ImageFilter.MinFilter(3))
    points = [(offset % width, offset // width, palette[index], bool(interior.getpixel((offset % width, offset // width))))
              for offset, index in enumerate(piece.get_flattened_data())
              if index and not 198 <= index <= 205]
    assert len(points) > 0
    interior_points = sum(point[3] for point in points)
    assert interior_points > 0
    foregrounds, providers = [], {}
    for rim in view["rims"]:
        if rim["mask"] in (1, 2):
            name, direction = "tube-atlas-manifest.json", "NE" if rim["mask"] == 1 else "SE"
        elif rim["mask"] in (4, 8, 16, 32):
            name, direction = "tube-curves-atlas-manifest.json", {4: "N", 8: "S", 16: "W", 32: "E"}[rim["mask"]]
        else:
            name, direction = ("tube-crossing-atlas-manifest.json" if rim["mask"] == 3 else "tube-junction-atlas-manifest.json"), "none"
        provider_path = ROOT / "logs/proofs/mlm/native-art/ground" / name
        provider = json.loads(provider_path.read_text(encoding="utf-8"))
        native_atlas = ROOT / provider["path"]
        assert identity(native_atlas)["sha256"] == provider["sha256"]
        providers[name] = {"manifest": identity(provider_path), "atlas": identity(native_atlas)}
        # Shared-edge atlas variants retain the complete actual native mask;
        # a source view alone must not silently pick a different junction.
        front = next(row for row in provider["sprites"] if (row["state"], row["direction"], row["part"]) ==
                     ("powered" if rim["energized"] else "unpowered", direction, "front") and
                     row.get("native_track_bits", row.get("trackbits", rim["mask"])) == rim["mask"])
        fx, fy, fw, fh = front["box"]
        foregrounds.append((rim, Image.open(native_atlas).crop((fx, fy, fx + fw, fy + fh))))
    matches = []
    for zoom, scale in ((2, 1), (1, 2)):
        frame_path = context.images / f"tube-travel-{view['view']}-zoom-{zoom}-0.png"
        frame = Image.open(frame_path).convert("RGB")
        visible, occluded, interior_visible = 0, 0, 0
        for px, py, rgb, core in points:
            rx, ry = view["dx"] + row["anchor"][0] + px, view["dy"] + row["anchor"][1] + py
            possible = []
            # Only source-alpha foreground pixels at this exact projected
            # coordinate may explain occlusion. Ground backs, arbitrary grey
            # scenery and nearby but nonoverlapping rim pixels cannot pass.
            for rim, front_piece in foregrounds:
                fx, fy = rx - rim["dx"] + 31, ry - rim["dy"] + 16
                if 0 <= fx < front_piece.width and 0 <= fy < front_piece.height:
                    index = front_piece.getpixel((fx, fy))
                    if index:
                        possible.append(palette[index])
            for dy in range(scale):
                for dx in range(scale):
                    actual = frame.getpixel((320 + rx * scale + dx, 180 + ry * scale + dy))
                    if actual == rgb:
                        visible += 1
                        interior_visible += core
                    else:
                        assert actual in possible, (model, view["view"], zoom, px, py, rgb, actual, possible)
                        occluded += 1
        # Legibility concerns the body interior, excluding only its one-pixel
        # source-alpha outline. Narrow cardinal poses can put over half their
        # outline behind real rims while retaining the readable central hull.
        # Every outline pixel remains accounted above; this source-derived
        # interior metric neither removes failed pixels nor masks a frame.
        assert interior_visible * 2 >= interior_points * scale * scale, (model, view["view"], interior_visible, interior_points)
        assert visible * 2 >= len(points) * scale * scale, (model, view["view"], visible, occluded)
        matches.append({"zoom": zoom, "source_points": len(points), "visible_pixels": visible, "occluded_by_original_rim_pixels": occluded,
                        "exact_pixels": visible + occluded, "interior_source_points": interior_points, "interior_visible_pixels": interior_visible})
    return {"manifest": identity(manifest_path), "atlas": identity(atlas_path), "palette": identity(palette_path),
            "foreground_sources": list(providers.values()), "matches": matches}



def write_scene(context, configuration, model, layout=0):
    """Write one independent real model and paid curved-route world.

    @param context Reserved native artifacts/runtime/log owner.
    @param configuration Actual writer build.
    @param model Actual command-purchased available model at the disclosed2005 calendar.
    @param layout Explicit plain0 or actual paid crossing/junction1 native fixture.
    @return Native process evidence and the original complete file/scalar identities.
    """
    output, proof = native(context, configuration, "[tube-travel-art-write]", f"write-{model}",
                           {"OTTD_TUBE_TRAVEL_MODEL": str(model), "OTTD_TUBE_TRAVEL_LAYOUT": str(layout)})
    match = re.search(r"TUBE_TRAVEL_SAVE ((?:\w+=\d+ )+)path=([^\r\n]+)", output); assert match is not None
    fields = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+)", match[1])}; assert fields["model"] == model
    proof.update(input=identity(match[2]), fields=fields)
    return proof


def raster(context, configuration, model, writer_report, layout=0):
    """Earn physical headings through native work and verify independently pure raster boundaries.

    @param context Reserved native review/runtime/log owner.
    @param configuration Actual opposite reader build.
    @param model Exact original writer's real model.
    @param writer_report Exact immutable successful writer report.
    @param layout Explicit plain0 or original paid crossing/junction1 writer geometry.
    @return Every complete boundary, real returned world, repeated RGB, original hull pixels and native picking evidence.
    """
    writer_path = ROOT / writer_report; writer = json.loads(writer_path.read_text(encoding="utf-8"))
    assert writer["focus"] == "write" and writer["model"] == model
    assert writer.get("layout", 0) == layout
    original = writer["native"]; source = ROOT / original["input"]["path"]; assert identity(source) == original["input"]
    startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md13DB.\ntube_travel_art\n", encoding="utf-8")
    environment = {f"OTTD_TUBE_TRAVEL_{key.upper()}": str(value) for key, value in original["fields"].items()}
    environment["OTTD_TUBE_TRAVEL_INPUT"] = str(source)
    environment["OTTD_TUBE_TRAVEL_LAYOUT"] = str(layout)
    output, proof = native(context, configuration, "[.tube-travel-art-runtime]", f"travel-{model}", environment)
    matches = re.findall(r"TUBE_TRAVEL_VIEW ((?:\w+=-?\d+ )+)picked=true wrong_layer_refused=true before=([^\r\n]+) after=([^\r\n]+)", output)
    assert len(matches) == 8
    views = []
    # Each heading is earned by actual train movement. Complete owner equality
    # applies across each capture; real ticks between captures remain visible
    # work and must never be normalized into a no-op simulation comparison.
    for fields_text, before, after in matches:
        view = {key: int(value) for key, value in re.findall(r"(\w+)=(-?\d+)", fields_text)}
        assert view["model"] == model and view["speed"] > 0
        view.update(before=identity(before), after=identity(after), complete_chunks_exact=compare_chunks(Path(before), Path(after), None),
                    native_picking=True, wrong_layer_refused=True)
        rim_matches = re.findall(r"TUBE_TRAVEL_RIM view=" + str(view["view"]) + r" ((?:\w+=-?\d+ ?)+)", output)
        view["rims"] = [{key: int(value) for key, value in re.findall(r"(\w+)=(-?\d+)", text)} for text in rim_matches]
        assert len(view["rims"]) > 0
        frames = []
        for zoom in (1, 2):
            previous = None
            for repeat in range(2):
                ppm = context.images / f"tube-travel-{view['view']}-zoom-{zoom}-{repeat}.ppm"
                frame = Image.open(ppm).convert("RGB"); assert frame.size == (640, 360) and len(set(frame.get_flattened_data())) > 8
                pixels = frame.tobytes()
                if previous is not None:
                    assert pixels == previous, (model, view["view"], zoom)
                previous = pixels; png = ppm.with_suffix(".png"); save_native_capture(context, frame, ppm, png)
                frames.append({"zoom": zoom, "repeat": repeat, "file": identity(png)})
        view["frames"] = frames; view["packed_pixels"] = match_travel_pixels(context, model, view); views.append(view)
    assert {view["view"] for view in views} == set(range(8))
    if layout == 1:
        crossing = next(view for view in views if view["view"] == 3)
        junction = next(view for view in views if view["view"] == 2)
        assert (crossing["mask"], crossing["track"]) == (3, 2)
        assert (junction["mask"], junction["track"]) == (5, 4)
    done = re.search(r"TUBE_TRAVEL_DONE model=(\d+) views=255 native_ticks=(\d+) start=(\d+) end=(\d+) used_j=(\d+) returned=true packet_retained=true final=([^\r\n]+)", output)
    assert done is not None and int(done[1]) == model and int(done[2]) == int(done[4]) - int(done[3]) > 0
    proof.update(writer=identity(writer_path), input=identity(source), fields=original["fields"], views=views,
                 native_ticks=int(done[2]), start_tick=int(done[3]), end_tick=int(done[4]), used_j=int(done[5]), final=identity(done[6]))
    assert proof["used_j"] > 0
    assert identity(source) == original["input"]
    return proof


def main():
    """Execute one bounded original model writer or genuine directional travel observer.

    @return None; failed/empty native cases or any identity/pixel mismatch refuse the report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=("write", "raster"), required=True)
    parser.add_argument("--model", type=int, choices=tuple(MODELS), required=True)
    parser.add_argument("--writer-report")
    parser.add_argument("--layout", choices=("plain", "joined"), default="plain")
    args = parser.parse_args(); layout = int(args.layout == "joined")
    context = current_run_context("tubes", "joins-art" if layout else "travel-art"); context.prepare_runtime()
    if args.focus == "write":
        proof = write_scene(context, args.config, args.model, layout)
    else:
        assert args.writer_report is not None
        proof = raster(context, args.config, args.model, args.writer_report, layout)
    report = {"_documentation": f"r2mlvs17-underground-transport-tubes.md{'13DCB' if layout else '13DB'}; actual2005 model commands/paid route/native travel; prepared floor and portal are boundary inputs",
              "configuration": args.config, "focus": args.focus, "model": args.model, "layout": layout, "native": proof,
              "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"), "controller": identity(__file__),
              "sources": [identity(ROOT / name) for name in SOURCES], "packed_art": identity(ROOT / "media/baseset/openttd.grf")}
    path = context.output("reports", f"tube-travel-art-{args.config}-{args.focus}-{args.model}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


if __name__ == "__main__":
    main()
