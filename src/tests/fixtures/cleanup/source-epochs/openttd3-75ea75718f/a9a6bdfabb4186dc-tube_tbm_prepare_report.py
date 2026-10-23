"""Seal actual paid-world mine construction,finite production and first capsule loading.

@file tube_tbm_prepare_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group16C.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Loaded native cargo precedes the following useful transfer/Surface acceptance phase.
"""

import argparse
import hashlib
import json
import re
import sys

sys.dont_write_bytecode = True
from tube_tbm_useful import ROOT, SOURCES
from construction_provider_return import identity
from portal_consumer_report import CONFIGURATIONS, checked
from save_tables import chunks, read_stream

CLEARANCE = ("logs/tubes/tbm-final-use/20261019-native-01/clearance-verification-2.json",
             "b3709dadebb2ceb998b2d7bef7a038a96e7b8287ca1f7c57c5265b0f0383a226")


def accept(selection_path, output_path):
    """Verify both current native loaded preparations and unchanged original paid lineage.

    @param selection_path Explicit completed both-reader preparation/build index.
    @param output_path Absent retained acceptance receipt beneath logs.
    @return Exact original loaded-ready worlds for subsequent useful physical delivery.
    """
    selection_path = ROOT / selection_path
    selection = json.loads(selection_path.read_text(encoding="utf-8"))
    assert selection["complete"] is True
    clearance_path = ROOT / CLEARANCE[0]
    clearance_ref = identity(clearance_path)
    assert clearance_ref["sha256"] == CLEARANCE[1]
    clearance = json.loads(clearance_path.read_text(encoding="utf-8"))
    cache, records, worlds = {}, {}, {}
    # Each reader must load the exact opposite original paid world and earn
    # native machinery,finite output and loading independently;prepared stock
    # or another report cannot stand in for this same-world continuation.
    for row in selection["records"]:
        cfg = row["configuration"]
        assert cfg in CONFIGURATIONS and cfg not in records
        path = ROOT / row["report"]
        report = json.loads(path.read_text(encoding="utf-8"))
        assert report["configuration"] == cfg and report["focus"] == "prepare"
        assert report["native_version"] == 434
        assert report["tag"] == "[.tube-tbm-useful-prepare]"
        opposite = next(writer for writer in CONFIGURATIONS if writer != cfg)
        original = clearance["records"][opposite]
        assert report["clearance_receipt"] == clearance_ref
        assert report["clearance_report"] == original["report"]
        assert report["original_input"] == original["original_ready"]
        assert report["original_owners"] == original["owners"]
        assert report["original_paid_observations"] == original["metrics"]
        assert report["controller"]["path"] == "src/tests/multilayer/tube_tbm_useful.py"
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        assert [ref["path"] for ref in report["sources"]] == list(SOURCES)
        for ref in [report["controller"], report["binary"], *report["sources"], report["original_input"]]:
            checked(ref, cache)
        raw = checked(report["native"]["log"], cache).read_text(encoding="utf-8")
        assert "NATIVE_EXIT_CODE=0" in raw and "Filters: [.tube-tbm-useful-prepare]" in raw
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", raw)
        assert counts is not None
        assert int(counts[1]) == report["native"]["assertions"] > 0
        assert int(counts[2]) == report["native"]["cases"] == 1
        observations = report["observations"]
        assert observations["loaded"] > 0
        assert observations["opening"] == observations["remaining"] + observations["extracted"]
        assert observations["extracted"] >= observations["loaded"]
        assert 0 < observations["cash"] < report["original_owners"]["cash"]
        version, stream = read_stream(checked(report["output"], cache))
        assert version == 434
        worlds[cfg] = {item["name"].decode("ascii"): item["raw"] for item in chunks(stream)}
        assert len(worlds[cfg]) == 126
        records[cfg] = {"report": identity(path), **report}
    assert set(records) == set(CONFIGURATIONS)
    assert records["Debug"]["observations"] == records["RelWithDebInfo"]["observations"]
    assert worlds["Debug"] == worlds["RelWithDebInfo"]
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md16C.1;actual original paid TBM floor,paid native mine/conventional shaft/receiver/fleet/external service and genuine finite production/native capsule loading;Surface useful transfer remains next",
        "controller": identity(__file__), "selection": identity(selection_path), "native_version": 434,
        "native_cases": 2, "native_assertions": sum(row["native"]["assertions"] for row in records.values()),
        "clearance_handoff": clearance_ref, "records": records,
        "original_owners": records["Debug"]["original_owners"], "observations": records["Debug"]["observations"],
        "complete_equal_loaded_ready_chunks": 126,
        "loaded_ready_chunk_sha256": {name: hashlib.sha256(raw).hexdigest() for name, raw in worlds["Debug"].items()},
        "build_logs": {cfg: identity(ROOT / selection["build_logs"][cfg]) for cfg in CONFIGURATIONS},
        "remaining": "Continue16C.2 from the exact loaded-ready worlds through useful transfer/Surface delivery/whole empty returns,then16D actual queue/files/paid-grid recovery;whole master and operator gates stay pending.",
    }
    output = (ROOT / output_path).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "native_assertions": result["native_assertions"], "complete_equal_loaded_ready_chunks": 126}), flush=True)
    return result


def main():
    """Select an exact both-reader preparation matrix and absent immutable handoff.

    @return None;stale native owners,altered originals,unearned stock or unequal worlds refuse.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.output)


if __name__ == "__main__":
    main()
