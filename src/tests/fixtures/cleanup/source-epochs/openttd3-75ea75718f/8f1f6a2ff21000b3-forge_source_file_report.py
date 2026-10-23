"""Accept exact original2005 Forge source files and complete ordinary opposite-reader continuation.

@file forge_source_file_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FBC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Source file readiness does not establish physical raw carriage, manufactured RFMT or useful Forge movement.
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
from forge_source_files import STOCK_RECEIPT
from forge_source_prerequisites import CASES
from forge_source_files import validate_point


def main():
    """Require original source boundaries, all opposite ordinary readers and current source/definition/four-target evidence.

    @return None; changed originals, stale current references or missing complete source continuation prevent acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs=6, required=True, help="Two original boundary writers plus reload/step reports per opposite reader.")
    parser.add_argument("--current-stocks", nargs=2, required=True)
    parser.add_argument("--gates", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=4, required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args(); output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    receipt = ROOT / STOCK_RECEIPT
    assert identity(receipt)["sha256"] == "219509cd72e634046615ab8b9e6c71b34df9a8aa52e76d625820789e77296714"
    history = json.loads(receipt.read_text(encoding="utf-8")); source = history["source"]
    originals = {row["configuration"]: row for row in history["records"] if row["focus"] == "raw-stock"}
    configurations = ("Debug", "RelWithDebInfo"); cache, writers, readers, stocks, gates, records = {}, {}, {}, {}, {}, []
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in configurations}
    # Current records bind actual current sources/binaries/logs. The sealed
    # original receipt remains opaque history; later builds cannot rewrite
    # its accepted original worlds or fresh-compare its dated nested binaries.
    for name in (*args.reports, *args.current_stocks, *args.gates):
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_references(report, cache)
        cfg = report["configuration"]; assert cfg in configurations
        focus = report.get("focus", "file")
        if focus == "raw-stock":
            assert cfg not in stocks and report["source"] == source; stocks[cfg] = report
        elif focus == "producers":
            assert cfg not in gates and [process["case"] for process in report["native"]] == [case for case, _ in CASES]; gates[cfg] = report
        else:
            assert focus == "file" and report["stock_receipt"] == identity(receipt)
            phase = report["phase"]
            if phase == "write":
                assert cfg not in writers and len(report["checkpoints"]) == 2 and len(report["native"]) == 1
                writers[cfg] = {"report": identity(path), **report}
            else:
                assert phase in ("reload", "step") and (cfg, phase) not in readers and len(report["native"]) == 2
                readers[cfg, phase] = report
        processes = [report["native"]] if focus == "raw-stock" else report["native"]
        for process in processes:
            count_native(process, summary[cfg])
        records.append({"report": identity(path), **report})
    assert writers.keys() == stocks.keys() == gates.keys() == set(configurations)
    assert readers.keys() == {(cfg, phase) for cfg in configurations for phase in ("reload", "step")}
    assert summary["Debug"] == summary["RelWithDebInfo"]
    grids = {cfg: {point["step"]: point for point in writers[cfg]["checkpoints"]} for cfg in configurations}; pairs = []
    # Every current native producer and boundary0 must still be the exact
    # accepted original source. A refactored shared tick cannot invent stock,
    # reset work, alter reserve or quietly substitute an author template.
    for cfg in configurations:
        opposite = configurations[1 - configurations.index(cfg)]; original = originals[opposite]
        assert writers[cfg]["stock_report"] == original["report"] and writers[cfg]["native"][0]["input"] == original["native"]["output"]
        assert writers[cfg]["original_complete_chunks_exact"] == 126
        assert writers[cfg]["binary"] == stocks[cfg]["binary"] == gates[cfg]["binary"]
        assert compare_chunks(ROOT / stocks[cfg]["native"]["output"]["path"], ROOT / originals[cfg]["native"]["output"]["path"], None) == 126
        for step, point in grids[cfg].items():
            validate_point(point, source)
        assert compare_chunks(ROOT / original["native"]["output"]["path"], ROOT / grids[cfg][0]["input"]["path"], None) == 126
    for step in range(2):
        left, right = (grids[cfg][step] for cfg in configurations)
        assert left["snapshot"] == right["snapshot"] and left["parts"] == right["parts"] and left["references"] == right["references"]
        assert compare_chunks(ROOT / left["input"]["path"], ROOT / right["input"]["path"], None) == 126
        pairs.append({"phase": "writer", "step": step, "left": left["input"], "right": right["input"], "complete_chunks_exact": 126})
    # Every ordinary consumer loads the exact opposite native writer. Pure
    # preview and repeats preserve all original owner bytes; continuation
    # matches its untouched complete clocks/vehicles/industry opportunity.
    for (cfg, phase), report in readers.items():
        opposite = configurations[1 - configurations.index(cfg)]; points = grids[opposite]
        assert report["writer"] == writers[opposite]["report"] and report["checkpoint"] == points[0]
        assert report["binary"] == writers[cfg]["binary"] and all(process["preview_pure"] for process in report["native"])
        first, second = (ROOT / process["output"]["path"] for process in report["native"])
        if phase == "reload":
            assert report["first_load_complete_chunks_exact"] == report["repeated_complete_chunks_exact"] == 126
            assert all(process["observed_snapshot"] == points[0]["snapshot"] for process in report["native"])
            assert compare_chunks(ROOT / points[0]["input"]["path"], first, None) == compare_chunks(first, second, None) == 126
        else:
            assert report["uninterrupted_baseline"] == points[1]["input"]
            assert all(process["observed_snapshot"] == points[1]["snapshot"] for process in report["native"])
            assert report["next_step"] == compare_next_step(first, second, points[0]["tick"], points[1]["tick"])
            assert compare_chunks(ROOT / points[1]["input"]["path"], first, None) == compare_chunks(first, second, None) == 126
    # Cross-reader comparisons include every native chunk, including GLOG;
    # no catalogue, map, account or diagnostic exclusion can hide divergence.
    for phase in ("reload", "step"):
        for index in range(2):
            left, right = (readers[cfg, phase]["native"][index]["output"] for cfg in configurations)
            assert compare_chunks(ROOT / left["path"], ROOT / right["path"], None) == 126
            pairs.append({"phase": phase, "boundary": index, "left": left, "right": right, "complete_chunks_exact": 126})
    builds = []
    for name in args.build_logs:
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "error MSB" not in log
        assert "openttd.exe" in log or "openttd_test.exe" in log; builds.append(identity(path))
    packed = identity(ROOT / "media/baseset/openttd.grf")
    assert packed["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15FBC; exact original2005 raw sources/empty CFMT ordinary files and complete next opportunity; raw carriage/manufacture remain15FCB/15FCC",
        "controller": identity(__file__), "summary": summary, "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()), "records": records, "source": source,
        "complete_world_pairs": pairs, "stock_receipt": identity(receipt), "exact_identities_verified": len(cache), "native_version": 434,
        "builds": builds, "packed_art": packed,
        "binaries": [identity(ROOT / f"build/{cfg}/{target}.exe") for cfg in configurations for target in ("openttd", "openttd_test")],
        "continuation_contract": "Exact original four-part/three-industry CORE/Coal raw source and unfed CFMT retains all native126 chunks through first loading, pure copied preview, repeated saves and opposite-reader complete next opportunities. The common driver advances real clocks/vehicles/industry work once; no source builder, registry, date, account or stock repair participates. Native simulation RNG is canonical; unsaved client RNG is protected independently through preview/save.",
        "fixture_classification": stocks["Debug"]["fixture_classification"],
        "remaining": "15FCB paid actual raw routes precede15FCC physical refinery inputs/manufactured RFMT, produced-source files and actual Forge modal route/files/raster; actual TBM/bootstrap and human/operator gates remain pending.",
    }
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "complete_world_pairs": len(pairs)}))


if __name__ == "__main__":
    main()
