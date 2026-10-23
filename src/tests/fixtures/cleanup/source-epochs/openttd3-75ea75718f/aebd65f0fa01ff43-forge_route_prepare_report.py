"""Accept actual paid stopped Forge modal preparation in exact original manufactured-RFMT worlds.

@file forge_route_prepare_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FDB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Actual native useful freight remains15FDC; paid preparation does not establish delivery or TBM bootstrap.
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
from forge_route_prepare import SOURCE_RECEIPT, SOURCE_SHA256, validate_prepared


def main():
    """Require original opposite inputs, actual paid stopped owners and unchanged complete source continuation.

    @return None; altered originals, stale current references or unequal complete native ready/source worlds prevent acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--source-checks", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    # Acceptance evidence stays immutable in the reproducible log owner;
    # another invocation must select a fresh absent receipt destination.
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / SOURCE_RECEIPT
    assert identity(receipt)["sha256"] == SOURCE_SHA256
    history = json.loads(receipt.read_text(encoding="utf-8"))
    originals = {row["configuration"]: row for row in history["records"] if row.get("phase") == "write"}
    configurations = ("Debug", "RelWithDebInfo")
    cache, prepared, checks, records = {}, {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Exact opposite accepted source0 worlds own every original material,
    # recipe and participant. Dated nested binaries remain opaque history;
    # a current rebuild cannot authorize stock or source reconstruction.
    for name in args.reports:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]
        assert cfg in configurations and cfg not in prepared and report["focus"] == "route-prepare"
        opposite = configurations[1 - configurations.index(cfg)]
        original = originals[opposite]
        point = next(row for row in original["checkpoints"] if row["step"] == 0)
        assert report["source_writer"] == original["report"] and report["source_checkpoint"] == point and report["source_receipt"] == identity(receipt)
        assert report["native"]["input"] == point["input"]
        validate_prepared(report["prepared"], report["snapshot"], point, history)
        count_native(report["native"], summary[cfg])
        prepared[cfg] = report
        records.append(dict(report=identity(path), **report))
    # The borrowed loader now admits explicit later modal pools only when
    # asked. Original file callers retain strict4/3/3 pools and exact native
    # source0->1 results; every saved original chunk remains independently exact.
    for name in args.source_checks:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]
        assert cfg in configurations and cfg not in checks and report["phase"] == "step"
        opposite = configurations[1 - configurations.index(cfg)]
        original = originals[opposite]
        points = {point["step"]: point for point in original["checkpoints"]}
        assert report["writer"] == original["report"] and report["checkpoint"] == points[0] and report["uninterrupted_baseline"] == points[1]["input"]
        assert report["binary"] == prepared[cfg]["binary"] and len(report["native"]) == 2
        assert all(process["observed_snapshot"] == points[1]["snapshot"] and process["preview_pure"] for process in report["native"])
        first, second = (ROOT / process["output"]["path"] for process in report["native"])
        assert report["next_step"] == compare_next_step(first, second, points[0]["tick"], points[1]["tick"])
        assert compare_chunks(ROOT / points[1]["input"]["path"], first, None) == compare_chunks(first, second, None) == 126
        for process in report["native"]:
            count_native(process, summary[cfg])
        checks[cfg] = report
        records.append(dict(report=identity(path), **report))
    assert prepared.keys() == checks.keys() == set(configurations) and summary["Debug"] == summary["RelWithDebInfo"]
    assert prepared["Debug"]["prepared"] == prepared["RelWithDebInfo"]["prepared"] and prepared["Debug"]["snapshot"] == prepared["RelWithDebInfo"]["snapshot"]
    left, right = (prepared[cfg]["native"]["output"] for cfg in configurations)
    assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
    builds = []
    for name in args.build_logs:
        path = ROOT / name
        raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log
        builds.append(identity(path))
    result = dict(
        _documentation="r2mlvs17-underground-transport-tubes.md15FDB; original genuine RFMT paid stopped Forge modal preparation with unchanged finite/raw/recipe/packet/account owners",
        controller=identity(__file__), summary=summary, records=records, prepared=prepared["Debug"]["prepared"],
        native_cases=sum(row["cases"] for row in summary.values()), native_assertions=sum(row["assertions"] for row in summary.values()),
        complete_ready_pair=dict(left=left, right=right, complete_chunks_exact=126), source_receipt=identity(receipt),
        exact_identities_verified=len(cache), builds=builds, native_version=434,
        test_binaries=[identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        fixture_classification=prepared["Debug"]["fixture_classification"],
        remaining="15FDC must earn actual original RFMT through conventional Surface/UG feed, local paid Forge and conventional Surface ELEC intake/payment, then whole empty returns. Native source/recipe/unused raw/packet/current+old account/power owners remain conserved. Original useful files/raster, actual TBM/bootstrap and human/gallery/operator gates remain pending.",
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(receipt=identity(output), summary=summary, prepared=result["prepared"])))


if __name__ == "__main__":
    main()
