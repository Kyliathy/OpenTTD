"""Accept original physical Forge input delivery and actual native RFMT manufacture.

@file forge_raw_delivery_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FCC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Produced-source files, downstream Forge modal routing, runtime silhouette and paid TBM/bootstrap remain separate tasks.
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
from forge_raw_delivery import PREPARE_RECEIPT, PREPARE_SHA256, validate_result


def main():
    """Require exact opposite paid inputs, independent native balances and every complete final native chunk.

    @return None; altered originals, stale current references or unequal genuine manufacturing worlds prevent acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    # A receipt is immutable evidence in the reproducible log owner; another
    # invocation cannot silently overwrite the accepted original handoff.
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / PREPARE_RECEIPT
    assert identity(receipt)["sha256"] == PREPARE_SHA256
    history = json.loads(receipt.read_text(encoding="utf-8"))
    originals = {row["configuration"]: row for row in history["records"] if row.get("focus") == "raw-route-prepare"}
    configurations = ("Debug", "RelWithDebInfo")
    cache, runs, records = {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Each run consumes the exact opposite original paid boundary. Historical
    # nested binaries stay opaque; rebuilding cannot authorize new participants.
    for name in args.reports:
        path = ROOT / name
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg = report["configuration"]
        assert cfg in configurations and cfg not in runs and report["focus"] == "raw-delivery"
        opposite = configurations[1 - configurations.index(cfg)]
        original = originals[opposite]
        assert report["prepare_report"] == original["report"] and report["prepare_receipt"] == identity(receipt)
        assert report["native"]["input"] == original["native"]["output"]
        validate_result(report["result"], report["materials"], report["scenes"], report["returned"], report["recipe"], report["snapshot"], original["prepared"])
        count_native(report["native"], summary[cfg])
        runs[cfg] = report
        records.append(dict(report=identity(path), **report))
    assert set(runs) == set(configurations) and summary["Debug"] == summary["RelWithDebInfo"]
    for key in ("result", "materials", "scenes", "returned", "recipe", "snapshot"):
        assert runs["Debug"][key] == runs["RelWithDebInfo"][key]
    left, right = [runs[cfg]["native"]["output"] for cfg in configurations]
    assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
    builds = []
    # This group changes only native test consumers and controllers. Require
    # both completed test targets; existing game-target evidence stays valid.
    for name in args.build_logs:
        path = ROOT / name
        raw = path.read_bytes()
        log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log
        builds.append(identity(path))
    result = dict(
        _documentation="r2mlvs17-underground-transport-tubes.md15FCC; original actual raw carriage/manufacture/whole empty returns with continuing finite source and actual unused inventories",
        controller=identity(__file__), summary=summary, records=records,
        native_cases=sum(row["cases"] for row in summary.values()), native_assertions=sum(row["assertions"] for row in summary.values()),
        result=runs["Debug"]["result"], materials=runs["Debug"]["materials"], prepare_receipt=identity(receipt),
        complete_world_pair=dict(left=left, right=right, complete_chunks_exact=126), builds=builds,
        exact_identities_verified=len(cache), native_version=434,
        test_binaries=[identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        fixture_classification=runs["Debug"]["fixture_classification"],
        remaining="15FCD original manufactured-RFMT ordinary files precede15FDB/15FDC actual Forge modal preparation/useful delivery and15FEB/15FEC/15FEE original files/native silhouette. Actual TBM/bootstrap and human/gallery/operator gates remain pending.",
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(receipt=identity(output), summary=summary, result=result["result"], materials=result["materials"])))


if __name__ == "__main__":
    main()
