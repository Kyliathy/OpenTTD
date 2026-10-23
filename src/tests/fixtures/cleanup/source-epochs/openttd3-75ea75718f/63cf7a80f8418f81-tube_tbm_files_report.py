"""Seal complete original paid-route queue/power native file acceptance.

@file tube_tbm_files_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group16D.2.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md10.8.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Every current file field is compared;upstream paid history remains its exact sealed original.
"""

import argparse
import json
import re
import sys

sys.dont_write_bytecode = True
from construction_provider_return import ROOT, identity
from tube_tbm_files import QUEUE, SOURCES, STAGES, equal_world
from portal_consumer_report import CONFIGURATIONS, checked

HANDOFFS = {
    "paid-safe-reuse": ("logs/tbm/nonrail-reuse/20261019-native-01/verification.json", "ebd7968021620078041ead008c1441aa95ff99417686bbbc0d700c80ca005516"),
    "complete-catalogue-paid-clearing": ("logs/tubes/tbm-final-use/20261019-native-01/clearance-verification-2.json", "b3709dadebb2ceb998b2d7bef7a038a96e7b8287ca1f7c57c5265b0f0383a226"),
    "earned-first-cargo": ("logs/tubes/tbm-final-use/20261019-native-01/useful-prepare-verification.json", "13d536cf642e679636c17fbf28d30a0d91380029e87709ed44404bdd9e8126a2"),
    "actual-useful-Surface-route": ("logs/tubes/tbm-final-use/20261019-native-01/useful-delivery-verification.json", "9420fa1df3b943b6acb1fc8d4a1d44f7f103a57a500f6896defeeca36bd09662"),
    "actual-queue-paid-recovery": QUEUE,
}


