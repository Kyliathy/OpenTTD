"""Accept ordinary useful Crate files and exact full-native continuation before runtime art.

@file crate_useful_file_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md task15CC.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The three later models, Underground/Deep chain, runtime silhouette and actual TBM/bootstrap remain separate.
"""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from local_electrical_feed_checks import identity
from geothermal_tube_work_files import compare_chunks, compare_next_step
from tube_sprite_report import verify_references
from crate_useful_files import OWNERS, PRODUCER_RECEIPT

FIELDS = ("scene", "step", "tick", "parts", "references", "snapshot")


def main():
    """Require independent writers, every opposite-reader boundary and the original native driver.

    @return None; any stale artifact, missing owner or changed complete continuation prevents file acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=26, required=True, help="Two exact writers and six reload/step scenes in each opposite reader.")
    parser.add_argument("--affected-reports", nargs=6, required=True, help="Current original Crate route, Goods source and affected Mole route in both builds.")
    parser.add_argument("--build-logs", nargs=4, required=True)
    parser.add_argument("--output", required=True, help="Absent immutable receipt under logs.")
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    receipt = ROOT / PRODUCER_RECEIPT
    assert identity(receipt)["sha256"] == "1d58ae27a4a7ef37cb2d2564e791c6f813d82e0a8e53bdd4ced89ad7cf3c424f"
    cache, writers, readers, affected, records = {}, {}, {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # The genuine full-native producer and every original participant remain
    # mandatory. Same-build characterization and the older raw-front driver
    # cannot replace opposite-writer ordinary application continuation.
    for name in (*args.reports, *args.affected_reports):
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in configurations
        if "phase" not in report:
            focus = "crate" if "source_receipt" in report else "source" if "source" in report else report["focus"]
            assert focus in ("crate", "source", "mole") and (cfg, focus) not in affected
            affected[cfg, focus] = report
            processes = [report["native"]]
        elif report["phase"] == "write":
            assert cfg not in writers
            assert len(report["checkpoints"]) == 12
            assert report["producer_receipt"] == identity(receipt)
            writers[cfg] = {"report": identity(path), **report}
            processes = [report["native"]]
        else:
            phase, scene = report["phase"], report["case"]
            assert phase in ("reload", "step") and scene in range(6) and (cfg, phase, scene) not in readers
            assert len(report["native"]) == 2
            readers[cfg, phase, scene] = report
            processes = report["native"]
        for process in processes:
            log = (ROOT / process["log"]["path"]).read_text(encoding="utf-8")
            count = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", log)
            assert count is not None and "NATIVE_EXIT_CODE=0" in log
            assert (int(count[1]), int(count[2])) == (process["assertions"], process["cases"])
            summary[cfg]["cases"] += process["cases"]
            summary[cfg]["assertions"] += process["assertions"]
        records.append({"report": identity(path), **report})
    assert writers.keys() == set(configurations)
    assert affected.keys() == {(cfg, focus) for cfg in configurations for focus in ("crate", "source", "mole")}
    assert readers.keys() == {(cfg, phase, scene) for cfg in configurations for phase in ("reload", "step") for scene in range(6)}
    assert summary["Debug"] == summary["RelWithDebInfo"]
    for focus in ("crate", "source", "mole"):
        field = "source" if focus == "source" else "route"
        assert affected["Debug", focus][field] == affected["RelWithDebInfo", focus][field]
    pairs = []
    # Both original simulations must earn identical current worlds. Compare
    # all126 chunks, not just the currently loaded vehicle or money tuple.
    for scene in range(6):
        for step in range(2):
            left, right = [next(point for point in writers[cfg]["checkpoints"] if (point["scene"], point["step"]) == (scene, step)) for cfg in configurations]
            assert {key: left[key] for key in FIELDS} == {key: right[key] for key in FIELDS}
            assert compare_chunks(ROOT / left["input"]["path"], ROOT / right["input"]["path"], None) == 126
            pairs.append({"kind": "writer", "scene": scene, "boundary": step, "left": left["input"], "right": right["input"], "complete_chunks_exact": 126})
    # Tie every consumer to its exact opposite producer and original full
    # baseline, preventing same-build characterization from earning closure.
    for (cfg, phase, scene), report in readers.items():
        opposite = configurations[1 - configurations.index(cfg)]
        writer = writers[opposite]
        assert report["writer"] == writer["report"]
        points = {point["step"]: point for point in writer["checkpoints"] if point["scene"] == scene}
        assert report["checkpoint"] == points[0]
        first, second = [ROOT / process["output"]["path"] for process in report["native"]]
        if phase == "reload":
            assert all(process["observed_snapshot"] == points[0]["snapshot"] for process in report["native"])
            assert compare_chunks(ROOT / points[0]["input"]["path"], first, OWNERS) == len(OWNERS)
            assert compare_chunks(first, second, None) == 126
        else:
            assert all(process["observed_snapshot"] == points[1]["snapshot"] for process in report["native"])
            assert report["uninterrupted_baseline"] == points[1]["input"]
            assert report["next_step"] == compare_next_step(first, second, points[0]["tick"], points[1]["tick"])
            assert compare_chunks(first, second, OWNERS) == len(OWNERS)
    # Every first/repeated and baseline/continued output must also agree in
    # both independent current readers, including complete clocks/positions,
    # packet age, native town/industry/quarter records, income, grants and reservations.
    for phase in ("reload", "step"):
        for scene in range(6):
            for index in range(2):
                left, right = [readers[cfg, phase, scene]["native"][index]["output"] for cfg in configurations]
                assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
                pairs.append({"kind": phase, "scene": scene, "boundary": index, "left": left, "right": right, "complete_chunks_exact": 126})
    builds = []
    # Decode solely for target success checks while retaining byte-exact
    # PowerShell diagnostics and immutable evidence identities.
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
        "_documentation": "r2mlvs17-underground-transport-tubes.md15CC.1; actual useful Crate ordinary files and full-native continuation; runtime art15CC.2 and other models remain pending",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "complete_world_pairs": pairs, "exact_identities_verified": len(cache),
        "complete_owner_chunks": sorted(name.decode() for name in OWNERS),
        "builds": builds, "native_version": 434, "packed_art": packed,
        "binaries": [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in configurations for binary in ("openttd", "openttd_test")],
        "producer_receipt": identity(receipt),
        "continuation_contract": "All six actual loaded modal/final1975 worlds preserve five original parts, three fronts, every native split packet, source industries, town and current/historical native accounts through ordinary loading. Shared full-native timer/vehicle code advances one complete opportunity, including clock, daily work, loading, aging, common power and deferred restart. All125 chunks plus independently precise GLOG load-event timing match uninterrupted originals; repeat and cross-reader outputs retain all126.",
        "fixture_classification": "Prepared floor, original machinery, generated native town and commissioned conventional shafts remain explicit boundaries. Original Forest production and physically driven Wood delivery earn39 Sawmill Goods. Native cargo properties, full definition mappings, valid face/tile-loop identities and actual derived tariffs/recurring costs precede original observations; readers perform no repair. Historical diagnostic inputs remain immutable and excluded.",
        "remaining": "15CC.2 actual runtime Crate pixels/picking; three later model/date routes, Underground/Deep conventional chain, actual TBM/bootstrap and human/operator gates remain separate.",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "complete_world_pairs": len(pairs)}))


if __name__ == "__main__":
    main()
