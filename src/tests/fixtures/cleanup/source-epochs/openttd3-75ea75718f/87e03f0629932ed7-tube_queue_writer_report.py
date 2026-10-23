"""Accept actual native queue checkpoint producers without claiming ordinary reload acceptance.

@file tube_queue_writer_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group14EB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Prepared floor/materials/loaded stock remain explicit boundary inputs;14EC owns file consumers.
"""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from geothermal_tube_work_files import compare_chunks
from local_electrical_feed_checks import identity
from tube_sprite_report import verify_references
from tube_queue_writer import FIELDS


def main():
    """Check both original producers, complete native worlds and four successful builds.

    @return None; stale artifacts, any failed native check or different complete worlds prevent closure.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True, help="Exact independent Debug/RelWithDebInfo queue writer reports.")
    parser.add_argument("--build-logs", nargs=4, required=True, help="Final successful game/test build logs.")
    parser.add_argument("--output", required=True, help="Absent immutable receipt beneath repository logs.")
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    cache, writers, records, summary = {}, {}, [], {}
    expected = {(6, 0), (6, 1), (7, 0), (7, 1), (8, 0)}
    # Independent writers must earn both actual path contention and native
    # starvation, then clear both original packets after the same real repair.
    # A passing display/reservation fixture cannot replace these actual files.
    for name in args.reports:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]
        assert cfg in configurations and cfg not in writers and report["phase"] == "write"
        proof = report["native"]
        log = (ROOT / proof["log"]["path"]).read_text(encoding="utf-8")
        counts = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", log)
        assert counts is not None and "NATIVE_EXIT_CODE=0" in log
        assert (int(counts[1]), int(counts[2])) == (proof["assertions"], proof["cases"])
        assert proof["cases"] == 1 and proof["assertions"] > 0
        points = {(point["phase"], point["step"]): point for point in report["checkpoints"]}
        assert len(report["checkpoints"]) == len(points) == 5 and points.keys() == expected
        assert points[6, 1]["tick"] == points[6, 0]["tick"] + 1
        assert points[7, 1]["tick"] == points[7, 0]["tick"] + 1
        assert points[8, 0]["stopped"] == 1 and report["clearing_ticks"] > 0
        queue = report["queue"]
        assert queue["primary"] == queue["winner"] != queue["secondary"]
        assert queue["primary_packet"] != queue["secondary_packet"] and queue["mask"] == 36
        assert (points[6, 0]["vehicle"], points[6, 0]["packet"], points[6, 0]["tick"]) == (queue["winner"], queue["primary_packet"], queue["tick"])
        writers[cfg] = points
        summary[cfg] = {"cases": proof["cases"], "assertions": proof["assertions"]}
        records.append({"report": identity(path), **report})
    assert writers.keys() == set(configurations) and summary["Debug"] == summary["RelWithDebInfo"]
    assert records[0]["queue"] == records[1]["queue"]
    assert records[0]["clearing_ticks"] == records[1]["clearing_ticks"]
    pairs = []
    # Compare every native owner, not just the primary contender's tuple:
    # complete VEHS/CAPA/orders, shared grants, physical map and reservations
    # must agree across the two independently earned original simulations.
    for phase, step in sorted(expected):
        first, second = [writers[cfg][phase, step] for cfg in configurations]
        assert {key: first[key] for key in FIELDS} == {key: second[key] for key in FIELDS}
        assert compare_chunks(ROOT / first["input"]["path"], ROOT / second["input"]["path"], None) == 126
        pairs.append({"phase": phase, "step": step, "left": first["input"], "right": second["input"], "complete_chunks_exact": 126})
    builds = []
    # Decode solely to check successful target diagnostics; retain exact log
    # bytes and identities so a later rebuild cannot overwrite this evidence.
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
        "_documentation": "r2mlvs17-underground-transport-tubes.md14EB; genuine competing junction and original Heat-loss grant-starved producers, followed by actual repair/two arrivals;14EC ordinary readers remain pending",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "records": records, "cross_writer_pairs": pairs, "exact_identities_verified": len(cache),
        "builds": builds, "native_version": 434, "packed_art": packed,
        "binaries": [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in configurations for binary in ("openttd", "openttd_test")],
        "fixture_classification": "Prepared floor/materials and two loaded17-unit packets are boundary inputs. Commands/normal orders/native ticks earn all reservation, movement, grant starvation, repair and arrivals. No relocation or reservation-bit writes.",
        "remaining": "14EC ordinary/repeated/opposite-reader/all-front continuation;15 useful routes, runtime gallery and human/operator gates remain separate.",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "cross_writer_pairs": len(pairs)}))


if __name__ == "__main__":
    main()