def accept(selection_path, output_path):
    """Accept all original cold/repeat/full-next phases and exact paid construction/useful lineage.

    @param selection_path Explicit complete eight-phase file report/build index.
    @param output_path Absent retained final technical handoff beneath logs.
    @return Complete native16D/original16.3 acceptance without a normal-newgame or operator claim.
    """
    selection_path = ROOT / selection_path
    selection = json.loads(selection_path.read_text(encoding="utf-8"))
    assert selection["complete"] is True
    handoffs, documents = {}, {}
    # Preserve the exact paid cutting,whole disposal,construction,finite cargo,
    # useful Surface transfer and real queue/recovery histories. Dated binaries
    # are never reopened or relabelled as these current file-reader results.
    for role, (name, expected) in HANDOFFS.items():
        path = ROOT / name
        reference = identity(path)
        assert reference["sha256"] == expected, role
        handoffs[role] = reference
        documents[role] = json.loads(path.read_text(encoding="utf-8"))
    queue = documents["actual-queue-paid-recovery"]
    expected = {(cfg, stage) for cfg in CONFIGURATIONS for stage in STAGES}
    cache, records, seen = {}, [], set()
    native_cases, native_assertions, comparisons = 0, 0, 0
    by_phase = {}
    # Every saved queue,power-loss,paid repair and recovered phase must survive
    # both actual readers independently. Another stage cannot fill a missing
    # packet/position/grant/reservation continuation boundary.
    for row in selection["records"]:
        cfg, stage = row["configuration"], row["focus"]
        key = cfg, stage
        assert key in expected and key not in seen
        seen.add(key)
        path = ROOT / row["report"]
        report = json.loads(path.read_text(encoding="utf-8"))
        assert (report["configuration"], report["focus"], report["tag"]) == (cfg, stage, "[.tube-tbm-queue-file]")
        assert report["native_version"] == 434
        opposite = next(writer for writer in CONFIGURATIONS if writer != cfg)
        producer = queue["records"][opposite]
        assert report["queue_receipt"] == handoffs["actual-queue-paid-recovery"]
        assert report["queue_report"] == producer["report"]
        assert report["original"] == producer["outputs"][stage]
        assert report["baseline"] == producer["outputs"][stage + "-next"]
        assert report["controller"]["path"] == "src/tests/multilayer/tube_tbm_files.py"
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        assert [ref["path"] for ref in report["sources"]] == list(SOURCES)
        for ref in [report["controller"], report["native_loader"], report["binary"], *report["sources"], report["original"], report["baseline"]]:
            checked(ref, cache)
        assert [case["action"] for case in report["cases"]] == ["read", "reload", "next"]
        first_output, assertions, pairs = None, 0, 0
        # Independently recheck every selected native child/log and complete
        # saved comparison;JSON success fields alone cannot establish replay.
        for case in report["cases"]:
            action = case["action"]
            source = first_output if action == "reload" else report["original"]
            assert case["input"] == source
            checked(case["input"], cache)
            raw = checked(case["native"]["log"], cache).read_text(encoding="utf-8")
            assert "NATIVE_EXIT_CODE=0" in raw
            assert "Filters: [.tube-tbm-queue-file]" in raw
            counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", raw)
            assert counts is not None
            assert int(counts[1]) == case["native"]["assertions"] > 0
            assert int(counts[2]) == case["native"]["cases"] == 1
            native_cases += 1
            assertions += case["native"]["assertions"]
            expected_file = report["baseline"] if action == "next" else report["original"]
            comparison = equal_world(checked(expected_file, cache), checked(case["output"], cache))
            assert case["comparisons"]["original_or_full_next"] == comparison
            pairs += 1
            if action != "next":
                repeated = equal_world(ROOT / case["output"]["path"], checked(case["repeat"], cache))
                assert case["comparisons"]["pure_repeat"] == repeated
                pairs += 1
            if action == "read":
                first_output = case["output"]
        assert report["native_cases"] == 3
        assert report["native_assertions"] == assertions
        assert report["complete126_chunk_comparisons"] == pairs == 5
        native_assertions += assertions
        comparisons += pairs
        records.append({"report": identity(path), **report})
        by_phase[key] = report
    assert seen == expected
    assert native_cases == 24 and comparisons == 40
    cross_pairs = 0
    # Exact same original baselines also produce identical actual outputs in
    # both readers;verify these twenty cross-reader whole-world pairs directly.
    for stage in STAGES:
        first, second = (by_phase[(cfg, stage)] for cfg in CONFIGURATIONS)
        for left, right in zip(first["cases"], second["cases"]):
            equal_world(ROOT / left["output"]["path"], ROOT / right["output"]["path"])
            cross_pairs += 1
            if left["repeat"] is not None:
                equal_world(ROOT / left["repeat"]["path"], ROOT / right["repeat"]["path"])
                cross_pairs += 1
    assert cross_pairs == 20
    artwork = identity(ROOT / "media/baseset/openttd.grf")
    assert artwork["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md16D/original16.3;master10.8 actual paid native cutting/spoil/withdrawal/tube construction/useful Surface freight/real queue and paid-grid recovery with complete ordinary files",
        "controller": identity(__file__), "comparison_owner": identity(ROOT / "src/tests/multilayer/tube_tbm_files.py"),
        "selection": identity(selection_path), "native_version": 434, "native_cases": native_cases,
        "native_assertions": native_assertions, "complete126_chunk_original_repeat_next_comparisons": comparisons,
        "complete126_chunk_cross_reader_pairs": cross_pairs, "ignored_chunks_or_fields": [],
        "records": records, "retained_paid_handoffs": handoffs,
        "actual_queue_recovery": queue["actual_queue_recovery"], "artwork": artwork,
        "build_logs": {cfg: identity(ROOT / selection["build_logs"][cfg]) for cfg in CONFIGURATIONS},
        "classification": "Initial generated company/world/commissioned spoil logistics remain disclosed scenario inputs;all subsequent paid excavation,physical disposal,whole withdrawal,final construction,finite useful freight and actual queue/loss/paid repair share the same exact original native owners. No normal-newgame,balance,performance,human or network claim.",
        "remaining": "Return master10.8 for dispatched completion/documentation review,then its next unchecked queue task;whole Plans3/4/M1 and later human/network/operator gates retain their own conditions.",
    }
    output = (ROOT / output_path).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "native_cases": native_cases, "native_assertions": native_assertions,
                      "complete126_chunk_comparisons": comparisons, "complete126_chunk_cross_reader_pairs": cross_pairs}), flush=True)
    return result


def main():
    """Select the complete current file matrix and an absent final technical receipt.

    @return None;stale sources,missing native phases,changed history or any unequal world refuses.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.output)


if __name__ == "__main__":
    main()
