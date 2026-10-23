"""Accept original actual RFMT useful Forge freight with conserved finite sources and whole empty returns.

@file forge_useful_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FDC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Original loaded/final files, actual moving silhouette and paid TBM/bootstrap retain their subsequent tasks.
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
from forge_useful_checks import PREPARE_RECEIPT, PREPARE_SHA256, validate_result


def main():
    """Require exact opposite paid inputs, complete equal useful worlds and unchanged original strict produced-file consumers.

    @return None; altered originals, stale current references, stock/payment divergence or unequal native outputs prevent acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--source-checks", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    # Acceptance evidence is immutable in the reproducible log owner. Later
    # runs select another absent destination instead of replacing a handoff.
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / PREPARE_RECEIPT
    assert identity(receipt)["sha256"] == PREPARE_SHA256
    history = json.loads(receipt.read_text(encoding="utf-8"))
    originals = {row["configuration"]: row for row in history["records"] if row.get("focus") == "route-prepare"}
    source_receipt = ROOT / history["source_receipt"]["path"]
    assert identity(source_receipt) == history["source_receipt"]
    source = json.loads(source_receipt.read_text(encoding="utf-8"))
    source_writers = {row["configuration"]: row for row in source["records"] if row.get("phase") == "write"}
    configurations = ("Debug", "RelWithDebInfo")
    cache, routes, checks, records = {}, {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Each useful run borrows the exact opposite accepted ready boundary.
    # Historical nested binaries stay opaque; current rebuilds never permit
    # participant/quantity reconstruction or a replacement source packet.
    for name in args.reports:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]
        assert cfg in configurations and cfg not in routes and report["focus"] == "forge-useful"
        opposite = configurations[1 - configurations.index(cfg)]
        original = originals[opposite]
        assert report["prepare_report"] == original["report"] and report["prepare_receipt"] == identity(receipt)
        assert report["native"]["input"] == original["native"]["output"]
        validate_result(report["result"], report["materials"], report["scenes"], report["returned"], report["snapshot"], original["prepared"])
        count_native(report["native"], summary[cfg])
        routes[cfg] = report
        records.append(dict(report=identity(path), **report))
    # Strict produced-only callers must still forbid manufactured RFMT
    # dispatch and preserve exact source0->1. The explicit useful mode cannot
    # weaken original raw ownership, ordinary preview or complete continuation.
    for name in args.source_checks:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]
        assert cfg in configurations and cfg not in checks and report["phase"] == "step"
        opposite = configurations[1 - configurations.index(cfg)]
        original = source_writers[opposite]
        points = {point["step"]: point for point in original["checkpoints"]}
        assert report["writer"] == original["report"] and report["checkpoint"] == points[0] and report["uninterrupted_baseline"] == points[1]["input"]
        assert report["binary"] == routes[cfg]["binary"] and len(report["native"]) == 2
        assert all(process["observed_snapshot"] == points[1]["snapshot"] and process["preview_pure"] for process in report["native"])
        first, second = (ROOT / process["output"]["path"] for process in report["native"])
        assert report["next_step"] == compare_next_step(first, second, points[0]["tick"], points[1]["tick"])
        assert compare_chunks(ROOT / points[1]["input"]["path"], first, None) == compare_chunks(first, second, None) == 126
        for process in report["native"]:
            count_native(process, summary[cfg])
        checks[cfg] = report
        records.append(dict(report=identity(path), **report))
    assert routes.keys() == checks.keys() == set(configurations) and summary["Debug"] == summary["RelWithDebInfo"]
    for key in ("result", "materials", "scenes", "returned", "snapshot"):
        assert routes["Debug"][key] == routes["RelWithDebInfo"][key]
    left, right = (routes[cfg]["native"]["output"] for cfg in configurations)
    assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
    builds = []
    # Only native test consumers change in this group. Require both current
    # completed test targets; existing production-target evidence stays valid.
    for name in args.build_logs:
        path = ROOT / name
        raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log
        builds.append(identity(path))
    result = dict(
        _documentation="r2mlvs17-underground-transport-tubes.md15FDC; exact original RFMT useful native modal freight and whole empty returns with continuing finite/raw/recipe/packet/account/power owners",
        controller=identity(__file__), summary=summary, records=records,
        native_cases=sum(row["cases"] for row in summary.values()), native_assertions=sum(row["assertions"] for row in summary.values()),
        result=routes["Debug"]["result"], materials=routes["Debug"]["materials"], prepare_receipt=identity(receipt),
        complete_world_pair=dict(left=left, right=right, complete_chunks_exact=126), builds=builds, exact_identities_verified=len(cache), native_version=434,
        test_binaries=[identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        fixture_classification=routes["Debug"]["fixture_classification"],
        remaining="15FEB must retain actual original moving/final Forge worlds and untouched full native baselines;15FEC ordinary readers and15FEE actual moving native pixels/picking precede15F.1/original15.2 closure. Actual TBM/bootstrap and human/gallery/operator gates remain pending.",
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(receipt=identity(output), summary=summary, result=result["result"], materials=result["materials"])))


if __name__ == "__main__":
    main()
