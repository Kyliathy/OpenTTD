"""Verify complete native tube facility art owners, independent writers and exact viewport evidence.

@file tube_facility_sprite_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups13CC/13DCC.
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
    """Verify explicit current scene/unit reports and publish an immutable facility closure.

    @return None; missing scopes, stale identities or any owner/pixel mismatch leave output absent.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs="+", required=True, help="Forty-two exact current native report paths.")
    parser.add_argument("--build-logs", nargs=4, required=True, help="Successful Debug/optimized game/test build logs.")
    parser.add_argument("--output", required=True, help="New closure JSON beneath repository logs.")
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    unit_scopes = ("geometry", "service", "stock", "readiness", "interchange", "guideway", "capsule", "meter", "ui")
    scopes = {(focus, 0) for focus in unit_scopes} | {(focus, scene) for focus in ("write", "raster") for scene in range(6)}
    cache, grid, reports = {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Only explicit exact paths can satisfy the complete matrix. Current
    # source/binary hashes prevent a stale fixture or earlier failed scene from
    # being presented as verification of this final implementation.
    for report_path in args.reports:
        path = ROOT / report_path
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg, focus, scene = report["configuration"], report["focus"], report["scene"]
        key = (cfg, focus, scene)
        assert cfg in configurations and (focus, scene) in scopes and key not in grid
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
            assert [row["rgb_pixels_exact"] for row in proof["packed_pixels"]["matches"]] == [9, 36]
            assert "TUBE_FACILITY_DISPLAY" in log
            assert proof["selection"] == {"native_window": True, "body_hidden": scene != 2, "correct_layer": True, "wrong_layer_refused": True}
    assert set(grid) == {(cfg, focus, scene) for cfg in configurations for focus, scene in scopes}
    originals, boundaries, frames = [], [], []
    # Both independently constructed writer worlds and both opposite readers
    # must retain every complete native chunk. There is no quantity, timer,
    # vehicle, original source, animation or diagnostic normalization here.
    for scene in range(6):
        writers = [grid[cfg, "write", scene] for cfg in configurations]
        assert writers[0]["fields"] == writers[1]["fields"]
        count = compare_worlds(writers[0]["input"], writers[1]["input"])
        originals.append({"scene": scene, "left": writers[0]["input"], "right": writers[1]["input"], "complete_chunks_exact": count})
        readers = [grid[cfg, "raster", scene] for cfg in configurations]
        # Each reader uses the other configuration's genuine writer, rather
        # than silently inspecting its own original or a regenerated copy.
        for index, proof in enumerate(readers):
            assert proof["input"] == writers[1 - index]["input"]
            assert proof["fields"] == writers[1 - index]["fields"]
        for boundary in ("before", "after"):
            count = compare_worlds(readers[0][boundary], readers[1][boundary])
            boundaries.append({"scene": scene, "boundary": boundary, "left": readers[0][boundary], "right": readers[1][boundary], "complete_chunks_exact": count})
        assert len(readers[0]["frames"]) == len(readers[1]["frames"]) == 4
        for first, second in zip(readers[0]["frames"], readers[1]["frames"]):
            a, b = Image.open(ROOT / first["path"]).convert("RGB"), Image.open(ROOT / second["path"]).convert("RGB")
            assert a.size == b.size == (640, 360) and a.tobytes() == b.tobytes()
            frames.append({"scene": scene, "left": first, "right": second, "rgb_sha256": hashlib.sha256(a.tobytes()).hexdigest(), "pixels_exact": True})
    builds = []
    # Preserve native PowerShell UTF16 logs byte-for-byte. Decoding their BOM
    # is only for success checks and never rewrites accepted diagnostics.
    for name in args.build_logs:
        path = ROOT / name
        raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log
        assert "openttd.exe" in log or "openttd_test.exe" in log
        builds.append(identity(path))
    binaries = [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in configurations for binary in ("openttd", "openttd_test")]
    packed = identity(ROOT / "media/baseset/openttd.grf")
    assert packed["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {"_documentation": "r2mlvs17-underground-transport-tubes.md13DCC; actual native depot/platform/interchange geometry, status, source closure and native window/body selection; declared floor/material/stock/wear fixtures are not bootstrap acceptance",
              "controller": identity(__file__), "summary": summary,
              "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
              "reports": reports, "cross_build_originals": originals, "cross_build_render_boundaries": boundaries, "cross_build_frames": frames,
              "exact_status_pixels": sum(row["rgb_pixels_exact"] for proof in grid.values() if "packed_pixels" in proof for row in proof["packed_pixels"]["matches"]),
              "exact_identities_verified": len(cache), "builds": builds, "binaries": binaries, "packed_art": packed, "native_version": 434,
              "additional_sources": [identity(ROOT / name) for name in ("src/rail.cpp", "src/transport_tube.cpp", "src/transport_tube_runtime.cpp", "src/transport_tube_power.cpp",
                                                                       "src/transport_tube_geothermal.cpp", "src/transport_tube_vehicle_sprite.cpp", "src/utility_power.cpp", "src/geothermal_generator.cpp",
                                                                       "src/saveload/afterload.cpp", "src/saveload/transport_tube_vehicle_sl.cpp", "src/saveload/transport_tube_geothermal_sl.cpp",
                                                                       "src/tests/multilayer/tube_sprite_report.py", "src/tests/multilayer/utilities/geothermal_tube_work_files.py")],
              "fixture_classifications": ["Actual default station axes/roles and native depot doors, not synthetic StationGfx or source-only metadata.",
                                          "Canonical original rail labels initialize before UTUB; an empty isolated table otherwise allocates UTUB at nominal ordinary slot0.",
                                          "Native clear/rebuild extends the original compact power yard so an adjacent depot roof does not hide status glass; original selected inlet/path remains32,24.",
                                          "Art captures explicitly disable the optional client reservation overlay, which retains actual native reservation ownership and its later visual scope.",
                                          "Native ANIT/m6 registration owns lamp refresh; rendering never registers animation or advances work. Instant facilities have no unfinished construction phase0.",
                                          "Explicit pending wear, finite-queue pressure and prepared floor/materials remain disclosed boundary inputs; actual native orders/ticks earn loading arrival.",
                                          "Genuine native source closure retires actual external/native offers while retaining original plant, packet and financial history; no visual power flag is edited.",
                                          "Actual native tile clicks open the original depot/station window after capture; hidden depot bodies remain unpickable and exposed loading bodies use the true-layer picker without work."],
              "remaining": "13E authoritative work files,14 lifecycle,15 useful routes; runtime gallery and human feedback pending. Parent13DC/13D/13.1 also require the completed earlier visual stage receipts."}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "original_pairs": len(originals), "boundary_pairs": len(boundaries), "frame_pairs": len(frames)}))


if __name__ == "__main__":
    main()
