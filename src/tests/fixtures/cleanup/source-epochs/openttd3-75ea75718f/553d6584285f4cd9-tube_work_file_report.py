"""Accept original capsule orders, physical state and power through current native readers.

@file tube_work_file_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13E.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Retained paid-work fixtures disclose prepared floor and loaded stock; these files
do not establish useful introduction-date freight routes or host/client acceptance.
"""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from geothermal_tube_binding_files import AUTHORITY, world_chunks
from geothermal_tube_work_files import compare_chunks, compare_next_step
from local_electrical_feed_checks import identity
from tube_sprite_report import verify_references

# Native vehicle transport includes actual tile/coordinates, direction, motion,
# service and current order. ORDL owns the ordinary destination list; CAPA owns
# the original packet. Map/portal/station/depot transports retain real network
# identities independently of rebuilt display and pathfinder caches.
OWNERS = AUTHORITY | {
    b"VEHS", b"ORDL", b"CAPA", b"STNN", b"MLMD", b"LPWH", b"LPWK",
    b"TUPH", b"TUPD", b"MAPS", b"MAPT", b"MAPH", b"MAPO", b"MAP2",
    b"M3LO", b"M3HI", b"MAP5", b"MAPE", b"MAP7",
}
HISTORY = "logs/tubes/geothermal-work-files/20261017-files-01/verification.json"
EARLIER = "logs/tubes/geothermal-compatibility/20261017-files-01/closure-verification.json"
BUILD_RECEIPT = "logs/tubes/facility-interaction/20261018-native-01/verification.json"


def retained_writers():
    """Pin genuine earlier paid-work checkpoints without requiring historical binaries.

    @return Two retained writer reports and their twelve original checkpoint identities.
    """
    history = json.loads((ROOT / HISTORY).read_text(encoding="utf-8"))
    writers = {}
    # Historical receipts retain their original source/binary identities. Only
    # immutable writer reports and actual input saves are checked against disk;
    # rebuilding today's binary cannot rewrite yesterday's accepted evidence.
    for record in history["records"]:
        if record["phase"] != "write":
            continue
        assert identity(ROOT / record["report"]["path"]) == record["report"]
        writer = json.loads((ROOT / record["report"]["path"]).read_text(encoding="utf-8"))
        assert len(writer["checkpoints"]) == 12
        for point in writer["checkpoints"]:
            assert identity(ROOT / point["input"]["path"]) == point["input"]
            assert OWNERS <= world_chunks(ROOT / point["input"]["path"]).keys()
        writers[writer["configuration"]] = {"report": record["report"], **writer}
    assert set(writers) == {"Debug", "RelWithDebInfo"}
    return writers


