"""Verify exact current native conduit owners, independent files and original ground pixels.

@file tube_conduit_sprite_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13CD.
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
    """Verify explicit current reports and publish an absent immutable closure beneath logs.

    @return None; missing/stale scopes or any complete owner/pixel mismatch refuse closure.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs="+", required=True)
    parser.add_argument("--build-logs", nargs=4, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    scopes = {(focus, 0) for focus in ("path", "purity", "native", "guideway", "facility", "meter")} | {
        (focus, scene) for focus in ("write", "raster") for scene in range(3)}
    cache, grid, reports = {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Explicit original report paths and current source/binary identities stop
    # old successes or substituted inputs from satisfying this final matrix.
    for name in args.reports:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg, focus, scene = report["configuration"], report["focus"], report["scene"]
        key = (cfg, focus, scene)
        assert cfg in configurations and (focus, scene) in scopes and key not in grid
        proof = report["native"]
        log = (ROOT / proof["log"]["path"]).read_text(encoding="utf-8")
        counts = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", log)
        assert counts is not None and "NATIVE_EXIT_CODE=0" in log
        assert (int(counts[1]), int(counts[2])) == (proof["assertions"], proof["cases"])
        summary[cfg]["cases"] += proof["cases"]; summary[cfg]["assertions"] += proof["assertions"]
        grid[key] = proof; reports.append({"report": identity(path), **report})
        if focus == "raster":
            assert compare_worlds(proof["before"], proof["after"]) == 126
    assert set(grid) == {(cfg, focus, scene) for cfg in configurations for focus, scene in scopes}
    originals, boundaries, frames = [], [], []
    # Independent writer worlds and both opposite readers retain all chunks;
    # no counter, map bit, source, packet, timer or RGB byte is normalized.
    for scene in range(3):
        writers = [grid[cfg, "write", scene] for cfg in configurations]
        assert writers[0]["fields"] == writers[1]["fields"]
        assert compare_worlds(writers[0]["input"], writers[1]["input"]) == 126
        originals.append({"scene": scene, "left": writers[0]["input"], "right": writers[1]["input"], "complete_chunks_exact": 126})
        readers = [grid[cfg, "raster", scene] for cfg in configurations]
        for index, proof in enumerate(readers):
            assert proof["input"] == writers[1 - index]["input"] and proof["fields"] == writers[1 - index]["fields"]
        for boundary in ("before", "after"):
            assert compare_worlds(readers[0][boundary], readers[1][boundary]) == 126
            boundaries.append({"scene": scene, "boundary": boundary, "left": readers[0][boundary], "right": readers[1][boundary], "complete_chunks_exact": 126})
        assert len(readers[0]["frames"]) == len(readers[1]["frames"]) == 12
        for first, second in zip(readers[0]["frames"], readers[1]["frames"]):
            assert (first["y"], first["zoom"], first["repeat"]) == (second["y"], second["zoom"], second["repeat"])
            a, b = Image.open(ROOT / first["file"]["path"]).convert("RGB"), Image.open(ROOT / second["file"]["path"]).convert("RGB")
            assert a.size == b.size == (640, 360) and a.tobytes() == b.tobytes()
            frames.append({"scene": scene, "left": first, "right": second, "rgb_sha256": hashlib.sha256(a.tobytes()).hexdigest(), "pixels_exact": True})
    builds = []
    # Only decode PowerShell BOMs for verification; retained diagnostic bytes
    # remain immutable and are hashed exactly as the build produced them.
    for name in args.build_logs:
        path = ROOT / name; raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log
        assert "openttd.exe" in log or "openttd_test.exe" in log
        builds.append(identity(path))
    packed = identity(ROOT / "media/baseset/openttd.grf")
    assert packed["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {"_documentation": "r2mlvs17-underground-transport-tubes.md13CD; original paid external path ground branches, not a new electrical or routing owner",
              "controller": identity(__file__), "summary": summary,
              "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
              "reports": reports, "cross_build_originals": originals, "cross_build_render_boundaries": boundaries, "cross_build_frames": frames,
              "exact_source_pixels": sum(row["pixels"] for proof in grid.values() if "packed_pixels" in proof for row in proof["packed_pixels"]["checks"]),
              "exact_identities_verified": len(cache), "builds": builds, "packed_art": packed, "native_version": 434,
              "binaries": [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in configurations for binary in ("openttd", "openttd_test")],
              "fixture_classifications": ["Prepared floor/materials and commissioned portal records are explicit inherited boundary inputs, not bootstrap acceptance.",
                                          "Real native commands purchase a replacement external corridor; publication and closure occur before pure capture.",
                                          "Original native geothermal links retain geothermal.conduit ownership; no extra wire is inferred from a plant or inlet.",
                                          "Dark/published/retired scenes retain original native plant, packet17, source/reference digests and simulation tick."],
              "remaining": "13D full directional raster/picking,13E work files,14 lifecycle,15 useful routes; Plan14 native geothermal utility/gallery and human acceptance retain their scopes"}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "original_pairs": len(originals), "boundary_pairs": len(boundaries), "frame_pairs": len(frames)}))


if __name__ == "__main__":
    main()
