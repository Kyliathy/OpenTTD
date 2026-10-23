"""Verify original paid native riser campuses against approved equipment pixels and whole saved owners.

@file geothermal_riser_raster.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BB.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
All construction, ready, transferred and sealed inputs are exact earlier paid-world originals; no builder or work/energy grant runs here.
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
from geothermal_machinery_raster import ROOT, identity, match_contour, verify_art_references, original as generator_original
from runner import current_run_context
from tube_sprite_report import compare_worlds
from mole_useful_raster_report import count_native

RECEIPTS = {
    "funded": ("logs/geothermal/first-plant/20261017-native-01/construction-preparation.json", "c1b654aace9712626299aa8fbfd403695f7322d362bc173b27ad4cdc38b05dd2"),
    "ready": ("logs/geothermal/first-plant/20261017-native-01/construction-delivery.json", "3e96de8db98f83de8a94da551fbdcad869faf476c6e560f906f06ade7b14cb54"),
    "sealed": ("logs/geothermal/first-plant/20261017-native-01/selected-saves-verification.json", "311a0851c785e50d49c9dbf96d3920f8edd196a2affe1590f3e2d59f4f553da4"),
}
CASES = ("funded", "ready", "working", "sealed")
SOURCES = ("src/geothermal_sprite.h", "src/geothermal_sprite.cpp", "src/geothermal_riser_sprite.cpp", "src/geothermal_riser_construction.cpp",
           "src/multilayer_sprite.cpp", "src/industry_cmd.cpp", "src/tests/geothermal_window_raster.cpp", "src/tests/native_art_render_fixture.h",
           "src/tests/multilayer/geothermal_riser_raster.py", "src/tests/multilayer/geothermal_machinery_raster.py")


def original(configuration, case):
    """Resolve an exact paid original, retaining old producer receipts opaquely.

    @param configuration Current native reader; construction/service select the opposite original writer.
    @param case Exact funded, fully delivered ready, actual useful transfer or original native sealed boundary.
    @return Hash-bound sealed receipt/input identities; the sealed case uses the one original paid fault pinned by selected-save acceptance.
    """
    if case == "working":
        selected, _ = generator_original(configuration, "vent-working")
        return selected
    name, sha = RECEIPTS[case]; path = ROOT / name; assert identity(path)["sha256"] == sha
    receipt = json.loads(path.read_text(encoding="utf-8")); opposite = "RelWithDebInfo" if configuration == "Debug" else "Debug"
    selected = {"receipt": identity(path)}
    if case == "funded":
        point = receipt["prepared_worlds"][opposite]
    elif case == "ready":
        point = receipt["original_checkpoints"][opposite]["full"]["complete"]
    else:
        manifest_path = ROOT / receipt["input_manifest"]["path"]; assert identity(manifest_path) == receipt["input_manifest"]
        fault = json.loads(manifest_path.read_text(encoding="utf-8"))["cases"]["vt-failed-sealed"]
        selected.update(manifest=identity(manifest_path), original_receipt=fault["original_receipt"]); point = fault["input"]
    selected["input"] = {key: point[key] for key in ("path", "bytes", "sha256")}
    assert identity(ROOT / selected["input"]["path"]) == selected["input"]
    return selected


def expected_state(record, case):
    """Require scenario-specific original work/portal/flow facts before inspecting source or rendered pixels.

    @param record Actual native paid machinery/retained-link observation, independent of the decoded sprite state.
    @param case Original receipt-selected construction, availability, transfer or sealed scenario.
    @return Exact approved state; a different native scenario cannot pass by selecting a visually convenient pose.
    """
    assert record["asset_id"] == "facility.utility-riser" and record["layer"] in (1, 3)
    if case == "funded":
        assert record["work"] == 0 and not record["materials_committed"] and not record["operational"] and not record["links"]
        return 0
    assert record["work"] == 296 and record["materials_committed"]
    flowing = any(link["active"] and link["flow"] > 0 for link in record["links"])
    if case == "sealed" and record["layer"] == 3:
        # The retained paid fault seals the original Deep pair only; actual
        # unavailable machinery stays blocked while the upper pair survives.
        assert not record["operational"] and not flowing
        return 3
    assert record["operational"]
    if case == "working":
        assert flowing
        return 2
    assert not flowing
    return 1


def capture(configuration, case):
    """Capture both real paid risers through the existing ordinary native viewport observer.

    @param configuration Actual current Debug or optimized native test executable.
    @param case Exact original paid work/portal/flow boundary, never a newly built fixture.
    @return None; source contours, full world owners, clock, RNG and unchanged original must pass before publication.
    """
    selected = original(configuration, case); context = current_run_context("graphics", "geothermal-riser-raster")
    script = ROOT / "src/tests/multilayer/utilities/geothermal_window_raster_checks.py"
    process = subprocess.run([sys.executable, str(script), "--config", configuration, "--scene", "magma-loop", "--input", str(ROOT / selected["input"]["path"])], cwd=ROOT,
                             env=dict(os.environ, OTTD_GEO_RISER_CAPTURE="1"), capture_output=True, timeout=110, check=False)
    log = context.logs / f"{configuration}-{case}-controller.log"; log.write_text((process.stdout + process.stderr).decode("utf-8", errors="replace"), encoding="utf-8")
    assert process.returncode == 0, log
    path = Path(process.stdout.decode("utf-8").splitlines()[-1]); native = json.loads(path.read_text(encoding="utf-8")); assert native["input"] == selected["input"]
    assert native["all_complete_native_chunks_equal"] and compare_worlds(native["before"], native["after"]) == 126
    raw = (ROOT / native["log"]["path"]).read_text(encoding="utf-8"); records = [json.loads(line) for line in re.findall(r"^GEO_RISER_FRAME (.+)$", raw, re.M)]
    assert len(records) == 4 and {(row["layer"], row["zoom"]) for row in records} == {(layer, zoom) for layer in (1, 3) for zoom in (1, 2)}
    contours = []
    for record in records:  # Each actual rock-plane sender and both full native zooms remain mandatory, even when the other sender is blocked.
        state = expected_state(record, case); contours.append(dict(match_contour(record, state), industry=record["industry"], layer=record["layer"]))
    assert identity(ROOT / selected["input"]["path"]) == selected["input"]
    report = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84BB.2; actual paid original riser bodies, not whole84BB.3/gallery acceptance",
              "configuration": configuration, "case": case, "original": selected, "native_report": identity(path), "native": native, "campuses": records, "contours": contours,
              "controller": identity(__file__), "sources": [identity(ROOT / name) for name in SOURCES], "controller_log": identity(log),
              "remaining": "84BB.3 current whole machinery transparency/occlusion/layer/picking/four-target checks,84BC/84BD/84B.2/84C/84D; human/network/operator acceptance remains pending."}
    output = context.output("reports", f"riser-campus-{configuration}-{case}.json"); output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(output, flush=True)


def accept(focused_reports, raster_reports, build_logs, output):
    """Accept current native riser boundaries and every exact paid-campus scenario in both readers.

    @param focused_reports Current Debug/optimized isolated riser selector/native owner reports.
    @param raster_reports Eight current exact paid-riser capture reports; all four original cases must cover both configurations.
    @param build_logs Successful current native test-target diagnostics.
    @param output Absent immutable riser-only acceptance receipt beneath repository logs.
    @return None; stale identities, missing states, differing complete RGB frames or any changed native owner prevent closure.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    cache, focused, rasters, pairs = {}, {}, {}, []; totals = {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    for name in focused_reports:  # Current native boundaries bind freshly; earlier paid producer binaries remain opaque dated receipt identities.
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, cache)
        cfg = report["configuration"]; assert cfg in totals and cfg not in focused and report["part"] == "risers"
        for proof in report["native"]:
            count_native(proof, totals[cfg])
        focused[cfg] = {"report": identity(path), **report}
    assert focused.keys() == totals.keys()
    for name in raster_reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, cache)
        cfg, case = report["configuration"], report["case"]; key = cfg, case
        assert cfg in totals and case in CASES and key not in rasters
        assert report["original"] == original(cfg, case) and report["native"]["binary"] == focused[cfg]["binary"]
        assert report["native"]["case_runs"] == 1 and compare_worlds(report["native"]["before"], report["native"]["after"]) == 126
        for record in report["campuses"]:
            match_contour(record, expected_state(record, case))
        totals[cfg]["cases"] += 1; totals[cfg]["assertions"] += report["native"]["assertions"]; rasters[key] = {"report": identity(path), **report}
    assert rasters.keys() == {(cfg, case) for cfg in totals for case in CASES} and totals["Debug"] == totals["RelWithDebInfo"]
    pixels = 0
    for case in CASES:  # Corresponding actual whole worlds and all original full RGB campuses must agree without masks, palette normalization or search.
        left, right = rasters["Debug", case], rasters["RelWithDebInfo", case]
        for stage in ("before", "after"):
            assert compare_worlds(left["native"][stage], right["native"][stage]) == 126
        first = {(row["layer"], row["zoom"]): row for row in left["contours"]}; second = {(row["layer"], row["zoom"]): row for row in right["contours"]}; assert first.keys() == second.keys()
        for key in first:
            a, b = first[key], second[key]; assert a["original_exact_contour_pixels"] == b["original_exact_contour_pixels"]
            with Image.open(ROOT / a["frame"]["path"]) as image_a, Image.open(ROOT / b["frame"]["path"]) as image_b:
                assert image_a.size == image_b.size and image_a.convert("RGB").tobytes() == image_b.convert("RGB").tobytes()
            pixels += a["original_exact_contour_pixels"] + b["original_exact_contour_pixels"]
            pairs.append({"case": case, "layer": key[0], "zoom": key[1], "left": a["frame"], "right": b["frame"], "complete_rgb_exact": True})
    builds = []
    for name in build_logs:  # Only terminal successful native targets may bind current selector/paid-world observer evidence.
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "fatal error" not in log and "warning C" not in log and "openttd_test.exe" in log
        builds.append(identity(path))
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84BB.2; pure actual native riser binding/paid original body proof only",
              "controller": identity(__file__), "focused": list(focused.values()), "rasters": list(rasters.values()), "builds": builds, "summary": totals,
              "native_cases": sum(row["cases"] for row in totals.values()), "native_assertions": sum(row["assertions"] for row in totals.values()),
              "complete_cross_reader_world_pairs": len(CASES) * 2, "complete_campus_frame_pairs": pairs, "exact_original_contour_pixels": pixels, "exact_identities_verified": len(cache),
              "native_version": 434, "remaining": "84BB.3 full machinery/four-target acceptance,84BC/84BD/84B.2/84C/84D; whole84.3/master10.7/human/network/operator gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": totals, "frame_pairs": len(pairs), "exact_contour_pixels": pixels}), flush=True)


def main():
    """Select one bounded actual paid native riser capture or complete riser-only acceptance.

    @return None; every required original/configuration/report must exist before dependent verification starts.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--phase", choices=("capture", "accept"), default="capture")
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo")); parser.add_argument("--case", choices=CASES)
    parser.add_argument("--focused-reports", nargs=2); parser.add_argument("--raster-reports", nargs="+"); parser.add_argument("--build-logs", nargs=2); parser.add_argument("--output"); args = parser.parse_args()
    if args.phase == "capture":
        assert args.config is not None and args.case is not None; capture(args.config, args.case)
    else:
        assert args.focused_reports is not None and args.raster_reports is not None and args.build_logs is not None and args.output is not None
        accept(args.focused_reports, args.raster_reports, args.build_logs, args.output)


if __name__ == "__main__":
    main()
