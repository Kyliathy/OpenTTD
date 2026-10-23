"""Accept the real useful Crate's opposite-reader rendering and complete native world purity.

@file crate_useful_raster_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md task15CC.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Historical producers remain immutable inputs; their dated binary/source identities are not current build claims.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

from PIL import Image

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from local_electrical_feed_checks import identity
from geothermal_tube_work_files import compare_chunks, compare_next_step
from tube_sprite_report import compare_worlds, verify_references
from crate_useful_files import OWNERS
from crate_useful_raster import FILE_RECEIPT


def count_native(proof, summary):
    """Check an original successful native log before counting its focused assertions.

    @param proof Exact native process evidence and original diagnostic identity.
    @param summary Mutable case/assertion totals for this process's configuration.
    @return None; changed logs or mismatched counts refuse acceptance.
    """
    log = (ROOT / proof["log"]["path"]).read_text(encoding="utf-8")
    count = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", log)
    assert count is not None and "NATIVE_EXIT_CODE=0" in log
    assert (int(count[1]), int(count[2])) == (proof["assertions"], proof["cases"])
    summary["cases"] += proof["cases"]
    summary["assertions"] += proof["assertions"]


def main():
    """Require both real loaded views, affected native contracts and four successful current targets.

    @return None; stale identities, changed worlds or any frame/source mismatch prevent the absent receipt.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True, help="Exact current opposite-reader raster reports.")
    parser.add_argument("--affected-reports", nargs=8, required=True, help="Body, geometry, useful Crate and loaded step2 per build.")
    parser.add_argument("--build-logs", nargs=4, required=True)
    parser.add_argument("--output", required=True, help="Absent immutable receipt beneath logs.")
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    receipt = ROOT / FILE_RECEIPT
    assert identity(receipt)["sha256"] == "34eaf3f165d8b4c2b712c619c5481b94b0fdc3c0288a03f96d937b5a01f99457"
    history = json.loads(receipt.read_text(encoding="utf-8"))
    writers = {row["configuration"]: row for row in history["records"] if row.get("phase") == "write"}
    assert writers.keys() == set(configurations)
    cache, views, affected, records = {}, {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    visible = occluded = 0
    # Each view must come from the exact opposite accepted useful-route writer,
    # retaining its original39 units. The historical receipt is opaque dated
    # evidence; only current reports' references make current source/build claims.
    for name in args.reports:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]
        assert cfg in configurations and cfg not in views and report["focus"] == "raster"
        opposite = configurations[1 - configurations.index(cfg)]
        writer = writers[opposite]
        assert report["writer"] == writer["report"] and report["file_receipt"] == identity(receipt)
        point = next(row for row in writer["checkpoints"] if (row["scene"], row["step"]) == (2, 0))
        assert report["checkpoint"] == point and point["snapshot"]["part2_units"] == 39 and point["snapshot"]["total_delivered"] == 0
        assert report["native"]["input"] == point["input"] and report["native"]["action"] == "raster"
        assert report["native_steps"] == report["end_tick"] - report["start_tick"] > 0
        assert report["start_tick"] == point["tick"] and report["consumed_j"] > 0
        view = report["view"]
        assert view["model"] == 10 and view["speed"] > 0 and view["tick"] == report["end_tick"]
        assert view["native_picking"] and view["wrong_layer_refused"] and view["rims"]
        assert compare_worlds(view["before"], view["after"]) == 126
        assert compare_worlds(view["after"], report["native"]["output"]) == 126
        assert {row["zoom"] for row in view["packed_pixels"]["matches"]} == {1, 2}
        # Every non-company hull point must be visible or covered by an exact
        # original guideway rim. Half-whole-hull and half-interior thresholds
        # protect legibility without removing any outline pixel from coverage.
        for pixels in view["packed_pixels"]["matches"]:
            scale = 2 if pixels["zoom"] == 1 else 1
            assert pixels["exact_pixels"] == pixels["source_points"] * scale * scale
            assert pixels["interior_visible_pixels"] * 2 >= pixels["interior_source_points"] * scale * scale
            assert pixels["visible_pixels"] * 2 >= pixels["exact_pixels"]
            assert pixels["visible_pixels"] + pixels["occluded_by_original_rim_pixels"] == pixels["exact_pixels"]
            visible += pixels["visible_pixels"]
            occluded += pixels["occluded_by_original_rim_pixels"]
        count_native(report["native"], summary[cfg])
        views[cfg] = report
        records.append({"report": identity(path), **report})
    assert views.keys() == set(configurations)
    # Verify only directly affected geometry/body/route/loading contracts.
    # The genuine full-driver step must still match its untouched opposite
    # writer baseline after adding a pure renderer action to the same observer.
    for name in args.affected_reports:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]
        assert cfg in configurations
        focus = "crate" if "source_receipt" in report else report.get("focus", report.get("phase"))
        assert focus in ("body", "geometry", "crate", "step") and (cfg, focus) not in affected
        processes = report["native"] if focus == "step" else [report["native"]]
        if focus == "step":
            assert report["case"] == 2 and len(processes) == 2
            writer = writers[configurations[1 - configurations.index(cfg)]]
            assert report["writer"] == writer["report"]
            points = {row["step"]: row for row in writer["checkpoints"] if row["scene"] == 2}
            assert report["checkpoint"] == points[0] and report["uninterrupted_baseline"] == points[1]["input"]
            first, second = [ROOT / row["output"]["path"] for row in processes]
            assert report["next_step"] == compare_next_step(first, second, points[0]["tick"], points[1]["tick"])
            assert compare_chunks(first, second, OWNERS) == len(OWNERS)
        for process in processes:
            count_native(process, summary[cfg])
        affected[cfg, focus] = report
        records.append({"report": identity(path), **report})
    assert affected.keys() == {(cfg, focus) for cfg in configurations for focus in ("body", "geometry", "crate", "step")}
    assert summary["Debug"] == summary["RelWithDebInfo"]
    assert affected["Debug", "crate"]["route"] == affected["RelWithDebInfo", "crate"]["route"]
    left, right = [views[cfg] for cfg in configurations]
    for field in ("native_steps", "start_tick", "end_tick", "consumed_j"):
        assert left[field] == right[field], field
    for field in ("model", "view", "direction", "frame", "sprite", "tile", "mask", "track", "speed", "tick", "dx", "dy", "rims", "packed_pixels"):
        assert left["view"][field] == right["view"][field], field
    pairs = []
    # Compare complete pre/post-render and final returned worlds across both
    # independent readers, preserving every native clock/packet/source owner.
    for boundary in ("before", "after", "output"):
        first, second = [row["native"]["output"] if boundary == "output" else row["view"][boundary] for row in (left, right)]
        assert compare_worlds(first, second) == 126
        pairs.append({"boundary": boundary, "left": first, "right": second, "complete_chunks_exact": 126})
    frames = []
    assert len(left["view"]["frames"]) == len(right["view"]["frames"]) == 4
    expected_frames = {(zoom, repeat) for zoom in (1, 2) for repeat in range(2)}
    assert {(row["zoom"], row["repeat"]) for row in left["view"]["frames"]} == expected_frames
    previous = {}
    # Full RGB equality includes the background and company remaps. Both
    # independent builds and both repetitions retain every640x360 pixel.
    for first, second in zip(left["view"]["frames"], right["view"]["frames"]):
        assert (first["zoom"], first["repeat"]) == (second["zoom"], second["repeat"])
        a, b = [Image.open(ROOT / row["file"]["path"]).convert("RGB") for row in (first, second)]
        assert a.size == b.size == (640, 360) and a.tobytes() == b.tobytes()
        zoom = first["zoom"]
        if zoom in previous:
            assert previous[zoom] == a.tobytes()
        previous[zoom] = a.tobytes()
        frames.append({"zoom": zoom, "repeat": first["repeat"], "left": first["file"], "right": second["file"],
                       "rgb_sha256": hashlib.sha256(a.tobytes()).hexdigest(), "pixels_exact": True})
    builds = []
    # Decode original PowerShell diagnostics solely to check target success;
    # neither their bytes nor any prior producer receipt may be rewritten.
    for name in args.build_logs:
        path = ROOT / name
        raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log
        assert "openttd.exe" in log or "openttd_test.exe" in log
        builds.append(identity(path))
    packed = identity(ROOT / "media/baseset/openttd.grf")
    assert packed["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15CC.2; actual useful1975 Crate opposite-reader pixels/picking and complete native purity; closes15C.1 with pinned ordinary file proof",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "complete_world_pairs": pairs, "cross_build_frames": frames,
        "exact_visible_hull_pixels": visible, "exact_occluding_rim_pixels": occluded,
        "exact_identities_verified": len(cache), "builds": builds, "native_version": 434, "packed_art": packed,
        "binaries": [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in configurations for binary in ("openttd", "openttd_test")],
        "file_receipt": identity(receipt), "producer_receipt": history["producer_receipt"],
        "fixture_classification": "Prepared excavated floor, completed original machinery, native generated town and commissioned conventional shafts remain explicit boundary inputs. Actual1975 Forest/Wood/Sawmill production earns39 original Goods, conveyed through conventional feed, layer-local Crate and conventional Surface town delivery. Native tariffs/recurring costs match saved inflation. All five original parts, packets, source industries, town and company history remain native-owned. Rendering earns no work and changes no original owner.",
        "remaining": "Original15.2 retains the three later introduction-date models and Underground/Deep conventional chain; Group16 actual TBM/bootstrap, runtime gallery and human/operator acceptance remain separate.",
    }
    verify_references(result["producer_receipt"], cache)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "complete_world_pairs": len(pairs), "frame_pairs": len(frames)}))


if __name__ == "__main__":
    main()
