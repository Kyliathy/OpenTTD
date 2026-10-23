"""Accept actual original-world paid Forge raw modal preparation before useful movement.

@file forge_raw_route_prepare_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FCB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Original finite mines/CFMT/raw carriers remain; stopped paid preparation is not actual input delivery, RFMT manufacture or TBM bootstrap.
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
from forge_raw_route_prepare import SOURCE_RECEIPT


def main():
    """Require exact opposite source inputs, real paid stopped participants and unchanged complete source-driver boundaries.

    @return None; changed source/history/quantity owners, stale current references or unequal complete ready worlds prevent acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--source-checks", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / SOURCE_RECEIPT
    assert identity(receipt)["sha256"] == "3213729e82ceb9ef5c7892617576b78df649bbb8ea6980950e6017d62126fc5b"
    history = json.loads(receipt.read_text(encoding="utf-8")); source = history["source"]
    originals = {row["configuration"]: row for row in history["records"] if row.get("phase") == "write"}
    configurations = ("Debug", "RelWithDebInfo"); cache, prepared, checks, records = {}, {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # The accepted source remains an opaque original handoff. Every current
    # preparation must load its exact opposite actual boundary0, retaining
    # the original funded mines/refinery/carriers rather than recreating owners.
    for name in args.reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in configurations and cfg not in prepared and report["focus"] == "raw-route-prepare"
        opposite = configurations[1 - configurations.index(cfg)]; original = originals[opposite]
        point = next(row for row in original["checkpoints"] if row["step"] == 0)
        assert report["source_writer"] == original["report"] and report["source_checkpoint"] == point and report["source_receipt"] == identity(receipt)
        assert report["native"]["input"] == point["input"]
        ready = report["prepared"]
        assert ready["tick"] == source["tick"] and 0 < ready["cash"] < source["cash"] and ready["loan"] == 0
        assert all(ready[key] == source[key] for key in ("copper", "coal", "copper_reserve", "coal_reserve", "parts", "industries"))
        assert len(set(ready["parts"])) == 4 and len(set(ready["stations"])) == 3 and len(set(ready["portals"])) == 2
        count_native(report["native"], summary[cfg]); prepared[cfg] = report; records.append({"report": identity(path), **report})
    # Adding actual raw-route consumers must not alter the original shared
    # native source driver or its ordinary source continuation. Recheck exactly
    # the affected opposite original source0->1 pair in each current binary.
    for name in args.source_checks:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in configurations and cfg not in checks and report["phase"] == "step"
        opposite = configurations[1 - configurations.index(cfg)]; original = originals[opposite]
        points = {point["step"]: point for point in original["checkpoints"]}
        assert report["writer"] == original["report"] and report["checkpoint"] == points[0] and report["uninterrupted_baseline"] == points[1]["input"]
        assert report["binary"] == prepared[cfg]["binary"] and len(report["native"]) == 2
        assert all(process["observed_snapshot"] == points[1]["snapshot"] and process["preview_pure"] for process in report["native"])
        first, second = (ROOT / process["output"]["path"] for process in report["native"])
        assert report["next_step"] == compare_next_step(first, second, points[0]["tick"], points[1]["tick"])
        assert compare_chunks(first, second, None) == 126
        for process in report["native"]:
            count_native(process, summary[cfg])
        checks[cfg] = report; records.append({"report": identity(path), **report})
    assert prepared.keys() == checks.keys() == set(configurations) and summary["Debug"] == summary["RelWithDebInfo"]
    assert prepared["Debug"]["prepared"] == prepared["RelWithDebInfo"]["prepared"]
    left, right = (prepared[cfg]["native"]["output"] for cfg in configurations)
    assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
    builds = []
    for name in args.build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log and "openttd_test.exe" in log; builds.append(identity(path))
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15FCB; actual paid stopped Forge raw input preparation, preserving exact source and native continuation; physical input delivery/manufacture remain15FCC",
        "controller": identity(__file__), "summary": summary, "records": records, "prepared": prepared["Debug"]["prepared"],
        "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
        "complete_ready_pair": {"left": left, "right": right, "complete_chunks_exact": 126}, "source_receipt": identity(receipt),
        "exact_identities_verified": len(cache), "builds": builds, "native_version": 434,
        "test_binaries": [identity(ROOT / f"build/{cfg}/openttd_test.exe") for cfg in configurations],
        "fixture_classification": prepared["Debug"]["fixture_classification"],
        "remaining": "15FCC must earn actual CORE/Coal dispatch, conventional Underground-Surface input carriage, exact native CFMT RFMT manufacture and whole empty raw returns. Produced-source files precede the later Forge modal route/files/raster. Actual TBM/bootstrap and human/operator gates remain separate.",
    }
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "prepared": result["prepared"]}))


if __name__ == "__main__":
    main()
