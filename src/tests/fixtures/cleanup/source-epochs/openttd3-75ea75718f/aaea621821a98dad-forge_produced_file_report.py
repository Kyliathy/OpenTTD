"""Accept original genuine RFMT files and complete ordinary opposite-reader continuation.

@file forge_produced_file_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FCD.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Both finite sources, unused raw inputs, actual batches and native accounts remain authoritative saved owners.
"""
import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from local_electrical_feed_checks import identity
from geothermal_tube_work_files import compare_chunks, compare_next_step
from tube_sprite_report import verify_references
from mole_useful_raster_report import count_native
from forge_produced_files import DELIVERY_RECEIPT, DELIVERY_SHA256, validate_point


def main():
    """Require original manufactured boundaries, every opposite ordinary reader and unchanged current raw delivery.

    @return None; changed originals, stale current references or missing complete native continuation prevent acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=6, required=True, help="Two original writers and opposite reload/step reports per configuration.")
    parser.add_argument("--current-deliveries", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    # Accepted evidence remains immutable in the reproducible log owner;
    # subsequent executions must use a fresh absent receipt destination.
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / DELIVERY_RECEIPT
    assert identity(receipt)["sha256"] == DELIVERY_SHA256
    history = json.loads(receipt.read_text(encoding="utf-8"))
    originals = {row["configuration"]: row for row in history["records"]}
    configurations = ("Debug", "RelWithDebInfo")
    cache, writers, readers, deliveries, records = {}, {}, {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Current reports bind actual current code/binaries/outputs. The sealed
    # original lineage stays opaque; later builds cannot replace its worlds.
    for name in (*args.reports, *args.current_deliveries):
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]
        assert cfg in configurations
        if report.get("focus") == "raw-delivery":
            assert cfg not in deliveries
            deliveries[cfg] = report
            processes = [report["native"]]
        else:
            assert report["delivery_receipt"] == identity(receipt)
            phase = report["phase"]
            if phase == "write":
                assert cfg not in writers and len(report["checkpoints"]) == 2 and len(report["native"]) == 1
                writers[cfg] = dict(report=identity(path), **report)
            else:
                assert phase in ("reload", "step") and (cfg, phase) not in readers and len(report["native"]) == 2
                readers[cfg, phase] = report
            processes = report["native"]
        for process in processes:
            count_native(process, summary[cfg])
        records.append(dict(report=identity(path), **report))
    assert writers.keys() == deliveries.keys() == set(configurations)
    assert readers.keys() == {(cfg, phase) for cfg in configurations for phase in ("reload", "step")}
    assert summary["Debug"] == summary["RelWithDebInfo"]
    grids = {cfg: {point["step"]: point for point in writers[cfg]["checkpoints"]} for cfg in configurations}
    pairs = []
    # Every writer starts from the exact opposite accepted manufactured
    # output, and current route execution still earns that complete original
    # result. Neither new file observers nor shared helpers may alter it.
    for cfg in configurations:
        opposite = configurations[1 - configurations.index(cfg)]
        original = originals[opposite]
        assert writers[cfg]["delivery_report"] == original["report"] and writers[cfg]["native"][0]["input"] == original["native"]["output"]
        assert writers[cfg]["original_complete_chunks_exact"] == 126
        assert writers[cfg]["binary"] == deliveries[cfg]["binary"]
        for key in ("result", "materials", "scenes", "returned", "recipe", "snapshot"):
            assert deliveries[cfg][key] == originals[cfg][key]
        assert compare_chunks(ROOT / deliveries[cfg]["native"]["output"]["path"], ROOT / originals[cfg]["native"]["output"]["path"], None) == 126
        for point in grids[cfg].values():
            validate_point(point, original)
        assert compare_chunks(ROOT / original["native"]["output"]["path"], ROOT / grids[cfg][0]["input"]["path"], None) == 126
    for step in range(2):
        left, right = (grids[cfg][step] for cfg in configurations)
        assert left["snapshot"] == right["snapshot"]
        assert compare_chunks(ROOT / left["input"]["path"], ROOT / right["input"]["path"], None) == 126
        pairs.append(dict(phase="writer", step=step, left=left["input"], right=right["input"], complete_chunks_exact=126))
    # Every actual opposite reader preserves copied-preview purity and every
    # original byte through first/repeated loading or the full next native
    # opportunity. GLOG and all quantity owners receive no exclusions.
    for (cfg, phase), report in readers.items():
        opposite = configurations[1 - configurations.index(cfg)]
        points = grids[opposite]
        assert report["writer"] == writers[opposite]["report"] and report["checkpoint"] == points[0]
        assert report["binary"] == writers[cfg]["binary"] and all(process["preview_pure"] for process in report["native"])
        assert report["native"][0]["input"] == (points[0]["input"] if phase == "reload" else points[1]["input"])
        first, second = (ROOT / process["output"]["path"] for process in report["native"])
        if phase == "reload":
            assert report["first_load_complete_chunks_exact"] == report["repeated_complete_chunks_exact"] == 126
            assert report["native"][1]["input"] == report["native"][0]["output"]
            assert all(process["observed_snapshot"] == points[0]["snapshot"] for process in report["native"])
            assert compare_chunks(ROOT / points[0]["input"]["path"], first, None) == compare_chunks(first, second, None) == 126
        else:
            assert report["native"][1]["input"] == points[0]["input"] and report["uninterrupted_baseline"] == points[1]["input"]
            assert all(process["observed_snapshot"] == points[1]["snapshot"] for process in report["native"])
            assert report["next_step"] == compare_next_step(first, second, points[0]["tick"], points[1]["tick"])
            assert compare_chunks(ROOT / points[1]["input"]["path"], first, None) == compare_chunks(first, second, None) == 126
    for phase in ("reload", "step"):
        for index in range(2):
            left, right = (readers[cfg, phase]["native"][index]["output"] for cfg in configurations)
            assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
            pairs.append(dict(phase=phase, boundary=index, left=left, right=right, complete_chunks_exact=126))
    builds = []
    for name in args.build_logs:
        path = ROOT / name
        raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log
        builds.append(identity(path))
    packed = identity(ROOT / "media/baseset/openttd.grf")
    assert packed["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = dict(
        _documentation="r2mlvs17-underground-transport-tubes.md15FCD; exact original manufactured RFMT ordinary files and complete next opportunities preserve both finite mines/unused raw inputs/actual recipe and account histories",
        controller=identity(__file__), summary=summary, records=records,
        native_cases=sum(row["cases"] for row in summary.values()), native_assertions=sum(row["assertions"] for row in summary.values()),
        result=history["result"], materials=history["materials"], complete_world_pairs=pairs,
        delivery_receipt=identity(receipt), exact_identities_verified=len(cache), native_version=434, builds=builds, packed_art=packed,
        test_binaries=[identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        continuation_contract="Both original four-part/three-industry/three-station manufactured worlds retain513 named observations and all126 complete native chunks through ordinary loading, pure copied preview, repeated saves and opposite-reader complete next opportunities. The single common native driver advances both finite sources/industry work once without replaying raw delivery/manufacture/payment. Native RNG is canonical; unsaved client RNG is independently pure. No source/route builder, quantity/rating/pose/order/date/account repair or producer freeze participates.",
        fixture_classification=deliveries["Debug"]["fixture_classification"],
        remaining="15FDB paid stopped Forge modal preparation precedes15FDC useful RFMT delivery,15FEB/15FEC/15FEE files/runtime silhouette. Actual TBM/bootstrap and human/gallery/operator gates remain pending.",
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(receipt=identity(output), summary=summary, complete_world_pairs=len(pairs))))


if __name__ == "__main__":
    main()
