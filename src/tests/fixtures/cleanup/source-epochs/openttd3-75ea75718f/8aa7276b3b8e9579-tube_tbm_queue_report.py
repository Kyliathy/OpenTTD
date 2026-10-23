"""Seal actual paid-world competing queue,source loss and useful recovery originals.

@file tube_tbm_queue_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group16D.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The next phase must load these same original queue/grant/packet worlds through ordinary readers.
"""

import argparse
import hashlib
import json
import re
import sys

sys.dont_write_bytecode = True
from tube_tbm_queue import ROOT, DELIVERY, SOURCES, STAGES
from construction_provider_return import identity
from portal_consumer_report import CONFIGURATIONS, checked
from save_tables import chunks, read_stream


def accept(selection_path, output_path):
    """Verify both actual current queue/recovery runs and all original/full-next chunks.

    @param selection_path Explicit complete both-reader queue report/build index.
    @param output_path Absent retained original-world handoff beneath logs.
    @return Exact same paid queue/loss/restoration/recovered originals for ordinary file replay.
    """
    selection_path = ROOT / selection_path
    selection = json.loads(selection_path.read_text(encoding="utf-8"))
    assert selection["complete"] is True
    delivery_path = ROOT / DELIVERY[0]
    delivery_ref = identity(delivery_path)
    assert delivery_ref["sha256"] == DELIVERY[1]
    delivery = json.loads(delivery_path.read_text(encoding="utf-8"))
    cache, records, worlds = {}, {}, {}
    expected_outputs = {stage + suffix for stage in STAGES for suffix in ("", "-next")}
    # Every actual current reader must preserve the opposite accepted useful
    # original,earn its own real competing packets/path hold and pay for actual
    # service recovery;prepared queue fixtures cannot substitute this lineage.
    for row in selection["records"]:
        cfg = row["configuration"]
        assert cfg in CONFIGURATIONS and cfg not in records
        path = ROOT / row["report"]
        report = json.loads(path.read_text(encoding="utf-8"))
        assert report["configuration"] == cfg and report["focus"] == "queue"
        assert report["native_version"] == 434 and report["tag"] == "[.tube-tbm-queue-recovery]"
        opposite = next(writer for writer in CONFIGURATIONS if writer != cfg)
        original = delivery["records"][opposite]
        assert report["delivery_receipt"] == delivery_ref
        assert report["delivery_report"] == original["report"]
        assert report["original_input"] == original["outputs"]["useful-final"]
        assert report["original_owners"] == original["original_owners"]
        assert report["controller"]["path"] == "src/tests/multilayer/tube_tbm_queue.py"
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        assert [ref["path"] for ref in report["sources"]] == list(SOURCES)
        for ref in [report["controller"], report["native_loader"], report["binary"], *report["sources"], report["original_input"]]:
            checked(ref, cache)
        raw = checked(report["native"]["log"], cache).read_text(encoding="utf-8")
        assert "NATIVE_EXIT_CODE=0" in raw and "Filters: [.tube-tbm-queue-recovery]" in raw
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", raw)
        assert counts is not None
        assert int(counts[1]) == report["native"]["assertions"] > 0
        assert int(counts[2]) == report["native"]["cases"] == 1
        facts = report["observations"]
        assert facts["winner"] != facts["waiting"]
        assert facts["queued_units"] > 0
        assert facts["useful"] >= original["observations"]["useful"] + facts["queued_units"]
        assert facts["fresh_source"] != facts["old_source"]
        assert facts["fresh_energy"] > 0 and facts["old_energy"] >= original["observations"]["energy"]
        assert set(report["outputs"]) == expected_outputs
        worlds[cfg] = {}
        # Complete physical/cargo/power/reservation owners,including every
        # other native chunk,must match between both independent actual runs.
        for stage, reference in report["outputs"].items():
            version, stream = read_stream(checked(reference, cache))
            assert version == 434
            worlds[cfg][stage] = {item["name"].decode("ascii"): item["raw"] for item in chunks(stream)}
            assert len(worlds[cfg][stage]) == 126
        records[cfg] = {"report": identity(path), **report}
    assert set(records) == set(CONFIGURATIONS)
    assert records["Debug"]["observations"] == records["RelWithDebInfo"]["observations"]
    assert worlds["Debug"] == worlds["RelWithDebInfo"]
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md16D.1;actual original paid-world competing loaded PBS queue,closed paid supply,intact holds,fresh paid reconnection and useful Surface recovery;ordinary original file replay remains16D.2",
        "controller": identity(__file__), "selection": identity(selection_path), "native_version": 434,
        "delivery_handoff": delivery_ref, "records": records,
        "native_cases": 2, "native_assertions": sum(row["native"]["assertions"] for row in records.values()),
        "actual_queue_recovery": records["Debug"]["observations"], "complete_equal126_chunk_pairs": 8,
        "original_chunk_sha256": {stage: {name: hashlib.sha256(raw).hexdigest() for name, raw in data.items()} for stage, data in worlds["Debug"].items()},
        "build_logs": {cfg: identity(ROOT / selection["build_logs"][cfg]) for cfg in CONFIGURATIONS},
        "remaining": "16D.2 ordinary original repeat/opposite-reader/full-next packet/position/grant/reservation acceptance,then16.3/Group16/master10.8 closure;human/network/operator gates remain pending.",
    }
    output = (ROOT / output_path).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "native_assertions": result["native_assertions"], "actual_queue_recovery": result["actual_queue_recovery"]}), flush=True)
    return result


def main():
    """Select exact current original queue observations and an absent immutable handoff.

    @return None;stale sources,altered originals,missing actual queue/paid recovery or unequal worlds refuse.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.output)


if __name__ == "__main__":
    main()
