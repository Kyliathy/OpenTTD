"""Verify actual native purchase-preview pixels and unchanged original paid-world owners.

@file selected_purchase_raster.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups84CB/84C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Original saved company/date availability supplies preview policy; no purchase transaction or complete purchase window is claimed.
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
from geothermal_machinery_raster import ROOT, identity, original, verify_art_references
from runner import current_run_context
from tube_sprite_report import compare_worlds

sys.path.insert(0, str(ROOT / "media/multilayer"))
import pack_m1_art as packing

SOURCES = ("src/tests/selected_purchase_view_fixture.h", "src/tests/geothermal_window_raster.cpp", "src/tests/native_art_render_fixture.h",
           "src/tests/forge_useful_raster_fixture.h", "src/tests/multilayer/forge_useful_raster.py",
           "src/train_cmd.cpp", "src/transport_tube_vehicle_sprite.cpp", "src/multilayer_sprite.cpp", "src/transport_tube_engine.cpp",
           "src/tests/CMakeLists.txt", "src/tests/multilayer/selected_purchase_raster.py")


def inspect(native):
    """Match every authored opaque preview pixel using the native sprite-reported position.

    @param native Exact current ordinary observer report with renderer frames, complete owner saves and native application log.
    @return Native model/policy records, exact frame/source witnesses and complete repeated-frame pairs.
    """
    text = (ROOT / native["log"]["path"]).read_text(encoding="utf-8")
    records = [json.loads(line) for line in re.findall(r"^SELECTED_PURCHASE_FRAME (.+)$", text, re.M)]
    assert len(records) == 2 and {row["mode"] for row in records} == {"owner", "spectator"}
    public = json.loads(packing.PUBLIC.read_text(encoding="utf-8")); families = {row["asset_id"]: row for row in public["assets"]}
    palette_path = ROOT / "src/table/palettes.h"
    palette_text = palette_path.read_text().split("static const Palette _palette = {")[1].split("\n};")[0]
    palette = [(0, 0, 0)] + [tuple(map(int, rgb)) for rgb in re.findall(r"M\(\s*(\d+),\s*(\d+),\s*(\d+)\)", palette_text)]
    assert len(palette) == 256
    witnesses, repetitions, coverage = [], [], set()
    frames = {row["path"]: row for row in native["frames"]}
    for record in records:  # Original owner and spectator previews must independently retain the actual five-model engine mapping and complete unmodified canvases.
        assert len(record["models"]) == 5 and {row["model"] for row in record["models"]} == set(range(9, 14))
        paths = [Path(path).with_suffix(".png") for path in record["paths"]]
        assert len(paths) == 2
        images = []
        for path in paths:  # Each native repetition is independently hash-selected by the ordinary producer; an undeclared lookalike is not a preview capture.
            assert path.relative_to(ROOT).as_posix() in frames
            image = Image.open(path).convert("RGB"); assert image.size == (640, 80); images.append(image)
        assert images[0].tobytes() == images[1].tobytes(), "native purchase repetition changed"
        repetitions.append({"first": identity(paths[0]), "second": identity(paths[1]), "complete_rgb_exact": True})
        for model in record["models"]:  # Every model/state binds its original source atlas and public native range; no generic image or ordinal model cast can supply pixels.
            asset = f"purchase.model-{model['model']:02}"; assert model["asset_id"] == asset
            state = model["state"]; assert state == (0 if model["available"] else 1)
            if record["mode"] == "spectator":
                assert not model["available"], "a spectator cannot acquire native purchase permission"
            family = families[asset]; assert model["sprite"] - family["packed"]["native_offset"] - state >= 0
            coverage.add((asset, state))
            manifest_path = ROOT / packing.source_binding(family)["atlas_manifest"]
            manifest = json.loads(manifest_path.read_text(encoding="utf-8")); atlas = next(row for row in manifest["atlases"] if row["asset_id"] == asset)
            atlas_path = ROOT / atlas["path"]; assert identity(atlas_path)["sha256"] == atlas["sha256"]
            pose = next(row for row in atlas["sprites"] if row["state"] == ("available" if model["available"] else "locked"))
            x, y, width, height = pose["box"]; piece = Image.open(atlas_path).crop((x, y, x + width, y + height))
            # The native decoder trims transparent margins and folds them into
            # sprite offsets. Source pixels retain the full authored canvas,
            # so project its own anchor from the actual draw point exactly once.
            left, top = [origin + anchor for origin, anchor in zip(model["draw_anchor"], pose["anchor"])]
            assert left >= 0 and top >= 0 and left + width <= 640 and top + height <= 80
            exact = 0
            for offset, colour in enumerate(piece.get_flattened_data()):  # All original opaque source points must survive actual GetRailIcon drawing; no colour tolerance, search, crop or painted mask is permitted.
                if colour == 0:
                    continue
                point = (left + offset % width, top + offset // width)
                assert images[0].getpixel(point) == palette[colour], (asset, state, point, images[0].getpixel(point), palette[colour])
                exact += 1
            assert exact > 20
            witnesses.append({"asset_id": asset, "state": state, "model": model["model"], "engine": model["engine"], "sprite": model["sprite"],
                              "frame": identity(paths[0]), "source_origin": [left, top], "native_trimmed_origin": model["native_trimmed_origin"], "original_exact_pixels": exact,
                              "source_manifest": identity(manifest_path), "source_atlas": identity(atlas_path), "native_offset": family["packed"]["native_offset"], "sprite_count": family["packed"]["sprite_count"]})
    assert {asset for asset, state in coverage} == {f"purchase.model-{model:02}" for model in range(9, 14)}
    bases = {row["sprite"] - row["native_offset"] - row["state"] for row in witnesses}
    assert len(bases) == 1, "native purchase previews disagree on the loaded public sprite base"
    return {"records": records, "source_witnesses": witnesses, "complete_repeat_frame_pairs": repetitions, "public_manifest": identity(packing.PUBLIC), "palette": identity(palette_path)}


def capture(configuration, world, native_report=None):
    """Observe previews in one exact opposite-writer original paid world without changing it.

    @param configuration Actual current Debug or optimized native reader configuration.
    @param world Named original first-plant or useful Forge world; native saved availability is never altered to create a desired preview state.
    @param native_report Optional exact current same-binary native report; no latest-run discovery occurs.
    @return None; retain only successful current native/pixel/source/complete-world proof.
    """
    context = current_run_context("graphics", "selected-purchase-raster"); controller_log = None
    if world == "plant":
        selected, scene = original(configuration, "ready")
        script = ROOT / "src/tests/multilayer/utilities/geothermal_window_raster_checks.py"
        command = [sys.executable, str(script), "--config", configuration, "--scene", scene, "--input", str(ROOT / selected["input"]["path"])]
    else:
        import forge_useful_raster as forge
        receipt = ROOT / forge.FILE_RECEIPT
        assert identity(receipt)["sha256"] == "8b475d2812d10529bba306cfd4bf9b9cf31572eb5f885f180de3cda10e8a775b"
        history = json.loads(receipt.read_text(encoding="utf-8"))
        writer = next(row["report"] for row in history["records"] if row.get("phase") == "write" and row["configuration"] != configuration)
        assert identity(ROOT / writer["path"]) == writer
        points = json.loads((ROOT / writer["path"]).read_text(encoding="utf-8"))["checkpoints"]
        point = next(row for row in points if (row["scene"], row["step"]) == (2, 0))
        selected = {"receipt": identity(receipt), "writer": writer, "input": point["input"]}
        command = [sys.executable, str(ROOT / "src/tests/multilayer/forge_useful_raster.py"), "--config", configuration, "--writer-report", str(ROOT / writer["path"])]
    if native_report is None:
        process = subprocess.run(command, cwd=ROOT,
                                 env=dict(os.environ, OTTD_SELECTED_PURCHASE_CAPTURE="1"), capture_output=True, timeout=110, check=False)
        log = context.logs / f"{configuration}-controller.log"; log.write_bytes(process.stdout + process.stderr); assert process.returncode == 0, log
        path = Path(process.stdout.decode("utf-8").splitlines()[-1]); controller_log = identity(log)
    else:
        path = ROOT / native_report
    native = json.loads(path.read_text(encoding="utf-8")); verify_art_references(native, {})
    assert native["configuration"] == configuration and native["binary"] == identity(ROOT / f"build/{configuration}/openttd_test.exe")
    if world == "plant":
        assert native["input"] == selected["input"] and native["case_runs"] == 1 and native["all_complete_native_chunks_equal"]
        assert compare_worlds(native["before"], native["after"]) == 126
        proof = inspect(native)
    else:
        assert native["checkpoint"]["input"] == selected["input"] and native["view"]["complete_chunks_exact"] == 126
        assert compare_worlds(native["view"]["before"], native["view"]["after"]) == 126
        proof = inspect({"log": native["native"]["log"], "frames": native["purchase_frames"]})
        assert any(row["asset_id"] == "purchase.model-13" and row["state"] == 0 for row in proof["source_witnesses"]), "actual useful Forge world must supply its available purchase preview"
    assert identity(ROOT / selected["input"]["path"]) == selected["input"]
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84CB; actual native DrawTrainEngine/GetRailIcon preview canvases,not purchase transactions/windows",
              "configuration": configuration, "world": world, "original": selected, "native_report": identity(path), "native": native, **proof,
              "controller": identity(__file__), "controller_log": controller_log, "sources": [identity(ROOT / name) for name in SOURCES],
              "remaining": "Both-reader focused/current purchase acceptance,84CC/84CD then84C gallery/84D publication;whole master/human/operator/network gates remain pending."}
    output = context.output("reports", f"selected-purchase-raster-{configuration}-{world}.json"); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(output, flush=True)


def accept(reports, build_logs, output):
    """Seal both current native readers, all original purchase poses and complete paid-world purity.

    @param reports Four exact current reports covering first-plant and useful Forge originals in both configurations.
    @param build_logs Successful current Debug/optimized test-target build logs; production files are unchanged by this test-only stage.
    @param output Absent immutable acceptance receipt beneath repository logs.
    @return None; current sources/binaries, complete worlds, repeated/cross-reader RGB and every model/state must agree before closure.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    grid, cache, counts = {}, {}, {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    for name in reports:  # Every named original/reader must supply current source and native proof; a similarly named older capture cannot fill a missing model or policy state.
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, cache)
        cfg, world = report["configuration"], report["world"]; key = (cfg, world)
        assert cfg in counts and world in ("plant", "forge") and key not in grid
        assert report["controller"] == identity(__file__) and report["sources"] == [identity(ROOT / name) for name in SOURCES]
        native = report["native"]; assert native["binary"] == identity(ROOT / f"build/{cfg}/openttd_test.exe")
        if world == "plant":
            before, after = native["before"], native["after"]; observed = native
        else:
            before, after = native["view"]["before"], native["view"]["after"]
            observed = {"log": native["native"]["log"], "frames": native["purchase_frames"]}
            assert native["native_steps"] > 0 and native["consumed_j"] > 0, "Forge purchase policy must come from the actual useful moving world"
        assert compare_worlds(before, after) == 126
        proof = inspect(observed)
        for field, value in proof.items():  # Fresh exact source projection and native repetition must reproduce the reported witness; altered cached metadata never substitutes for actual pixels.
            assert report[field] == value, "purchase witness metadata differs from actual native evidence: " + field
        raw = (ROOT / observed["log"]["path"]).read_text(encoding="utf-8")
        match = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", raw)
        assert match is not None and "NATIVE_EXIT_CODE=0" in raw
        counts[cfg]["assertions"] += int(match[1]); counts[cfg]["cases"] += int(match[2])
        grid[key] = {"report": identity(path), **report}
    assert grid.keys() == {(cfg, world) for cfg in counts for world in ("plant", "forge")}
    pairs = []
    for world in ("plant", "forge"):  # Independent current readers must preserve the same complete native owner boundary and all renderer pixels on each retained original world.
        left, right = grid["Debug", world], grid["RelWithDebInfo", world]
        a, b = left["native"], right["native"]
        if world == "forge":
            a, b = a["view"], b["view"]
        assert compare_worlds(a["before"], b["before"]) == compare_worlds(a["after"], b["after"]) == 126
        for first, second in zip(left["complete_repeat_frame_pairs"], right["complete_repeat_frame_pairs"]):  # Both real owner and spectator policy must match as complete canvases, including transparent background and all five models.
            a, b = Image.open(ROOT / first["first"]["path"]).convert("RGB"), Image.open(ROOT / second["first"]["path"]).convert("RGB")
            assert a.size == b.size and a.tobytes() == b.tobytes(), "current native purchase readers disagree"
            pairs.append({"world": world, "left": first["first"], "right": second["first"], "complete_rgb_exact": True})
    for cfg in counts:  # The actual older plant and later useful Forge worlds jointly cover all ten available/locked poses without editing availability, dates or the saved company.
        coverage = {(row["asset_id"], row["state"]) for world in ("plant", "forge") for row in grid[cfg, world]["source_witnesses"]}
        assert coverage == {(f"purchase.model-{model:02}", state) for model in range(9, 14) for state in (0, 1)}
    builds = []
    for name in build_logs:  # Both test targets compile the optional shared capture helper; new compiler/linker warnings or failed target diagnostics cannot be hidden by green raster logs.
        path = ROOT / name; raw = path.read_text(encoding="utf-8", errors="replace")
        assert "openttd_test.vcxproj ->" in raw and not re.search(r"(?:warning|error) [A-Z]+\d+", raw)
        builds.append(identity(path))
    assert len(builds) == 2 and len({row["path"] for row in builds}) == 2
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84CB; actual native five-model purchase preview pixels,original paid/useful world availability and full purity;not transactions/windows",
              "controller": identity(__file__), "reports": list(grid.values()), "builds": builds, "summary": counts,
              "native_cases": sum(row["cases"] for row in counts.values()), "native_assertions": sum(row["assertions"] for row in counts.values()),
              "complete_cross_reader_frame_pairs": pairs, "complete_cross_reader_world_pairs": 4,
              "complete_repeat_frame_pairs": sum(len(row["complete_repeat_frame_pairs"]) for row in grid.values()),
              "exact_original_purchase_pixels": sum(witness["original_exact_pixels"] for row in grid.values() for witness in row["source_witnesses"]),
              "binaries": {cfg: identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in counts}, "packed_grf": identity(ROOT / "media/baseset/openttd.grf"),
              "native_version": 434, "remaining": "84CC current tube scenes,84CD current paid geothermal scenes,return84C.1/84C.2 aggregate/gallery and84D publication;whole master/human/operator/network gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "cases": result["native_cases"], "assertions": result["native_assertions"], "exact_original_pixels": result["exact_original_purchase_pixels"]}), flush=True)


def main():
    """Select explicit current observation, same-binary recheck or exact both-reader closure.

    @return None; original paid inputs and immutable source pixels remain unchanged.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--phase", choices=("capture", "accept"), default="capture")
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo")); parser.add_argument("--world", choices=("plant", "forge"))
    parser.add_argument("--native-report"); parser.add_argument("--reports", nargs=4); parser.add_argument("--build-logs", nargs=2); parser.add_argument("--output"); args = parser.parse_args()
    if args.phase == "capture":
        assert args.config and args.world; capture(args.config, args.world, args.native_report)
    else:
        assert args.reports and args.build_logs and args.output; accept(args.reports, args.build_logs, args.output)


if __name__ == "__main__":
    main()
