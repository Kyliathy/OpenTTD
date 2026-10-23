"""Check real paid geothermal machinery opacity, repetition, full native layers and logical industry picking.

@file geothermal_machinery_interaction.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BB.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Every viewport comes from an exact ordinary paid original; no new builder, stock, energy or source artwork is supplied.
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
from geothermal_machinery_raster import ROOT, CASES as GENERATOR_CASES, identity, original, match_contour, verify_art_references
from runner import current_run_context
from tube_sprite_report import compare_worlds
from mole_useful_raster_report import count_native

CASES = ("funded", "ready", "vent-working", "magma-working")
MODELS = {("facility.vent-collector", 3), ("facility.vent-powerhouse", 3), ("facility.magma-powerhouse", 3), ("facility.utility-riser", 1), ("facility.utility-riser", 3)}
SOURCES = ("src/geothermal_sprite.h", "src/geothermal_sprite.cpp", "src/geothermal_riser_sprite.cpp", "src/geothermal_generator.cpp", "src/geothermal_riser_construction.cpp",
           "src/multilayer_sprite.cpp", "src/industry_cmd.cpp", "src/viewport.cpp", "src/tests/geothermal_window_raster.cpp", "src/tests/geothermal_machinery_view_fixture.h",
           "src/tests/native_art_render_fixture.h", "src/tests/CMakeLists.txt", "src/tests/multilayer/geothermal_machinery_interaction.py", "src/tests/multilayer/geothermal_machinery_raster.py")
SEMANTICS = (
    ("logs/graphics/geothermal-machinery/20261018-native-01/generator-verification.json", "0bc586258ebf3a403b03587f2c91176535b60bacd9a476b55f97515f3f6eaa8c", ("src/geothermal_sprite.cpp", "src/geothermal_generator.cpp")),
    ("logs/graphics/geothermal-machinery/20261018-native-01/riser-verification.json", "58789b4be8d84807ec9b63a90ffc3793dcf4d002dd9b08a44cc9c397235ce1d0", ("src/geothermal_riser_sprite.cpp", "src/geothermal_riser_construction.cpp")),
)


def inspect(native, case):
    """Verify all actual model/layer/zoom/mode frames and source-defined local equipment witnesses.

    @param native Current ordinary native observer report, including all unchanged world owners and real application log.
    @param case Exact sealed paid generator scenario; prior generator/riser receipts own detailed state-boundary acceptance.
    @return Thirty native mode records, ten exact opaque source witnesses, full-frame repetitions and target-specific opacity deltas.
    """
    raw = (ROOT / native["log"]["path"]).read_text(encoding="utf-8")
    records = [json.loads(line) for line in re.findall(r"^GEO_MACHINERY_INTERACTION (.+)$", raw, re.M)]
    fallback = re.findall(r"^GEO_MACHINERY_FALLBACK industry=(\d+) unchanged_native_resolver=true$", raw, re.M)
    assert fallback and len(records) == 30
    grid = {(row["asset_id"], row["layer"], row["zoom"], row["mode"]): row for row in records}
    assert grid.keys() == {(asset, layer, zoom, mode) for asset, layer in MODELS for zoom in (1, 2) for mode in ("opaque", "transparent", "hidden")}
    witnesses, repetitions, opacity = [], [], []
    for asset, layer in sorted(MODELS):  # Both real rock-plane senders, retained collector and actual dated generator models must independently survive the complete native compositor.
        for zoom in (1, 2):
            selected = {mode: grid[asset, layer, zoom, mode] for mode in ("opaque", "transparent", "hidden")}; opaque = selected["opaque"]
            states = {cell["state"] for cell in opaque["cells"]}; assert len(states) == 1; state = states.pop()
            if asset in ("facility.vent-powerhouse", "facility.magma-powerhouse"):
                model = 0 if asset == "facility.vent-powerhouse" else 1
                assert state == GENERATOR_CASES[case][3][model]
            witness = match_contour(dict(opaque, path=opaque["paths"][0]), state)
            witnesses.append(dict(witness, industry=opaque["industry"], layer=layer))
            images = {}
            for mode, row in selected.items():
                assert row["native_industry_window"] and row["logical_pick_while_hidden"] and row["surface_same_owner_refused"]
                assert row["cells"] == opaque["cells"] and row["industry"] == opaque["industry"] and len(row["paths"]) == 2
                first, second = [Path(name).with_suffix(".png") for name in row["paths"]]
                with Image.open(first) as a, Image.open(second) as b:
                    assert a.size == b.size == (640, 360) and a.convert("RGB").tobytes() == b.convert("RGB").tobytes()
                    images[mode] = a.convert("RGB")
                repetitions.append({"asset_id": asset, "industry": row["industry"], "layer": layer, "zoom": zoom, "mode": mode,
                                    "first": identity(first), "second": identity(second), "complete_rgb_exact": True})
            # Fixed points come only from original exposed source geometry,
            # before observing any display-mode delta. Require opacity to
            # affect this machine's equipment, rather than merely a distant
            # neighbouring industry elsewhere in the full viewport.
            changes = {}
            for left, right in (("opaque", "transparent"), ("opaque", "hidden"), ("transparent", "hidden")):
                count = sum(images[left].getpixel(tuple(point)) != images[right].getpixel(tuple(point)) for point in witness["original_contour_points"])
                assert count > 0, (asset, layer, zoom, left, right)
                changes[f"{left}_to_{right}"] = count
            opacity.append({"asset_id": asset, "layer": layer, "zoom": zoom, "original_source_points": witness["original_exact_contour_pixels"], "changed_original_equipment_points": changes})
    return {"records": records, "opaque_source_witnesses": witnesses, "complete_repeat_frame_pairs": repetitions, "local_opacity": opacity, "unchanged_fallback_industries": fallback}


def capture(configuration, case, native_report=None):
    """Capture or freshly inspect one explicit current paid-world interaction observation.

    @param configuration Actual current native reader configuration.
    @param case Exact paid original scenario selected through its sealed producer receipt.
    @param native_report Optional explicit current same-binary native report; avoids repeating an already complete ordinary observation and never discovers a latest run.
    @return None; publish only after native state/RNG/source/pixel/opacity/repetition/picking checks pass.
    """
    selected, scene = original(configuration, case); context = current_run_context("graphics", "geothermal-machinery-interaction")
    controller_log = None
    if native_report is None:
        script = ROOT / "src/tests/multilayer/utilities/geothermal_window_raster_checks.py"
        process = subprocess.run([sys.executable, str(script), "--config", configuration, "--scene", scene, "--input", str(ROOT / selected["input"]["path"])], cwd=ROOT,
                                 env=dict(os.environ, OTTD_GEO_MACHINERY_INTERACTION="1"), capture_output=True, timeout=110, check=False)
        log = context.logs / f"{configuration}-{case}-controller.log"; log.write_text((process.stdout + process.stderr).decode("utf-8", errors="replace"), encoding="utf-8")
        assert process.returncode == 0, log
        path = Path(process.stdout.decode("utf-8").splitlines()[-1]); controller_log = identity(log)
    else:
        path = ROOT / native_report
    native = json.loads(path.read_text(encoding="utf-8")); cache = {}; verify_art_references(native, cache)
    assert native["configuration"] == configuration and native["input"] == selected["input"] and native["binary"] == identity(ROOT / f"build/{configuration}/openttd_test.exe")
    assert native["case_runs"] == 1 and native["all_complete_native_chunks_equal"] and compare_worlds(native["before"], native["after"]) == 126
    proof = inspect(native, case); assert identity(ROOT / selected["input"]["path"]) == selected["input"]
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84BB.3; complete actual paid machinery opacity/repetition/native logical industry picking; detailed owner states remain84BB.1/.2",
              "configuration": configuration, "case": case, "original": selected, "native_report": identity(path), "native": native, **proof,
              "controller": identity(__file__), "sources": [identity(ROOT / name) for name in SOURCES], "controller_log": controller_log,
              "remaining": "Current four-target acceptance and paired-reader interaction closure,then84BC/84BD/84B.2/84C/84D; whole84.3/master10.7/human/network/operator gates remain pending."}
    output = context.output("reports", f"machinery-interaction-{configuration}-{case}.json"); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(output, flush=True)


def accept(focused_reports, interaction_reports, build_logs, output):
    """Accept complete current machinery selectors and paid display/picking behavior from both native readers and four successful targets.

    @param focused_reports Current Debug/optimized isolated combined machinery reports.
    @param interaction_reports Eight current original interaction reports, covering all four paid scenarios in both readers.
    @param build_logs Four successful current Debug/optimized native game/test build logs.
    @param output Absent immutable complete-machinery acceptance receipt below repository logs.
    @return None; any stale binary/source, changed world/pixel, failed target or missing display/layer/model prevents closure.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    cache, focused, reports, pairs = {}, {}, {}, []; totals = {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    for name in focused_reports:  # Fresh current selectors supplement the exact earlier model/riser semantic receipts, without reopening their sealed original builders.
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, cache)
        cfg = report["configuration"]; assert cfg in totals and cfg not in focused and report["part"] == "machinery"
        for proof in report["native"]:
            count_native(proof, totals[cfg])
        focused[cfg] = {"report": identity(path), **report}
    assert focused.keys() == totals.keys()
    for name in interaction_reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, cache)
        cfg, case = report["configuration"], report["case"]; key = cfg, case; assert cfg in totals and case in CASES and key not in reports
        assert report["original"] == original(cfg, case)[0] and report["native"]["binary"] == focused[cfg]["binary"]
        assert compare_worlds(report["native"]["before"], report["native"]["after"]) == 126
        assert inspect(report["native"], case)["local_opacity"] == report["local_opacity"]
        totals[cfg]["cases"] += report["native"]["case_runs"]; totals[cfg]["assertions"] += report["native"]["assertions"]; reports[key] = {"report": identity(path), **report}
    assert reports.keys() == {(cfg, case) for cfg in totals for case in CASES} and totals["Debug"] == totals["RelWithDebInfo"]
    for case in CASES:  # Both readers retain every current complete native owner and the entire corresponding RGB viewport in every mode, never a normalized mask.
        left, right = reports["Debug", case], reports["RelWithDebInfo", case]
        for stage in ("before", "after"):
            assert compare_worlds(left["native"][stage], right["native"][stage]) == 126
        first = {(row["asset_id"], row["layer"], row["zoom"], row["mode"]): row for row in left["complete_repeat_frame_pairs"]}
        second = {(row["asset_id"], row["layer"], row["zoom"], row["mode"]): row for row in right["complete_repeat_frame_pairs"]}; assert first.keys() == second.keys()
        for key in first:
            a, b = first[key], second[key]
            with Image.open(ROOT / a["first"]["path"]) as image_a, Image.open(ROOT / b["first"]["path"]) as image_b:
                assert image_a.size == image_b.size and image_a.convert("RGB").tobytes() == image_b.convert("RGB").tobytes()
            pairs.append({"case": case, "asset_id": key[0], "layer": key[1], "zoom": key[2], "mode": key[3], "left": a["first"], "right": b["first"], "complete_rgb_exact": True})
    builds, targets = [], set()
    for name in build_logs:  # Each distinct current game/test configuration must really reach its own successful native target; four copies of one log cannot substitute.
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "fatal error" not in log and "warning C" not in log
        found = {(cfg, target) for cfg in totals for target in ("openttd", "openttd_test") if f"{target}.vcxproj -> " in log and f"\\{cfg}\\{target}.exe" in log}
        assert len(found) == 1 and not (found & targets); targets.update(found); builds.append(identity(path))
    assert targets == {(cfg, target) for cfg in totals for target in ("openttd", "openttd_test")}
    semantic_receipts = []
    for name, sha, source_names in SEMANTICS:  # Earlier exact state/paid-world contracts stay sealed; require their actual selector/work implementations to remain byte-identical now.
        path = ROOT / name; assert identity(path)["sha256"] == sha; receipt = json.loads(path.read_text(encoding="utf-8"))
        original_sources = {row["path"]: row for row in receipt["focused"][0]["sources"]}
        for source_name in source_names:
            assert identity(ROOT / source_name) == original_sources[source_name]
        semantic_receipts.append({"receipt": identity(path), "unchanged_semantic_sources": [original_sources[source_name] for source_name in source_names]})
    binaries = [identity(ROOT / f"build/{cfg}/{target}.exe") for cfg in totals for target in ("openttd", "openttd_test")]
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84BB.3; current complete paid machinery/native opacity/layers/logical industry picking, not whole selected gallery publication",
              "controller": identity(__file__), "focused": list(focused.values()), "interactions": list(reports.values()), "builds": builds, "binaries": binaries, "summary": totals, "semantic_receipts": semantic_receipts,
              "native_cases": sum(row["cases"] for row in totals.values()), "native_assertions": sum(row["assertions"] for row in totals.values()), "native_version": 434,
              "complete_cross_reader_world_pairs": len(CASES) * 2, "complete_cross_reader_frame_pairs": pairs,
              "complete_repeat_frame_pairs": sum(len(row["complete_repeat_frame_pairs"]) for row in reports.values()),
              "exact_original_contour_pixels": sum(witness["original_exact_contour_pixels"] for row in reports.values() for witness in row["opaque_source_witnesses"]),
              "exact_identities_verified": len(cache), "remaining": "84BC conduits,84BD control icons,return84B.2,84C current selected gallery,84D publication; whole84.3/master10.7/human/network/operator gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": totals, "cross_reader_frame_pairs": len(pairs), "repeat_pairs": result["complete_repeat_frame_pairs"], "exact_contour_pixels": result["exact_original_contour_pixels"]}), flush=True)


def main():
    """Select one bounded paid-world interaction check or complete current machinery acceptance.

    @return None; required exact inputs/reports and configurations must be present before verification starts.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--phase", choices=("capture", "accept"), default="capture")
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo")); parser.add_argument("--case", choices=CASES); parser.add_argument("--native-report")
    parser.add_argument("--focused-reports", nargs=2); parser.add_argument("--interaction-reports", nargs="+"); parser.add_argument("--build-logs", nargs=4); parser.add_argument("--output"); args = parser.parse_args()
    if args.phase == "capture":
        assert args.config is not None and args.case is not None; capture(args.config, args.case, args.native_report)
    else:
        assert args.focused_reports is not None and args.interaction_reports is not None and args.build_logs is not None and args.output is not None
        accept(args.focused_reports, args.interaction_reports, args.build_logs, args.output)


if __name__ == "__main__":
    main()
