"""Verify original selected tool icons in actual paid-world windows and retain complete native purity.

@file selected_control_raster.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BD.2.
@see docs/zz-reach2/architecture/gui/archi-gui.md.
The ordinary native loader/widget renderer supplies every pixel; no window, text, sprite or infrastructure is synthesized.
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
from selected_control_checks import CASES as FOCUSED_CASES, SOURCES as SELECTOR_SOURCES
from runner import current_run_context
from tube_sprite_report import compare_worlds
from mole_useful_raster_report import count_native

CASES = ("ready", "magma-working")
SOURCES = SELECTOR_SOURCES + ("src/tests/selected_control_view_fixture.h", "src/tests/geothermal_window_raster.cpp", "src/tests/native_art_render_fixture.h",
                              "src/tests/CMakeLists.txt", "src/tests/multilayer/selected_control_raster.py")
EXISTING_WARNINGS = {("src/excavation_route.cpp", 511, "C4267"), ("src/tbm_projects_gui.cpp", 360, "C4018"),
                     ("src/tests/launch_center.cpp", 488, "C4018"), ("src/tests/launch_center.cpp", 501, "C4018"),
                     ("src/tests/launch_center.cpp", 542, "C4018"), ("src/tests/launch_center.cpp", 555, "C4018")}


def build_warnings(log):
    """Record exact existing full-rebuild warnings in untouched owners while rejecting any new or selected-control warning.

    @param log Unmodified decoded successful native build diagnostics; no warning is suppressed or removed.
    @return Exact file/line/code identities for the six pre-existing conversion/sign warnings exposed by cleanup's full recompilation.
    """
    found = {(Path(path).resolve().relative_to(ROOT).as_posix(), int(line), code) for path, line, code in re.findall(r"^(.+?)\((\d+),\d+\): warning (C\d+):", log, re.M)}
    assert found <= EXISTING_WARNINGS, sorted(found - EXISTING_WARNINGS)
    return [{"source": identity(ROOT / name), "line": line, "code": code, "existing_untouched_owner": True} for name, line, code in sorted(found)]


def inspect(native):
    """Compare every visible actual icon with its exact approved source at the native widget-reported projection.

    @param native Current ordinary paid-world native observer report, including complete unchanged saves, log and producer-bound images.
    @return Exact original-pixel witnesses, native enabled/disabled/pressed coverage, whole-frame repetitions and actual widget/layout records.
    """
    log = (ROOT / native["log"]["path"]).read_text(encoding="utf-8")
    records = [json.loads(line) for line in re.findall(r"^SELECTED_CONTROL_FRAME (.+)$", log, re.M)]
    grid = {(row["layer"], row["mode"]): row for row in records}; assert len(records) == len(grid) == 6
    assert grid.keys() == {(layer, mode) for layer in (1, 3) for mode in ("closed", "expanded", "spectator")}
    public_path = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"; public = json.loads(public_path.read_text(encoding="utf-8"))
    families = {row["asset_id"]: row for row in public["assets"] if row["asset_id"] in ("control.tube", "control.geothermal")}
    palette_path = ROOT / "src/table/palettes.h"; raw = palette_path.read_text(encoding="utf-8").split("static const Palette _palette = {")[1].split("\n};")[0]
    palette = [(0, 0, 0)] + [tuple(map(int, rgb)) for rgb in re.findall(r"M\(\s*(\d+),\s*(\d+),\s*(\d+)\)", raw)]; assert len(palette) == 256
    sources, atlases, witnesses, repetitions, coverage, bases = {}, {}, [], [], set(), set()
    for asset, family in families.items():
        source_path = ROOT / family["source"]["manifest"]; source = json.loads(source_path.read_text(encoding="utf-8")); atlas_path = ROOT / source["path"]
        assert identity(atlas_path)["sha256"] == family["source"]["sha256"] and family["states"] == ["enabled", "disabled", "pressed"] and family["native_size"] == [20, 20]
        assert family["packed"]["native_offset"] == (152 if asset == "control.tube" else 155) and family["packed"]["sprite_count"] == 3
        sources[asset] = source; atlases[asset] = Image.open(atlas_path); assert atlases[asset].mode == "P"
    for row in records:  # Repeated actual widget rendering must preserve the full image, not merely an icon crop or normalized caption/background.
        assert row["width"] <= 640 and row["height"] <= 720 and len(row["paths"]) == 2
        first, second = [Path(name).with_suffix(".png") for name in row["paths"]]
        with Image.open(first) as a, Image.open(second) as b:
            assert a.size == b.size == (row["width"], row["height"]) and a.convert("RGB").tobytes() == b.convert("RGB").tobytes(); frame = a.convert("RGB")
        repetitions.append({"layer": row["layer"], "mode": row["mode"], "first": identity(first), "second": identity(second), "complete_rgb_exact": True})
        assert len(row["widgets"]) == (2 if row["mode"] == "closed" else 4)
        for widget in row["widgets"]:
            assert widget["visible"] and widget["caption"]
            asset = widget["asset_id"]; family = families[asset]; state = 1 if widget["disabled"] else 2 if widget["pressed"] else 0
            bases.add(widget["sprite"] - family["packed"]["native_offset"] - state); coverage.add((asset, state))
            pose = next(pose for pose in sources[asset]["sprites"] if pose["state"] == family["states"][state])
            x, y, width, height = pose["box"]; patch = atlases[asset].crop((x, y, x + width, y + height)); ox, oy = widget["source_origin"]
            left, top, right, bottom = widget["widget_rect"]; caption = widget["caption_rect"]
            assert widget["caption_size"][0] <= caption[2] - caption[0] + 1 and widget["caption_size"][1] <= caption[3] - caption[1] + 1
            exact, native_checker = 0, 0
            for index, colour in enumerate(patch.get_flattened_data()):
                if colour == 0:
                    continue
                px, py = ox + index % width, oy + index // width
                assert left <= px <= right and top <= py <= bottom and 0 <= px < frame.width and 0 <= py < frame.height
                # The existing native disabled widget paints every other
                # interior pixel after its icon/caption. Fixed native checker
                # parity is UI policy, not a screenshot-derived mask. Every
                # exposed original source pixel must remain byte-exact.
                if widget["disabled"] and (px - left - 1 + py - top - 1) % 2 == 1:
                    native_checker += 1
                    continue
                assert frame.getpixel((px, py)) == palette[colour], (asset, state, row["layer"], row["mode"], (px, py), frame.getpixel((px, py)), palette[colour])
                exact += 1
            assert exact > 40 and (not widget["disabled"] or native_checker > 40)
            witnesses.append({"asset_id": asset, "state": state, "widget": widget["widget"], "layer": row["layer"], "mode": row["mode"], "frame": identity(first),
                              "original_exact_pixels": exact, "native_disabled_checker_pixels": native_checker, "source_origin": widget["source_origin"],
                              "source_atlas": identity(ROOT / sources[asset]["path"]), "source_manifest": identity(ROOT / family["source"]["manifest"])})
    assert coverage == {(asset, state) for asset in families for state in range(3)} and len(bases) == 1
    return {"records": records, "source_witnesses": witnesses, "complete_repeat_frame_pairs": repetitions, "public_manifest": identity(public_path), "palette": identity(palette_path)}


def capture(configuration, case, native_report=None):
    """Observe one exact paid original through the ordinary application and verify real controls without world commands.

    @param configuration Current Debug or optimized native reader configuration.
    @param case Exact sealed connected-ready or original useful Magma-service world.
    @param native_report Optional explicit current same-binary native report; verifier-only fixes can reuse the exact complete observation without latest-run discovery.
    @return None; publish only after exact original pixels, captions, repetitions, saved owners and unchanged input pass.
    """
    selected, scene = original(configuration, case); context = current_run_context("graphics", "selected-control-raster"); controller_log = None
    if native_report is None:
        script = ROOT / "src/tests/multilayer/utilities/geothermal_window_raster_checks.py"
        process = subprocess.run([sys.executable, str(script), "--config", configuration, "--scene", scene, "--input", str(ROOT / selected["input"]["path"])], cwd=ROOT,
                                 env=dict(os.environ, OTTD_GEO_CONTROL_CAPTURE="1"), capture_output=True, timeout=110, check=False)
        log = context.logs / f"{configuration}-{case}-controller.log"; log.write_text((process.stdout + process.stderr).decode("utf-8", errors="replace"), encoding="utf-8")
        assert process.returncode == 0, log
        path = Path(process.stdout.decode("utf-8").splitlines()[-1]); controller_log = identity(log)
    else:
        path = ROOT / native_report
    native = json.loads(path.read_text(encoding="utf-8")); cache = {}; verify_art_references(native, cache)
    assert native["configuration"] == configuration and native["input"] == selected["input"] and native["binary"] == identity(ROOT / f"build/{configuration}/openttd_test.exe")
    assert native["case_runs"] == 1 and native["all_complete_native_chunks_equal"] and compare_worlds(native["before"], native["after"]) == 126
    proof = inspect(native); assert identity(ROOT / selected["input"]["path"]) == selected["input"]
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84BD.2; actual native enabled/disabled/pressed tool icons and caption layout on unchanged paid worlds",
              "configuration": configuration, "case": case, "original": selected, "native_report": identity(path), "native": native, **proof,
              "controller": identity(__file__), "controller_log": controller_log, "sources": [identity(ROOT / name) for name in SOURCES],
              "remaining": "Paired-reader/four-target84BD.2 acceptance,return84B.2,84C/84D;whole84.3/master10.7/human/network/operator gates remain pending."}
    output = context.output("reports", f"selected-control-raster-{configuration}-{case}.json"); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(output, flush=True)


def accept(focused_reports, raster_reports, build_logs, output):
    """Accept current selected control callbacks, original paid-world status pixels and both ordinary readers/four native targets.

    @param focused_reports Current Debug/optimized five-tag native control bursts.
    @param raster_reports Four exact current original-world icon/layout reports, one per configuration and selected paid scenario.
    @param build_logs Four distinct successful current native game/test build diagnostics.
    @param output Absent immutable complete-control receipt beneath repository logs.
    @return None; stale source/binary, incorrect state/pixel/layout, changed owner or missing native target prevents closure.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    cache, focused, reports = {}, {}, {}; totals = {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    for name in focused_reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, cache); cfg = report["configuration"]
        assert cfg in totals and cfg not in focused and tuple(row["case"] for row in report["native"]) == FOCUSED_CASES
        assert report["binary"] == identity(ROOT / f"build/{cfg}/openttd_test.exe")
        for proof in report["native"]:
            count_native(proof, totals[cfg])
        focused[cfg] = {"report": identity(path), **report}
    assert focused.keys() == totals.keys()
    for name in raster_reports:  # Only these exact current paid originals establish runtime pixels; old packed contact sheets or a metadata boolean cannot substitute.
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, cache); cfg, case = report["configuration"], report["case"]
        assert cfg in totals and case in CASES and (cfg, case) not in reports and report["original"] == original(cfg, case)[0]
        assert report["native"]["binary"] == focused[cfg]["binary"] and compare_worlds(report["native"]["before"], report["native"]["after"]) == 126
        assert inspect(report["native"])["source_witnesses"] == report["source_witnesses"]
        totals[cfg]["cases"] += report["native"]["case_runs"]; totals[cfg]["assertions"] += report["native"]["assertions"]
        reports[cfg, case] = {"report": identity(path), **report}
    assert reports.keys() == {(cfg, case) for cfg in totals for case in CASES} and totals["Debug"] == totals["RelWithDebInfo"]
    frames = []
    for case in CASES:
        left, right = reports["Debug", case], reports["RelWithDebInfo", case]
        for stage in ("before", "after"):
            assert compare_worlds(left["native"][stage], right["native"][stage]) == 126
        a = {(row["layer"], row["mode"]): row for row in left["complete_repeat_frame_pairs"]}
        b = {(row["layer"], row["mode"]): row for row in right["complete_repeat_frame_pairs"]}; assert a.keys() == b.keys()
        for key in a:
            with Image.open(ROOT / a[key]["first"]["path"]) as first, Image.open(ROOT / b[key]["first"]["path"]) as second:
                assert first.size == second.size and first.convert("RGB").tobytes() == second.convert("RGB").tobytes()
            frames.append({"case": case, "layer": key[0], "mode": key[1], "left": a[key]["first"], "right": b[key]["first"], "complete_rgb_exact": True})
    builds, targets, warnings = [], set(), []
    for name in build_logs:  # Each game/test configuration must reach its own actual executable; compile-only output or duplicate logs cannot establish a complete current build.
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert all(message not in log for message in ("error C", "fatal error", "error MSB", "warning LNK"))
        warnings.append({"build": identity(path), "existing_compiler_warnings": build_warnings(log)})
        found = {(cfg, target) for cfg in totals for target in ("openttd", "openttd_test") if f"{target}.vcxproj -> " in log and f"\\{cfg}\\{target}.exe" in log}
        assert len(found) == 1 and not (found & targets); targets.update(found); builds.append(identity(path))
    assert targets == {(cfg, target) for cfg in totals for target in ("openttd", "openttd_test")}
    grf = identity(ROOT / "media/baseset/openttd.grf"); assert grf["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84BD.2; actual selected control status/raster/layout and complete ordinary paid-world purity",
              "controller": identity(__file__), "focused": list(focused.values()), "rasters": list(reports.values()), "builds": builds, "build_warning_audit": warnings, "summary": totals, "native_version": 434,
              "native_cases": sum(row["cases"] for row in totals.values()), "native_assertions": sum(row["assertions"] for row in totals.values()),
              "complete_cross_reader_world_pairs": len(CASES) * 2, "complete_cross_reader_frame_pairs": frames,
              "complete_repeat_frame_pairs": sum(len(row["complete_repeat_frame_pairs"]) for row in reports.values()),
              "exact_original_icon_pixels": sum(w["original_exact_pixels"] for row in reports.values() for w in row["source_witnesses"]),
              "packed_grf": grf, "exact_identities_verified": len(cache), "binaries": [identity(ROOT / f"build/{cfg}/{target}.exe") for cfg in totals for target in ("openttd", "openttd_test")],
              "remaining": "Return84B.2 explicit validation,84C current selected gallery,84D publication;whole84.3/master10.7/human/network/operator gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": totals, "cross_reader_frame_pairs": len(frames), "repeat_pairs": result["complete_repeat_frame_pairs"], "exact_icon_pixels": result["exact_original_icon_pixels"]}), flush=True)


def main():
    """Select one bounded current paid icon observation or exact complete acceptance.

    @return None; required configurations/inputs/reports must exist and every native source/layout/owner check must pass.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--phase", choices=("capture", "accept"), default="capture")
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo")); parser.add_argument("--case", choices=CASES); parser.add_argument("--native-report")
    parser.add_argument("--focused-reports", nargs=2); parser.add_argument("--raster-reports", nargs=4); parser.add_argument("--build-logs", nargs=4); parser.add_argument("--output"); args = parser.parse_args()
    if args.phase == "capture":
        assert args.config and args.case; capture(args.config, args.case, args.native_report)
    else:
        assert args.focused_reports and args.raster_reports and args.build_logs and args.output; accept(args.focused_reports, args.raster_reports, args.build_logs, args.output)


if __name__ == "__main__":
    main()
