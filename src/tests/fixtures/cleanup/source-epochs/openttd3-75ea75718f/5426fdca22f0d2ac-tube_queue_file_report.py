"""Accept complete native queued/starved file transport and all-front continuation.

@file tube_queue_file_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group14EC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Historical producers retain dated binaries; current readers must preserve every original quantity and participant.
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

PRODUCER = "logs/tubes/queue-files/20261018-native-01/producer-verification.json"


def queue_writers():
    """Verify immutable queue producers and inputs while preserving their dated binary identities.

    @return Both accepted14EB writer records, each containing five actual native worlds.
    """
    receipt = ROOT / PRODUCER
    assert identity(receipt)["sha256"] == "eee60abfb16c65434154a5ffa225580c199ac1a15e5bdcd1f2456b1527d6edc1"
    original = json.loads(receipt.read_text(encoding="utf-8"))
    writers = {}
    # Current reader instrumentation changes the test binary. Only immutable
    # producer reports/logs/inputs are rechecked against disk; historical
    # binary identities remain evidence of their actual original invocation.
    for record in original["records"]:
        assert identity(ROOT / record["report"]["path"]) == record["report"]
        assert identity(ROOT / record["native"]["log"]["path"]) == record["native"]["log"]
        assert len(record["checkpoints"]) == 5
        for point in record["checkpoints"]:
            assert identity(ROOT / point["input"]["path"]) == point["input"]
        writers[record["configuration"]] = record
    assert writers.keys() == {"Debug", "RelWithDebInfo"}
    return writers


def main():
    """Verify ordinary repeat/all-front evidence, original cable waits and final native builds.

    @return None; any stale identity, missing participant or changed complete owner prevents closure.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=10, required=True, help="Opposite-writer reload6/7/8 and step6/7 in each build.")
    parser.add_argument("--affected-reports", nargs=4, required=True, help="Original cable-wait reload3/step3 in each current build.")
    parser.add_argument("--build-logs", nargs=4, required=True)
    parser.add_argument("--output", required=True, help="Absent immutable closure JSON under logs.")
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    writers, cable = queue_writers(), retained_writers()
    cache, grid, affected, records = {}, {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Every queue scene and both distinct shortage causes must pass their own
    # ordinary application processes. The primary tuple cannot stand in for
    # the secondary packet, native reservation map or full serialized state.
    for name in (*args.reports, *args.affected_reports):
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg, phase, case = report["configuration"], report["phase"], report["case"]
        assert cfg in configurations and phase in ("reload", "step")
        opposite = configurations[1 - configurations.index(cfg)]
        is_queue = case in (6, 7, 8)
        writer = writers[opposite] if is_queue else cable[opposite]
        assert report["writer"] == writer["report"]
        points = {point["step"]: point for point in writer["checkpoints"] if point["phase"] == case}
        assert report["checkpoint"] == points[0] and len(report["native"]) == 2
        first, second = [ROOT / process["output"]["path"] for process in report["native"]]
        if phase == "reload":
            assert compare_chunks(ROOT / points[0]["input"]["path"], first, OWNERS) == len(OWNERS)
            assert compare_chunks(first, second, None) == 126
        else:
            assert report["uninterrupted_baseline"] == points[1]["input"]
            assert report["next_step"] == compare_next_step(first, second, points[0]["tick"], points[1]["tick"])
            assert compare_chunks(first, second, OWNERS) == len(OWNERS)
        for process in report["native"]:
            log = (ROOT / process["log"]["path"]).read_text(encoding="utf-8")
            count = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", log)
            assert count is not None and "NATIVE_EXIT_CODE=0" in log
            assert (int(count[1]), int(count[2])) == (process["assertions"], process["cases"])
            summary[cfg]["cases"] += process["cases"]
            summary[cfg]["assertions"] += process["assertions"]
        if is_queue:
            assert (cfg, phase, case) not in grid and report["queue"] == writer["queue"]
            for process in report["native"]:
                assert process["participants"] == {key: writer["queue"][key] for key in ("primary", "secondary", "primary_packet", "secondary_packet")}
            if phase == "step":
                resumed = report["native"][1]
                assert resumed["action"] == "step-all" and resumed["fronts_advanced"] == 2
                assert resumed["common_opportunities"] == 1 and resumed["continued_tick"] == points[1]["tick"]
            grid[cfg, phase, case] = report
        else:
            assert case == 3 and (cfg, phase) not in affected
            affected[cfg, phase] = report
        records.append({"report": identity(path), **report})
    assert grid.keys() == {(cfg, phase, case) for cfg in configurations for phase, case in (("reload", 6), ("reload", 7), ("reload", 8), ("step", 6), ("step", 7))}
    assert affected.keys() == {(cfg, phase) for cfg in configurations for phase in ("reload", "step")}
    assert summary["Debug"] == summary["RelWithDebInfo"]
    pairs = []
    # Independent current readers must produce identical complete worlds at
    # every first/repeated and baseline/continued boundary, without erasing
    # either participant or normalizing reservation/grant/position fields.
    for phase, case in (("reload", 6), ("reload", 7), ("reload", 8), ("step", 6), ("step", 7), ("reload", 3), ("step", 3)):
        for index in range(2):
            reports = [affected[cfg, phase] if case == 3 else grid[cfg, phase, case] for cfg in configurations]
            left, right = [report["native"][index]["output"] for report in reports]
            assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
            pairs.append({"phase": phase, "case": case, "boundary": index, "left": left, "right": right, "complete_chunks_exact": 126})
    builds = []
    # Preserve byte-exact diagnostics; BOM decoding is only a success check,
    # never a rewrite of current or historical build evidence.
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
        "_documentation": "r2mlvs17-underground-transport-tubes.md14EC/14E.1/original14.4; complete actual queued/grant-starved repeated and all-front native files plus retained original cable-topology wait",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "cross_reader_pairs": pairs, "exact_identities_verified": len(cache),
        "producer_receipt": identity(ROOT / PRODUCER), "complete_owner_chunks": sorted(name.decode() for name in OWNERS),
        "builds": builds, "native_version": 434, "packed_art": packed,
        "binaries": [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in configurations for binary in ("openttd", "openttd_test")],
        "continuation_contract": "Every original bound front is reacquired from the ordinary loaded native pool and checked for distinct original packet custody. One common allocation precedes both native ticks in pool order. Complete125 chunks and independently precise GLOG Load-event timing match the untouched uninterrupted baseline; all126 cross-reader outputs match.",
        "fixture_classification": "Prepared14EB floor/materials/loaded stock are boundary inputs, not useful-route/bootstrap acceptance. Original Heat-loss Grant waiting and earlier cable-loss Topology waiting remain distinct actual causes.",
        "remaining": "15 useful introduction-date routes, selected runtime gallery and human/operator gates remain separate.",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "cross_reader_pairs": len(pairs)}))


if __name__ == "__main__":
    main()
