"""Accept pure loaded tube cache rebuilding from original native work owners.

@file tube_cache_file_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group14B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Prepared retained work scenes remain distinct from useful freight/bootstrap acceptance.
"""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from geothermal_tube_work_files import compare_chunks, compare_next_step
from local_electrical_feed_checks import identity
from tube_sprite_report import verify_references
from tube_work_file_report import OWNERS, retained_writers


def main():
    """Verify all six current cold/warm scenes, affected native continuation and builds.

    @return None; stale identities, any changed owner or missing source admission aborts acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=12, required=True, help="Six opposite-writer cache reports per configuration.")
    parser.add_argument("--affected-reports", nargs=2, required=True, help="Current opposite-writer step0 report from each configuration.")
    parser.add_argument("--build-logs", nargs=4, required=True, help="Four successful final game/test build logs.")
    parser.add_argument("--output", required=True, help="Absent new closure JSON beneath repository logs.")
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    writers = retained_writers()
    cache, grid, affected, records = {}, {}, set(), []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Explicit opposite-writer files retain original consumed work and native
    # physics. Neither an empty fixture nor an all-false cache can satisfy the
    # live external source and genuine geothermal cable-loss checks.
    for name in (*args.reports, *args.affected_reports):
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg, phase, case = report["configuration"], report["phase"], report["case"]
        assert cfg in configurations
        opposite = configurations[1 - configurations.index(cfg)]
        writer = writers[opposite]
        assert report["writer"] == writer["report"]
        points = {point["step"]: point for point in writer["checkpoints"] if point["phase"] == case}
        assert report["checkpoint"] == points[0]
        for process in report["native"]:
            log = (ROOT / process["log"]["path"]).read_text(encoding="utf-8")
            count = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", log)
            assert count is not None and "NATIVE_EXIT_CODE=0" in log
            assert (int(count[1]), int(count[2])) == (process["assertions"], process["cases"])
            summary[cfg]["cases"] += process["cases"]
            summary[cfg]["assertions"] += process["assertions"]
        if phase == "cache":
            assert case in range(6) and (cfg, case) not in grid and len(report["native"]) == 1
            proof = report["native"][0]
            assert proof["cache_rebuilds"] == 3
            assert proof["original_external_sources"] == proof["original_native_sources"] == 1
            assert proof["external_connected"] is True
            assert proof["native_connected"] == (case != 3)
            assert compare_chunks(ROOT / proof["cache_before"]["path"], ROOT / proof["output"]["path"], None) == 126
            assert compare_chunks(ROOT / proof["input"]["path"], ROOT / proof["output"]["path"], OWNERS) == len(OWNERS)
            grid[cfg, case] = proof
        else:
            assert phase == "step" and case == 0 and cfg not in affected and len(report["native"]) == 2
            baseline, resumed = [ROOT / process["output"]["path"] for process in report["native"]]
            assert report["next_step"] == compare_next_step(baseline, resumed, points[0]["tick"], points[1]["tick"])
            assert compare_chunks(baseline, resumed, OWNERS) == len(OWNERS)
            affected.add(cfg)
        records.append({"report": identity(path), **report})
    assert set(grid) == {(cfg, case) for cfg in configurations for case in range(6)}
    assert affected == set(configurations)
    pairs = []
    # Both current readers must derive the same original physical component
    # without creating a saved cache or changing any complete native owner.
    for case in range(6):
        for boundary in ("cache_before", "output"):
            first, second = [grid[cfg, case][boundary] for cfg in configurations]
            assert compare_chunks(ROOT / first["path"], ROOT / second["path"], None) == 126
            pairs.append({"case": case, "boundary": boundary, "left": first, "right": second, "complete_chunks_exact": 126})
    builds = []
    # Native PowerShell build logs retain their original bytes. BOM decoding
    # merely verifies success; it cannot rewrite accepted build diagnostics.
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
        "_documentation": "r2mlvs17-underground-transport-tubes.md14B; accepted original owners precede repeated pure loaded connectivity derivation",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "cross_reader_pairs": pairs, "exact_identities_verified": len(cache),
        "cache_resets": 36, "warm_lookup_rounds": 36, "complete_owner_chunks": sorted(name.decode() for name in OWNERS),
        "builds": builds,
        "binaries": [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in configurations for binary in ("openttd", "openttd_test")],
        "native_version": 434, "packed_art": packed,
        "authoritative_file_receipt": identity(ROOT / "logs/tubes/work-state-files/20261018-native-01/verification.json"),
        "owner_sources": [identity(ROOT / name) for name in (
            "src/saveload/afterload.cpp", "src/transport_tube_power.cpp", "src/transport_tube_geothermal.cpp",
            "src/transport_tube_runtime.cpp", "src/transport_tube_topology.cpp", "src/transport_tube_conduit_sprite.cpp",
            "src/tests/transport_tube_geothermal_work_load.cpp", "src/tests/multilayer/utilities/geothermal_tube_work_files.py",
            "src/tests/multilayer/tube_work_file_report.py")],
        "load_order": "Native vehicle/orders/packets/map labels and physical facilities resolve first; shared utility, original source families and saved capsule bindings accept before cold/warm circuit queries. No native work opportunity occurs inside the cache boundary.",
        "fixture_classification": "Six genuine retained paid-work/switch/wait/service files retain prepared floor/materials and loaded17-unit packet as declared inputs; not useful-route/bootstrap or host/client acceptance.",
        "remaining": "14.2 topology invalidation,14.3 ownership,14.4 queued/starved continuation,15 useful introduction-date routes; gallery and human gates remain separate.",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "cross_reader_pairs": len(pairs)}))


if __name__ == "__main__":
    main()