def main():
    """Verify focused current reload/continuation reports and publish complete owner evidence.

    @return None; any stale identity, missing scene or changed authoritative byte aborts acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=24, required=True, help="Exact reload/step reports for six scenes in both current builds.")
    parser.add_argument("--output", required=True, help="New immutable verification JSON beneath repository logs.")
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    configurations = ("Debug", "RelWithDebInfo")
    writers = retained_writers()
    earlier_path = ROOT / EARLIER
    earlier = json.loads(earlier_path.read_text(encoding="utf-8"))
    assert earlier["native_version"] == 434
    assert earlier["native_cases"] == 132
    # The genuine earlier-format corpus remains the authority for defaults and
    # malformed refusal/recovery. Pin every retained save, report and native log,
    # while leaving its historical executable/source hashes as dated evidence.
    historical_cache, expanded_reports = {}, set()
    pending = [earlier]
    while pending:
        value = pending.pop()
        if isinstance(value, dict):
            if {"path", "bytes", "sha256"} <= value.keys():
                path = value["path"]
                if path.startswith(("mlm/reviews/", "logs/")):
                    verify_references(value, historical_cache)
                    # Receipts link earlier receipts by identity. Expand each
                    # retained JSON once so genuine old save/log identities are
                    # checked too, without requiring an obsolete executable.
                    if path.endswith(".json") and path not in expanded_reports:
                        expanded_reports.add(path)
                        pending.append(json.loads((ROOT / path).read_text(encoding="utf-8")))
            pending.extend(value.values())
        elif isinstance(value, list):
            pending.extend(value)
    cache, grid, records = {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Both readers must inspect their opposite build's actual retained originals.
    # Complete repeated worlds and next-step owners, rather than just a power
    # digest, prevent a lost order/position/packet from passing this closure.
    for name in args.reports:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg, phase, case = report["configuration"], report["phase"], report["case"]
        key = cfg, phase, case
        assert cfg in configurations and phase in ("reload", "step") and case in range(6) and key not in grid
        opposite = configurations[1 - configurations.index(cfg)]
        writer = writers[opposite]
        assert report["writer"] == writer["report"]
        points = {point["step"]: point for point in writer["checkpoints"] if point["phase"] == case}
        assert report["checkpoint"] == points[0]
        assert len(report["native"]) == 2
        for process in report["native"]:
            log = (ROOT / process["log"]["path"]).read_text(encoding="utf-8")
            count = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", log)
            assert count is not None and "NATIVE_EXIT_CODE=0" in log
            assert (int(count[1]), int(count[2])) == (process["assertions"], process["cases"])
            summary[cfg]["cases"] += process["cases"]
            summary[cfg]["assertions"] += process["assertions"]
        original = ROOT / points[0]["input"]["path"]
        first, second = [ROOT / process["output"]["path"] for process in report["native"]]
        if phase == "reload":
            owner_count = compare_chunks(original, first, OWNERS)
            assert compare_chunks(first, second, None) == 126
            record_proof = {"first_complete_owners_exact": owner_count, "repeated_complete_chunks_exact": 126}
        else:
            assert report["uninterrupted_baseline"] == points[1]["input"]
            proof = compare_next_step(first, second, points[0]["tick"], points[1]["tick"])
            assert proof == report["next_step"]
            assert compare_chunks(first, second, OWNERS) == len(OWNERS)
            record_proof = {"next_step": proof, "next_step_complete_owners_exact": len(OWNERS)}
        records.append({"report": identity(path), "owner_proof": record_proof, **report})
        grid[key] = report
    assert set(grid) == {(cfg, phase, case) for cfg in configurations for phase in ("reload", "step") for case in range(6)}
    cross_readers = []
    # Independent writer worlds already agree completely. Replayed readers must
    # also agree on every output, including their actual native load-event tick.
    for case in range(6):
        for step in range(2):
            points = [next(point for point in writers[cfg]["checkpoints"] if (point["phase"], point["step"]) == (case, step)) for cfg in configurations]
            assert compare_chunks(ROOT / points[0]["input"]["path"], ROOT / points[1]["input"]["path"], None) == 126
        for phase in ("reload", "step"):
            for index in range(2):
                a, b = [grid[cfg, phase, case]["native"][index]["output"] for cfg in configurations]
                assert compare_chunks(ROOT / a["path"], ROOT / b["path"], None) == 126
                cross_readers.append({"case": case, "phase": phase, "output_index": index, "left": a, "right": b, "complete_chunks_exact": 126})
    build_path = ROOT / BUILD_RECEIPT
    builds = json.loads(build_path.read_text(encoding="utf-8"))
    verify_references(builds["binaries"], cache)
    verify_references(builds["builds"], cache)
    verify_references(builds["packed_art"], cache)
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md13E; complete native orders/physical vehicle/packet/service/power/network owners, independent of display caches",
        "controller": identity(__file__), "summary": summary,
        "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "complete_owner_chunks": sorted(name.decode() for name in OWNERS),
        "records": records, "cross_reader_pairs": cross_readers,
        "retained_writers": [writer["report"] for writer in writers.values()],
        "earlier_file_acceptance": identity(earlier_path),
        "earlier_file_classifications": {
            "434": "Actual native/external work, both source switches, cable waiting and each depot service retain original packet, invoices, fractions and generator identity.",
            "433": "Genuine earlier source files have no implicit geothermal vehicle binding; existing original external work remains its original authority.",
            "387": "Genuine paid external-source/metered movement retains its original invoices and capsule history without a fabricated geothermal provider.",
            "386": "Genuine pre-metering tube files restore no implicit powered capsule binding or invented consumed work.",
            "malformed": "Retained semantic preview/full refusal and valid recovery evidence remains pinned; compression-envelope errors are not semantic validation.",
        },
        "exact_current_identities": len(cache), "exact_historical_artifact_identities": len(historical_cache),
        "native_version": 434, "packed_art": builds["packed_art"], "binaries": builds["binaries"],
        "builds": builds["builds"], "build_receipt": identity(build_path),
        "owner_sources": [identity(ROOT / name) for name in (
            "src/saveload/vehicle_sl.cpp", "src/saveload/order_sl.cpp", "src/saveload/cargopacket_sl.cpp",
            "src/saveload/station_sl.cpp", "src/saveload/map_sl.cpp", "src/saveload/transport_tube_power_sl.cpp",
            "src/saveload/transport_tube_vehicle_sl.cpp", "src/saveload/transport_tube_geothermal_sl.cpp",
            "src/tests/transport_tube_geothermal_work_load.cpp", "src/tests/multilayer/utilities/geothermal_tube_work_files.py")],
        "remaining": "14 lifecycle,15 useful introduction-date routes; runtime gallery, host/client and human feedback remain separate.",
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "owner_chunks": len(OWNERS), "cross_reader_pairs": len(cross_readers)}))


if __name__ == "__main__":
    main()
