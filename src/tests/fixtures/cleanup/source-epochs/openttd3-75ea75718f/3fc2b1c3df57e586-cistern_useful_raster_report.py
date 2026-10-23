"""Accept the actual useful Cistern's opposite-reader pixels, native picking and complete render purity.

@file cistern_useful_raster_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DDE.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Original file evidence stays opaque accepted history; current reports independently bind current source/build identities.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

from PIL import Image

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from local_electrical_feed_checks import identity
from geothermal_tube_work_files import compare_chunks, compare_next_step
from tube_sprite_report import compare_worlds, verify_references
from mole_useful_raster_report import count_native
from cistern_useful_raster import FILE_RECEIPT


def main():
    """Require real loaded model11 inspection, unchanged native contracts and current four-target evidence.

    @return None; stale originals, changed complete worlds or any missing original hull/picking evidence prevent acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--affected-reports", nargs=8, required=True, help="Current body, geometry, useful route and loaded scene2 next-opportunity pair per build.")
    parser.add_argument("--build-logs", nargs=4, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / FILE_RECEIPT
    assert identity(receipt)["sha256"] == "9dbd7940192ea5f9d1a4b24645df880f5ef1057d49bda68174064ebb813c1e7b"
    history = json.loads(receipt.read_text(encoding="utf-8"))
    writers = {row["configuration"]: row for row in history["records"] if row.get("phase") == "write"}
    original_receipt = ROOT / history["writer_receipt"]["path"]; assert identity(original_receipt) == history["writer_receipt"]
    original_writers = {row["configuration"]: row for row in json.loads(original_receipt.read_text(encoding="utf-8"))["records"] if row.get("phase") == "write"}
    configurations = ("Debug", "RelWithDebInfo"); cache, views, affected, records = {}, {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}; visible = occluded = 0
    # Each independent native view must load its exact opposite accepted
    # original20CHEM world. Real common opportunities earn the inspection
    # point; rendering cannot replace source history with a matching scalar.
    for name in args.reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in configurations and cfg not in views and report["focus"] == "raster"
        opposite = configurations[1 - configurations.index(cfg)]; writer = writers[opposite]
        assert report["writer"] == writer["report"] and report["file_receipt"] == identity(receipt)
        point = next(row for row in writer["checkpoints"] if (row["scene"], row["step"]) == (2, 0))
        assert report["checkpoint"] == point and point["snapshot"]["part6_units"] == 20
        assert report["native"]["input"] == point["input"] and report["native"]["action"] == "raster" and report["native"]["preview_pure"]
        assert report["native_steps"] == report["end_tick"] - report["start_tick"] > 0
        assert report["start_tick"] == point["tick"] and report["consumed_j"] > 0
        view = report["view"]; assert view["model"] == 11 and view["speed"] > 0 and view["tick"] == report["end_tick"]
        assert view["native_picking"] and view["wrong_layer_refused"] and view["rims"]
        assert compare_worlds(view["before"], view["after"]) == compare_worlds(view["after"], report["native"]["output"]) == 126
        assert {row["zoom"] for row in view["packed_pixels"]["matches"]} == {1, 2}
        # Every non-company hull point remains visible or is covered by an
        # exact original foreground at its actual physical projection. Existing
        # whole-hull/interior legibility bounds do not discard outline pixels.
        for pixels in view["packed_pixels"]["matches"]:
            scale = 2 if pixels["zoom"] == 1 else 1
            assert pixels["exact_pixels"] == pixels["source_points"] * scale * scale
            assert pixels["interior_visible_pixels"] * 2 >= pixels["interior_source_points"] * scale * scale
            assert pixels["visible_pixels"] * 2 >= pixels["exact_pixels"]
            assert pixels["visible_pixels"] + pixels["occluded_by_original_rim_pixels"] == pixels["exact_pixels"]
            visible += pixels["visible_pixels"]; occluded += pixels["occluded_by_original_rim_pixels"]
        count_native(report["native"], summary[cfg]); views[cfg] = report; records.append({"report": identity(path), **report})
    assert views.keys() == set(configurations)
    # Only directly affected body/geometry/useful-route/scene2 continuation
    # cases run. The new raster action cannot alter the ordinary reader's next
    # common opportunity or any source, packet, account or original recipe.
    for name in args.affected_reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in configurations
        focus = "route" if report.get("focus") == "cistern-useful" else report.get("focus", report.get("phase"))
        assert focus in ("body", "geometry", "route", "step") and (cfg, focus) not in affected
        processes = report["native"] if focus == "step" else [report["native"]]
        if focus == "step":
            assert report["case"] == 2 and len(processes) == 2
            opposite = configurations[1 - configurations.index(cfg)]; writer = original_writers[opposite]
            assert report["writer"] == writer["report"]
            points = {row["step"]: row for row in writer["checkpoints"] if row["scene"] == 2}
            assert report["checkpoint"] == points[0] and report["uninterrupted_baseline"] == points[1]["input"]
            first, second = (ROOT / row["output"]["path"] for row in processes)
            assert report["next_step"] == compare_next_step(first, second, points[0]["tick"], points[1]["tick"])
            assert compare_chunks(first, second, None) == 126
            assert compare_chunks(ROOT / views[cfg]["checkpoint"]["input"]["path"], ROOT / points[0]["input"]["path"], None) == 126
        for process in processes:
            count_native(process, summary[cfg])
        affected[cfg, focus] = report; records.append({"report": identity(path), **report})
    assert affected.keys() == {(cfg, focus) for cfg in configurations for focus in ("body", "geometry", "route", "step")}
    assert summary["Debug"] == summary["RelWithDebInfo"]
    assert affected["Debug", "route"]["result"] == affected["RelWithDebInfo", "route"]["result"]
    left, right = (views[cfg] for cfg in configurations)
    for field in ("native_steps", "start_tick", "end_tick", "consumed_j"):
        assert left[field] == right[field]
    for field in ("model", "view", "direction", "frame", "sprite", "tile", "mask", "track", "speed", "tick", "dx", "dy", "rims", "packed_pixels"):
        assert left["view"][field] == right["view"][field]
    pairs, frames = [], []
    for boundary in ("before", "after", "output"):
        first, second = (row["native"]["output"] if boundary == "output" else row["view"][boundary] for row in (left, right))
        assert compare_worlds(first, second) == 126
        pairs.append({"boundary": boundary, "left": first, "right": second, "complete_chunks_exact": 126})
    # Full RGB equality also retains company remaps/backgrounds independently
    # of indexed hull matching. Both builds/repeats compare every640x360 pixel.
    expected = {(zoom, repeat) for zoom in (1, 2) for repeat in range(2)}; previous = {}
    assert len(left["view"]["frames"]) == len(right["view"]["frames"]) == 4
    assert {(row["zoom"], row["repeat"]) for row in left["view"]["frames"]} == expected
    for first, second in zip(left["view"]["frames"], right["view"]["frames"]):
        assert (first["zoom"], first["repeat"]) == (second["zoom"], second["repeat"])
        a, b = (Image.open(ROOT / row["file"]["path"]).convert("RGB") for row in (first, second))
        assert a.size == b.size == (640, 360) and a.tobytes() == b.tobytes()
        zoom = first["zoom"]
        if zoom in previous:
            assert previous[zoom] == a.tobytes()
        previous[zoom] = a.tobytes()
        frames.append({"zoom": zoom, "repeat": first["repeat"], "left": first["file"], "right": second["file"],
                       "rgb_sha256": hashlib.sha256(a.tobytes()).hexdigest(), "pixels_exact": True})
    builds = []
    for name in args.build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log
        assert "openttd.exe" in log or "openttd_test.exe" in log; builds.append(identity(path))
    packed = identity(ROOT / "media/baseset/openttd.grf")
    assert packed["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DDE/original15DD.2/15D.1; actual useful moving Cistern pixels/picking and complete native purity",
        "controller": identity(__file__), "summary": summary, "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()), "records": records,
        "complete_world_pairs": pairs, "cross_build_frames": frames, "exact_visible_hull_pixels": visible,
        "exact_occluding_rim_pixels": occluded, "exact_identities_verified": len(cache), "builds": builds,
        "native_version": 434, "packed_art": packed, "file_receipt": identity(receipt),
        "binaries": [identity(ROOT / f"build/{cfg}/{target}.exe") for cfg in configurations for target in ("openttd", "openttd_test")],
        "fixture_classification": "Prepared floor/machinery/Connected shafts and saved multi-industry option remain boundaries. Actual original raw production/CHEM manufacture, conventional liquid feed, local paid Cistern and conventional Surface PCHM intake/output retain all nine original parts, four industries, six stations, raw histories/reserve/unused Oil, packets and native current/old accounts. The accepted loaded20CHEM capsule earns its clear inspection point through the complete shared native driver. Pure rendering/picking changes no original owner; original art/pixel bounds remain unchanged.",
        "remaining": "Original15.2 retains Shield1985/Forge2005 and the conventional Underground/Deep chain; actual paid TBM/bootstrap, runtime gallery and human/operator gates remain pending.",
    }
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "complete_world_pairs": len(pairs), "frame_pairs": len(frames)}))


if __name__ == "__main__":
    main()
