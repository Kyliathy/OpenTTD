"""Accept pure original Forge writer worlds and untouched complete next-opportunity baselines.

@file forge_useful_writer_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FEB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Ordinary reader, native raster and paid TBM/bootstrap acceptance retain their separate task gates.
"""
import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from local_electrical_feed_checks import identity
from geothermal_tube_work_files import compare_chunks
from tube_sprite_report import verify_references
from mole_useful_raster_report import count_native
from forge_useful_checks import PREPARE_RECEIPT
from forge_useful_files import PRODUCER_RECEIPT, validate_points


def main():
    """Verify both independent original-world writers, unchanged actual route boundaries and current test builds.

    @return None; unequal original owners, changed files, incomplete next opportunities or stale current references prevent acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--writers", nargs=2, required=True)
    parser.add_argument("--routes", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    preparation, producer = ROOT / PREPARE_RECEIPT, ROOT / PRODUCER_RECEIPT
    assert identity(preparation)["sha256"] == "a05932f139fe4d4150ece5c79709890c6785a374254bda1b60df430735e64992"
    assert identity(producer)["sha256"] == "351772ad22fc7af32fcdb8414375deef15f0c7cd6618696aedbd428336cfd577"
    accepted = json.loads(preparation.read_text(encoding="utf-8")); history = json.loads(producer.read_text(encoding="utf-8"))
    prepared = {row["configuration"]: row for row in accepted["records"] if row.get("focus") == "route-prepare"}
    configurations = ("Debug", "RelWithDebInfo"); cache, writers, routes, records = {}, {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Current writer/route records verify their actual source/log/binary/file
    # identities. Sealed preparation/useful receipts remain opaque history;
    # their earlier nested binaries are not substituted for current targets.
    for phase, names, selected in (("write", args.writers, writers), ("route", args.routes, routes)):
        for name in names:
            path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
            cfg = report["configuration"]; assert cfg in configurations and cfg not in selected
            opposite = configurations[1 - configurations.index(cfg)]; original = prepared[opposite]
            assert report["prepare_report"] == original["report"] and report["prepare_receipt"] == identity(preparation)
            assert report["native"]["input"] == original["native"]["output"]
            if phase == "write":
                assert report["phase"] == "write" and report["producer_receipt"] == identity(producer)
                validate_points(report["checkpoints"], original, next(row for row in history["records"] if row["configuration"] == cfg and row.get("focus") == "forge-useful"))
                assert report["historical_final_complete_chunks_exact"] == 126
            else:
                assert report["focus"] == "forge-useful" and report["result"] == history["result"]
                previous = next(row for row in history["records"] if row["configuration"] == cfg and row.get("focus") == "forge-useful")
                assert report["scenes"] == previous["scenes"] and report["returned"] == previous["returned"] and report["materials"] == previous["materials"]
            count_native(report["native"], summary[cfg]); selected[cfg] = report
            records.append({"report": identity(path), **report})
    assert writers.keys() == routes.keys() == set(configurations) and summary["Debug"] == summary["RelWithDebInfo"]
    worlds = []
    grids = {cfg: {(row["scene"], row["step"]): row for row in writers[cfg]["checkpoints"]} for cfg in configurations}
    # All twelve original/baseline cross-build worlds compare every complete
    # native chunk. No map, account, packet, catalogue or diagnostic owner is
    # excluded merely because named scalar snapshots already agree.
    for scene in range(6):
        for step in range(2):
            left, right = (grids[cfg][scene, step] for cfg in configurations)
            assert left["snapshot"] == right["snapshot"] and left["parts"] == right["parts"] and left["references"] == right["references"]
            assert compare_chunks(ROOT / left["input"]["path"], ROOT / right["input"]["path"], None) == 126
            worlds.append({"scene": scene, "step": step, "left": left["input"], "right": right["input"], "complete_chunks_exact": 126})
    route_pairs = []
    # The unchanged route-only run must finish at precisely the writer's
    # original final state; writing earlier scenes cannot alter later work.
    for cfg in configurations:
        left, right = routes[cfg]["native"]["output"], grids[cfg][5, 0]["input"]
        assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
        route_pairs.append({"configuration": cfg, "route": left, "writer_final": right, "complete_chunks_exact": 126})
    builds = []
    for name in args.build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log; builds.append(identity(path))
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15FEB; actual six-state pure native writer and untouched next-opportunity baseline acceptance; ordinary readers/raster remain pending",
        "controller": identity(__file__), "summary": summary, "records": records,
        "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
        "complete_cross_build_world_pairs": worlds, "current_route_final_pairs": route_pairs,
        "prepare_receipt": identity(preparation), "producer_receipt": identity(producer), "builds": builds,
        "test_binaries": [identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        "exact_identities_verified": len(cache), "native_version": 434,
        "fixture_classification": writers["Debug"]["fixture_classification"],
        "remaining": "15FEC ordinary full-app read/preview/repeat/opposite-reader/complete-next continuation precedes15FEE moving model13 raster. Parent15F.1/original15.2/master10.7 and actual TBM/bootstrap/human/gallery/operator gates remain pending.",
    }
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "cross_build_pairs": len(worlds), "current_route_final_pairs": len(route_pairs)}))


if __name__ == "__main__":
    main()
