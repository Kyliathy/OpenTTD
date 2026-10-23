"""Verify independent paid directional worlds, complete native boundaries and exact body/foreground pixels.

@file tube_travel_sprite_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups13DB/13DCB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
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
from tube_sprite_report import compare_worlds, verify_references


def main():
    """Verify the complete explicit plain or joined model matrix and four builds before publishing an absent closure.

    @return None; stale identities, missing physical views or any original owner/pixel mismatch refuse closure.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs="+", required=True)
    parser.add_argument("--affected-reports", nargs=8, required=True, help="Current geometry/ends/body/service reports in both builds.")
    parser.add_argument("--build-logs", nargs=4, required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--layout", choices=("plain", "joined"), default="plain")
    args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    layout = int(args.layout == "joined")
    models = (10, 13) if layout else tuple(range(9, 14))
    scopes = {(focus, model) for focus in ("write", "raster") for model in models}
    cache, grid, reports = {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    visible = occluded = 0
    # Explicit exact paths retain original identities rather than replacing
    # stale inputs with newly generated lookalikes or stripping work counters.
    for name in args.reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg, focus, model = report["configuration"], report["focus"], report["model"]; key = (cfg, focus, model)
        assert cfg in configurations and (focus, model) in scopes and key not in grid
        assert report.get("layout", 0) == layout
        proof = report["native"]; log = (ROOT / proof["log"]["path"]).read_text(encoding="utf-8")
        counts = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", log)
        assert counts is not None and "NATIVE_EXIT_CODE=0" in log
        assert (int(counts[1]), int(counts[2])) == (proof["assertions"], proof["cases"])
        summary[cfg]["cases"] += proof["cases"]; summary[cfg]["assertions"] += proof["assertions"]
        grid[key] = proof; reports.append({"report": identity(path), **report})
        if focus != "raster":
            continue
        assert proof["native_ticks"] == proof["end_tick"] - proof["start_tick"] > 0 and proof["used_j"] > 0
        assert {view["view"] for view in proof["views"]} == set(range(8))
        if layout:
            crossing = next(view for view in proof["views"] if view["view"] == 3)
            junction = next(view for view in proof["views"] if view["view"] == 2)
            assert (crossing["mask"], crossing["track"]) == (3, 2)
            assert (junction["mask"], junction["track"]) == (5, 4)
        for view in proof["views"]:
            assert view["native_picking"] and view["wrong_layer_refused"] and view["speed"] > 0
            assert compare_worlds(view["before"], view["after"]) == 126
            for pixels in view["packed_pixels"]["matches"]:
                scale = 2 if pixels["zoom"] == 1 else 1
                assert pixels["interior_visible_pixels"] * 2 >= pixels["interior_source_points"] * scale * scale
                assert pixels["visible_pixels"] * 2 >= pixels["exact_pixels"]
                assert pixels["visible_pixels"] + pixels["occluded_by_original_rim_pixels"] == pixels["exact_pixels"]
                visible += pixels["visible_pixels"]; occluded += pixels["occluded_by_original_rim_pixels"]
    assert set(grid) == {(cfg, focus, model) for cfg in configurations for focus, model in scopes}
    affected, affected_grid = [], set()
    # The per-piece bounds invariant and directly affected body/facility
    # contracts accompany the real travel matrix; no unrelated regression
    # suite or stale mechanical fixture may satisfy these current boundaries.
    for name in args.affected_reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg, focus = report["configuration"], report["focus"]
        assert cfg in configurations and focus in ("geometry", "ends", "body", "service") and (cfg, focus) not in affected_grid
        proof = report["native"]; log = (ROOT / proof["log"]["path"]).read_text(encoding="utf-8")
        counts = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", log)
        assert counts is not None and "NATIVE_EXIT_CODE=0" in log
        assert (int(counts[1]), int(counts[2])) == (proof["assertions"], proof["cases"])
        summary[cfg]["cases"] += proof["cases"]; summary[cfg]["assertions"] += proof["assertions"]
        affected_grid.add((cfg, focus)); affected.append({"report": identity(path), **report})
    assert affected_grid == {(cfg, focus) for cfg in configurations for focus in ("geometry", "ends", "body", "service")}
    originals, boundaries, frames, returns = [], [], [], []
    # Independent writers, real native travel and complete returned worlds
    # agree across builds. Capture purity is checked separately at each earned
    # heading; no actual movement between those boundaries is normalized away.
    for model in models:
        writers = [grid[cfg, "write", model] for cfg in configurations]
        assert writers[0]["fields"] == writers[1]["fields"] and compare_worlds(writers[0]["input"], writers[1]["input"]) == 126
        originals.append({"model": model, "left": writers[0]["input"], "right": writers[1]["input"], "complete_chunks_exact": 126})
        readers = [grid[cfg, "raster", model] for cfg in configurations]
        for index, proof in enumerate(readers):
            assert proof["input"] == writers[1 - index]["input"] and proof["fields"] == writers[1 - index]["fields"]
        assert readers[0]["native_ticks"] == readers[1]["native_ticks"] and readers[0]["used_j"] == readers[1]["used_j"]
        assert compare_worlds(readers[0]["final"], readers[1]["final"]) == 126
        returns.append({"model": model, "left": readers[0]["final"], "right": readers[1]["final"], "complete_chunks_exact": 126,
                        "native_ticks": readers[0]["native_ticks"], "used_j": readers[0]["used_j"]})
        for first, second in zip(readers[0]["views"], readers[1]["views"]):
            for field in ("model", "view", "direction", "frame", "sprite", "tile", "speed", "tick", "dx", "dy", "rims"):
                assert first[field] == second[field], (model, field)
            assert first["packed_pixels"] == second["packed_pixels"]
            for boundary in ("before", "after"):
                assert compare_worlds(first[boundary], second[boundary]) == 126
                boundaries.append({"model": model, "view": first["view"], "boundary": boundary,
                                   "left": first[boundary], "right": second[boundary], "complete_chunks_exact": 126})
            assert len(first["frames"]) == len(second["frames"]) == 4
            for a_frame, b_frame in zip(first["frames"], second["frames"]):
                assert (a_frame["zoom"], a_frame["repeat"]) == (b_frame["zoom"], b_frame["repeat"])
                a, b = Image.open(ROOT / a_frame["file"]["path"]).convert("RGB"), Image.open(ROOT / b_frame["file"]["path"]).convert("RGB")
                assert a.size == b.size == (640, 360) and a.tobytes() == b.tobytes()
                frames.append({"model": model, "view": first["view"], "left": a_frame, "right": b_frame,
                               "rgb_sha256": hashlib.sha256(a.tobytes()).hexdigest(), "pixels_exact": True})
    builds = []
    # Preserve original PowerShell build bytes; BOM decoding only verifies
    # success and never rewrites a retained diagnostic or executable identity.
    for name in args.build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and ("openttd.exe" in log or "openttd_test.exe" in log)
        builds.append(identity(path))
    packed = identity(ROOT / "media/baseset/openttd.grf")
    assert packed["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {"_documentation": f"r2mlvs17-underground-transport-tubes.md{'13DCB' if layout else '13DB'}; genuine available2005 model travel, paid original work, native picking and complete original body/foreground pixels",
              "controller": identity(__file__), "summary": summary, "layout": layout,
              "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
              "reports": reports, "affected_reports": affected, "cross_build_originals": originals, "cross_build_render_boundaries": boundaries, "cross_build_frames": frames,
              "cross_build_returns": returns, "exact_visible_hull_pixels": visible, "exact_occluding_rim_pixels": occluded,
              "exact_identities_verified": len(cache), "builds": builds, "packed_art": packed, "native_version": 434,
              "binaries": [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in configurations for binary in ("openttd", "openttd_test")],
              "fixture_classifications": ["Prepared surrounding floor, clearing quota128 and original commissioned portal references remain explicit boundary inputs.",
                                          f"Actual selected models{models} use the disclosed available2005 calendar; introduction-date useful cargo chains remain15.2.",
                                          "Four actual outbound corners earn all eight body facings; normal native order replacement and ticks earn the complete paid return.",
                                          "Each non-company hull pixel is directly exact or covered by an exact original foreground pixel at its actual native location; at least half the source-defined hull interior remains visible. No outline pixel is discarded from coverage.",
                                          "Native correct-layer picking and wrong-layer refusal preserve complete original owner boundaries; no body position/direction or private work counter is assigned."],
              "remaining": "13DCC facility/status interaction and13DC/13D/13.1;13E work files,14 lifecycle,15 useful routes and runtime-gallery/human acceptance remain pending" if layout else
                           "13DC full joins/facility/status interaction and13D/13.1;13E work files,14 lifecycle,15 useful routes and runtime-gallery/human acceptance remain pending"}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "original_pairs": len(originals), "boundary_pairs": len(boundaries), "frame_pairs": len(frames)}))


if __name__ == "__main__":
    main()
