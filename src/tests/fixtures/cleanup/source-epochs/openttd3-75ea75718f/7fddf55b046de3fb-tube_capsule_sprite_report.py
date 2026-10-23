"""Verify exact capsule art owners, cross-build frames and focused native results.

@file tube_capsule_sprite_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13CB.
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
    """Verify explicit current reports and publish an immutable, reproducible capsule closure.

    @return None; any stale identity, incomplete matrix or owner/pixel mismatch leaves output absent.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs="+", required=True, help="Fourteen exact current native report paths.")
    parser.add_argument("--build-logs", nargs=4, required=True, help="Successful Debug/optimized game/test build logs.")
    parser.add_argument("--output", required=True, help="Absent closure JSON beneath repository logs.")
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    scopes = {(name, 0) for name in ("body", "purchase", "prior-body", "prior-purchase", "meter", "raster")} | {("raster", 3)}
    cache, grid, reports = {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Explicit report paths and current hashes prevent a prior failed run,
    # changed observer or stale binary from satisfying the current closure.
    for report_path in args.reports:
        path = ROOT / report_path
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg, focus, case = report["configuration"], report["focus"], report["case"]
        key = (cfg, focus, case)
        assert cfg in configurations and (focus, case) in scopes and key not in grid
        proof = report["native"]
        log = (ROOT / proof["log"]["path"]).read_text(encoding="utf-8")
        counts = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", log)
        assert counts is not None and "NATIVE_EXIT_CODE=0" in log
        assert (int(counts[1]), int(counts[2])) == (proof["assertions"], proof["cases"])
        summary[cfg]["cases"] += proof["cases"]
        summary[cfg]["assertions"] += proof["assertions"]
        grid[key] = proof
        reports.append({"report": identity(path), **report})
        if focus == "raster":
            assert compare_worlds(proof["before"], proof["after"]) == 126
            assert proof["cache_exact"]
            assert sum(row["rgb_pixels_exact"] for row in proof["packed_pixels"]["matches"]) > 100
    assert set(grid) == {(cfg, focus, case) for cfg in configurations for focus, case in scopes}
    world_pairs, frame_pairs = [], []
    # Opposite original writers feed the two reader builds. Neither source
    # control/travel setup nor any complete saved owner can be normalized.
    for case in (0, 3):
        left, right = grid["Debug", "raster", case], grid["RelWithDebInfo", "raster", case]
        assert left["capsule"] == right["capsule"]
        assert left["native_travel_ticks_before_render_boundary"] == right["native_travel_ticks_before_render_boundary"]
        assert left["writer"]["path"] != right["writer"]["path"]
        for boundary in ("before", "after"):
            count = compare_worlds(left[boundary], right[boundary])
            world_pairs.append({"case": case, "boundary": boundary, "left": left[boundary], "right": right[boundary], "complete_chunks_exact": count})
        assert len(left["frames"]) == len(right["frames"]) == 4
        for first, second in zip(left["frames"], right["frames"]):
            a, b = Image.open(ROOT / first["path"]).convert("RGB"), Image.open(ROOT / second["path"]).convert("RGB")
            assert a.size == b.size == (640, 360) and a.tobytes() == b.tobytes()
            frame_pairs.append({"case": case, "left": first, "right": second, "rgb_sha256": hashlib.sha256(a.tobytes()).hexdigest(), "pixels_exact": True})
    builds = []
    # Build logs must be actual successful targets, not empty launch records.
    # Native reports independently bind the final test executable identities.
    for name in args.build_logs:
        path = ROOT / name
        # Windows PowerShell retains native redirection as UTF16 with a BOM;
        # decode that declared envelope while hashing the original bytes intact.
        raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log
        assert "openttd.exe" in log or "openttd_test.exe" in log
        builds.append(identity(path))
    binaries = [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in configurations for binary in ("openttd", "openttd_test")]
    packed = identity(ROOT / "media/baseset/openttd.grf")
    assert packed["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {"_documentation": "r2mlvs17-underground-transport-tubes.md13CB; actual five-model body/purchase rendering; fixture cargo/floor and recovery/travel are declared native boundaries",
              "controller": identity(__file__), "summary": summary,
              "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
              "body_poses_per_configuration": 720, "purchase_poses_per_configuration": 10,
              "reports": reports, "cross_build_worlds": world_pairs, "cross_build_frames": frame_pairs,
              "exact_authored_body_pixels": sum(row["rgb_pixels_exact"] for proof in grid.values() if "packed_pixels" in proof for row in proof["packed_pixels"]["matches"]),
              "exact_identities_verified": len(cache), "builds": builds, "binaries": binaries, "packed_art": packed, "native_version": 434,
              "helpers": [identity(ROOT / name) for name in ("src/tests/multilayer/tube_sprite_report.py", "src/tests/multilayer/utilities/geothermal_tube_work_files.py",
                                                           "src/tests/multilayer/utilities/geothermal_tube_binding_files.py", "src/tests/multilayer/utilities/local_electrical_feed_checks.py")],
              "observed_fixture_failures": [{"input": identity(ROOT / "logs/tubes/capsule-art/20261017-221913-a742f8b3cd/body.log"),
                                             "reason": "Lightweight tube fixtures needed existing mock sprite headers after native art enabled viewport bounds; no mock pixels count as raster evidence."},
                                            {"input": identity(ROOT / "logs/tubes/capsule-art/20261017-222425-a3910f67b3/capsule-raster-0.log"),
                                             "reason": "Original saved pause correctly prevented source work; declared native travel must explicitly unpause before pure capture."}],
              "remaining": "13CC facilities,13CD conduits,13D complete raster/picking/directional acceptance,13E files,14 lifecycle,15 useful routes; runtime gallery and human feedback pending"}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "world_pairs": len(world_pairs), "frame_pairs": len(frame_pairs)}))


if __name__ == "__main__":
    main()
