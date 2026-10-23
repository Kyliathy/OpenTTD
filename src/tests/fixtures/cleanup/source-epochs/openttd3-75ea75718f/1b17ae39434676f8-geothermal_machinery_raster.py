"""Match actual paid native generator-campus pixels to their immutable approved source poses.

@file geothermal_machinery_raster.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BB.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The existing ordinary paid-world observer supplies real viewport frames and complete owner/RNG purity; this wrapper supplies no world state.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from PIL import Image

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from runner import current_run_context
from local_electrical_feed_checks import identity
from tube_sprite_report import compare_worlds, verify_references
from mole_useful_raster_report import count_native

RECEIPTS = {
    "funded": ("logs/geothermal/source-rebuild/20261018-native-01/funding-verification.json", "4c0f3b708a90f56ceb5cfd2111bd2bf9007cc83c14efd73f6ef073146f31032e"),
    "delivery": ("logs/geothermal/replacement-delivery/20261018-native-01/verification.json", "c69b48f271812ea2163de76a041782547fa89f250fc2d03acb7b42a69d213503"),
    "service": ("logs/geothermal/rebuilt-service/20261018-native-01/verification.json", "ce965450a66cb43b0c624bcb3e244cdcee8113c5b241e8f30a64dc17b11bd3b6"),
}
CASES = {"funded": ("funded", 3, "funded", (0, 0)), "partial": ("delivery", 6, "partial", (0, 0)),
         "blocked": ("delivery", 8, "magma-loop", (3, 3)), "ready": ("service", 0, "magma-loop", (1, 1)),
         "vent-working": ("service", 4, "vent-tap", (2, 4)), "magma-starved": ("service", 1, "magma-loop", (2, 4)),
         "magma-working": ("service", 8, "magma-loop", (3, 2))}
SOURCES = ("src/geothermal_sprite.h", "src/geothermal_sprite.cpp", "src/geothermal_generator.cpp", "src/multilayer_sprite.cpp", "src/industry_cmd.cpp",
           "src/tests/geothermal_window_raster.cpp", "src/tests/native_art_render_fixture.h", "src/tests/multilayer/geothermal_machinery_raster.py")


def original(configuration, case):
    """Resolve only the sealed opposite writer's actual funded/transported/rebuilt original.

    @param configuration Actual current reader configuration.
    @param case Exact named paid construction/port/conversion boundary.
    @return Opaque receipt/report/checkpoint identities plus the matching existing native observer scene.
    """
    family, stage, scene, _ = CASES[case]; name, sha = RECEIPTS[family]; receipt = ROOT / name
    assert identity(receipt)["sha256"] == sha; history = json.loads(receipt.read_text(encoding="utf-8"))
    record = next(row for row in history["records"] if row["configuration"] != configuration)
    if family == "funded":
        point = next(point for point in record["checkpoints"] if point["stage"] == stage)
    else:
        point = next(point for point in record["checkpoints"] if point["scene"] == stage and point["step"] == 0)
    assert identity(ROOT / point["input"]["path"]) == point["input"]
    return {"receipt": identity(receipt), "report": record["report"], "input": point["input"]}, scene


def match_contour(record, expected_state):
    """Require exact above-ground source contours at their actual native world projection, without pixel searching or tolerances.

    @param record Actual native viewport/model/cell/sprite observations emitted after ordinary paid-world loading.
    @param expected_state Approved source state required by the independently selected actual native scenario.
    @return Hash-bound native/source/palette identities and exact contour pixel counts; legitimately overlapping same-building parts retain only their original colours.
    """
    public_path = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"; public = json.loads(public_path.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == record["asset_id"])
    # Generator authoring exports name their native atlas explicitly; the
    # fixed-view riser publishes the same approved atlas contract directly
    # through source.manifest. Both paths come from the sole public family.
    source_name = family["authoring"]["atlas_manifest"] if "authoring" in family else family["source"]["manifest"]
    source_path = ROOT / source_name; source = json.loads(source_path.read_text(encoding="utf-8")); atlas_path = ROOT / source["path"]
    if "sha256" in source:
        expected_atlas_hash = source["sha256"]
    else:
        # The riser's approved source row owns the indexed atlas hash; its
        # manifest instead retains exact per-pose original/component ancestry.
        assert family["source"]["path"] == source["path"]
        expected_atlas_hash = family["source"]["sha256"]
    assert identity(atlas_path)["sha256"] == expected_atlas_hash; atlas = Image.open(atlas_path)
    frame_path = Path(record["path"]).with_suffix(".png"); frame = Image.open(frame_path).convert("RGB"); assert frame.size == (640, 360)
    palette_path = ROOT / "src/table/palettes.h"; raw = palette_path.read_text(encoding="utf-8").split("static const Palette _palette = {")[1].split("\n};")[0]
    palette = [(0, 0, 0)] + [tuple(map(int, rgb)) for rgb in re.findall(r"M\(\s*(\d+),\s*(\d+),\s*(\d+)\)", raw)]; assert len(palette) == 256
    assert record["zoom"] in (1, 2) and len(record["cells"]) == family["tile_variants"]; scale = 1 if record["zoom"] == 2 else 2
    allowed, upper, native_bases = {}, {}, set()
    # The approved solid riser contract puts its equipment in the back and
    # leaves the front empty. Its above-ground back contour is the same
    # original hardware witness as the generators' above-ground front;
    # neighbouring real portal roofs may legitimately cover floor edges.
    contour_part = "back" if family["asset_id"] == "facility.utility-riser" else "front"
    # Every original world cell and both parts retain the existing
    # native range. Each projected point admits only genuine same-building
    # source colours; no painted mask or nearby pixel can rescue a mismatch.
    for cell in record["cells"]:
        assert cell["state"] == expected_state and 0 <= cell["frame"] < family["frames"] and 0 <= cell["view"] < len(family["directions"])
        assert 0 <= cell["cell"] < family["tile_variants"]
        for part_index, part in enumerate(("back", "front")):
            key = (family["states"][cell["state"]], cell["frame"], family["directions"][cell["view"]], cell["cell"], part)
            pose = next(row for row in source["sprites"] if (row["state"], row["frame"], row["direction"], row["tile_variant"], row["part"]) == key)
            ordinal = cell["state"] * family["frames"] + cell["frame"]
            ordinal = (ordinal * len(family["directions"]) + cell["view"]) * family["tile_variants"] + cell["cell"]
            ordinal = ordinal * 2 + part_index
            native_bases.add(cell[part] - family["packed"]["native_offset"] - ordinal)
            x, y, width, height = pose["box"]; patch = atlas.crop((x, y, x + width, y + height)); anchor = pose["anchor"]
            left = 320 + int(((cell["y"] - record["centre_y"]) - (cell["x"] - record["centre_x"])) * 32 + anchor[0]) * scale
            top = 180 + int(((cell["x"] - record["centre_x"] - .5) + (cell["y"] - record["centre_y"] - .5)) * 16 - (cell["z"] - record["centre_z"]) + anchor[1]) * scale
            # Original opaque indexed pixels project through the engine's
            # fixed2:1 world geometry. Expand exact native pixels for2x, never
            # resize/search the native frame or normalize its palette.
            for index, colour in enumerate(patch.get_flattened_data()):
                if colour == 0:
                    continue
                px, py = index % width, index // width
                for dy in range(scale):
                    for dx in range(scale):
                        point = left + px * scale + dx, top + py * scale + dy
                        assert 0 <= point[0] < 640 and 0 <= point[1] < 360, (record["asset_id"], point, "clipped source campus")
                        allowed.setdefault(point, set()).add(palette[colour])
                        if part == contour_part and py + anchor[1] < 0 and (point[0] not in upper or point[1] < upper[point[0]][1]):
                            upper[point[0]] = point  # The top exposed machinery contour cannot be hidden by a same-building lower cell.
    assert len(native_bases) == 1
    if len(upper) < 20 * scale:
        # An initial native construction stage can have only foundation art.
        # Its actual source floor contour, not invented roof points, is then
        # the appropriate exact original-pixel witness.
        for point in allowed:
            if point[0] not in upper or point[1] < upper[point[0]][1]:
                upper[point[0]] = point
    assert len(upper) >= 20 * scale
    for point in upper.values():
        assert frame.getpixel(point) in allowed[point], (record["asset_id"], record["zoom"], point, frame.getpixel(point), allowed[point])
    return {"asset_id": record["asset_id"], "state": expected_state, "zoom": record["zoom"], "original_exact_contour_pixels": len(upper),
            "source_manifest": identity(source_path), "source_atlas": identity(atlas_path), "frame": identity(frame_path), "palette": identity(palette_path), "native_base": native_bases.pop(),
            "original_contour_points": [list(upper[column]) for column in sorted(upper)]}


def capture(configuration, case):
    """Run the existing real ordinary-world raster observer on an exact paid source and verify both full campuses.

    @param configuration Current actual native test executable configuration.
    @param case Exact accepted original construction/connection/service boundary; no builder or simulation advance is permitted.
    @return None; print current evidence only after exact native pixels, full saved owners, clocks, RNG and source identities pass.
    """
    selected, scene = original(configuration, case); context = current_run_context("graphics", "geothermal-machinery-raster")
    script = ROOT / "src/tests/multilayer/utilities/geothermal_window_raster_checks.py"
    process = subprocess.run([sys.executable, str(script), "--config", configuration, "--scene", scene, "--input", str(ROOT / selected["input"]["path"])], cwd=ROOT,
                             env=dict(os.environ, OTTD_GEO_MACHINERY_CAPTURE="1"), capture_output=True, timeout=110, check=False)
    text = (process.stdout + process.stderr).decode("utf-8", errors="replace"); log = context.logs / f"{configuration}-{case}-controller.log"; log.write_text(text, encoding="utf-8")
    assert process.returncode == 0, log
    path = Path(process.stdout.decode("utf-8").splitlines()[-1]); native = json.loads(path.read_text(encoding="utf-8")); assert native["input"] == selected["input"]
    assert native["all_complete_native_chunks_equal"] and compare_worlds(native["before"], native["after"]) == 126
    stdout = (ROOT / native["log"]["path"]).read_text(encoding="utf-8"); frames = [json.loads(line) for line in re.findall(r"^GEO_MACHINERY_FRAME (.+)$", stdout, re.M)]
    assert len(frames) == 4 and {(row["asset_id"], row["zoom"]) for row in frames} == {(asset, zoom) for asset in ("facility.vent-powerhouse", "facility.magma-powerhouse") for zoom in (1, 2)}
    contours = []
    for frame in frames:  # Each exact approved model must be present; a similar old provisional power-station tile cannot pass the source contour witness.
        model = 0 if frame["asset_id"] == "facility.vent-powerhouse" else 1
        contours.append(match_contour(frame, CASES[case][3][model]))
    assert identity(ROOT / selected["input"]["path"]) == selected["input"]
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84BB.1; actual original paid generator-campus pixels and full native purity, not whole84BB.3/84C acceptance",
              "configuration": configuration, "case": case, "original": selected, "controller": identity(__file__), "native_report": identity(path), "native": native,
              "contours": contours, "campuses": frames, "sources": [identity(ROOT / name) for name in SOURCES], "controller_log": identity(log),
              "remaining": "Native riser art84BB.2/full machinery transparency/layer/picking84BB.3 and selected gallery/publication84C/84D remain pending."}
    output = context.output("reports", f"generator-campus-{configuration}-{case}.json")
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(output, flush=True)


def verify_art_references(value, cache):
    """Verify exact identities while retaining native frame dimensions and producer metadata.

    @param value Native/report subtree; frame identities additionally describe their original PPM and dimensions.
    @param cache Current exact file identities already verified during this acceptance invocation.
    @return None; extra descriptive metadata never replaces validation of the underlying file or its nested producer.
    """
    if isinstance(value, dict):
        if {"path", "bytes", "sha256"} <= value.keys():
            verify_references({key: value[key] for key in ("path", "bytes", "sha256")}, cache)
        # Native frame records extend their file identity with the actual PPM
        # producer and dimensions. Verify those nested owners independently;
        # comparing the whole annotated record to a bare identity is invalid.
        for nested in value.values():
            verify_art_references(nested, cache)
    elif isinstance(value, list):
        for nested in value:  # Every actual captured frame and owner remains mandatory, including shared references.
            verify_art_references(nested, cache)


def accept(focused_reports, raster_reports, build_logs, output):
    """Accept both current native selector bursts and every actual original paid-campus state at both zooms.

    @param focused_reports Current Debug/optimized isolated generator selector/ownership reports.
    @param raster_reports Fourteen current original-campus reports covering every named paid state in both configurations.
    @param build_logs Successful current Debug/optimized test target diagnostics that include the actual campus observer.
    @param output Absent immutable generator-only acceptance receipt below repository logs.
    @return None; any stale native/source/pixel/owner/input reference or incomplete family/state coverage prevents84BB.1 closure.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    cache, focused, rasters = {}, {}, {}; summary = {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    for name in focused_reports:  # Actual current code/binaries are checked freshly; sealed prior paid producer receipts stay opaque identities.
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, cache)
        cfg = report["configuration"]; assert cfg in summary and cfg not in focused and report["part"] == "generators"
        for proof in report["native"]:
            count_native(proof, summary[cfg])
        focused[cfg] = {"report": identity(path), **report}
    assert focused.keys() == summary.keys()
    for name in raster_reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, cache)
        cfg, case = report["configuration"], report["case"]; key = cfg, case; assert cfg in summary and case in CASES and key not in rasters
        selected, _ = original(cfg, case); assert report["original"] == selected
        native = report["native"]; assert native["configuration"] == cfg and native["binary"] == focused[cfg]["binary"] and native["case_runs"] == 1
        assert compare_worlds(native["before"], native["after"]) == 126
        for campus in report["campuses"]:
            model = 0 if campus["asset_id"] == "facility.vent-powerhouse" else 1
            match_contour(campus, CASES[case][3][model])
        summary[cfg]["cases"] += native["case_runs"]; summary[cfg]["assertions"] += native["assertions"]
        rasters[key] = {"report": identity(path), **report}
    assert rasters.keys() == {(cfg, case) for cfg in summary for case in CASES} and summary["Debug"] == summary["RelWithDebInfo"]
    pairs, pixels = [], 0
    # Corresponding actual whole paid worlds and full campus RGB frames are
    # exact across builds; authored contours are additional source witnesses,
    # not a mask normalizing a different renderer or missing model.
    for case in CASES:
        left, right = rasters["Debug", case], rasters["RelWithDebInfo", case]
        for stage in ("before", "after"):
            assert compare_worlds(left["native"][stage], right["native"][stage]) == 126
        first = {(row["asset_id"], row["zoom"]): row for row in left["contours"]}; second = {(row["asset_id"], row["zoom"]): row for row in right["contours"]}; assert first.keys() == second.keys()
        for key in first:
            a, b = first[key], second[key]; assert a["original_exact_contour_pixels"] == b["original_exact_contour_pixels"]
            with Image.open(ROOT / a["frame"]["path"]) as image_a, Image.open(ROOT / b["frame"]["path"]) as image_b:
                assert image_a.size == image_b.size and image_a.convert("RGB").tobytes() == image_b.convert("RGB").tobytes()
            pixels += a["original_exact_contour_pixels"] + b["original_exact_contour_pixels"]
            pairs.append({"case": case, "asset_id": key[0], "zoom": key[1], "left": a["frame"], "right": b["frame"], "complete_frame_rgb_exact": True})
    builds = []
    for name in build_logs:  # The exact native targets compiled the actual world selector and optional paid-campus observer.
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "warning C" not in log and "openttd_test.exe" in log; builds.append(identity(path))
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84BB.1; selected actual generator binding and original paid-campus proof, no whole machinery/gallery promotion",
              "controller": identity(__file__), "focused": list(focused.values()), "rasters": list(rasters.values()), "summary": summary, "builds": builds,
              "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
              "complete_cross_reader_world_pairs": len(CASES) * 2, "complete_campus_frame_pairs": pairs, "exact_original_contour_pixels": pixels,
              "exact_identities_verified": len(cache), "native_version": 434,
              "remaining": "84BB.2 native risers/84BB.3 whole machinery transparency/occlusion/layer/picking/four-target checks,84BC conduits,84BD controls,84B.2/84C/84D selected runtime publication; full84.3/human/network/operator gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "full_frame_pairs": len(pairs), "exact_original_contour_pixels": pixels}), flush=True)


def main():
    """Select one bounded current paid-world generator-campus raster check.

    @return None; missing exact configuration/case or any native original/pixel failure stops before a report.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--phase", choices=("capture", "accept"), default="capture")
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo")); parser.add_argument("--case", choices=tuple(CASES))
    parser.add_argument("--focused-reports", nargs=2); parser.add_argument("--raster-reports", nargs="+"); parser.add_argument("--build-logs", nargs=2); parser.add_argument("--output"); args = parser.parse_args()
    if args.phase == "capture":
        assert args.config is not None and args.case is not None; capture(args.config, args.case)
    else:
        assert args.focused_reports is not None and args.raster_reports is not None and args.build_logs is not None and args.output is not None
        accept(args.focused_reports, args.raster_reports, args.build_logs, args.output)


if __name__ == "__main__":
    main()
