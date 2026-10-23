"""Seal useful Surface freight and complete original worlds from actual paid TBM floor.

@file tube_tbm_delivery_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group16C.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Queue/reservation reload and paid-grid recovery remain the following original-world phase.
"""

import argparse
import hashlib
import json
import re
import sys

sys.dont_write_bytecode = True
from tube_tbm_delivery import ROOT, PREPARATION, SOURCES
from construction_provider_return import identity
from portal_consumer_report import CONFIGURATIONS, checked
from save_tables import chunks, read_stream


def accept(selection_path, output_path):
    """Verify exact both-reader useful delivery and every complete final/full-next chunk.

    @param selection_path Explicit complete original delivery report/build index.
    @param output_path Absent retained receipt beneath repository logs.
    @return Same-world paid useful-route handoff preserving all original cargo/floor/spoil owners.
    """
    selection_path = ROOT / selection_path
    selection = json.loads(selection_path.read_text(encoding="utf-8"))
    assert selection["complete"] is True
    prep_path = ROOT / PREPARATION[0]
    prep_ref = identity(prep_path)
    assert prep_ref["sha256"] == PREPARATION[1]
    prepared = json.loads(prep_path.read_text(encoding="utf-8"))
    cache, records, worlds = {}, {}, {}
    # Both actual readers must continue the opposite sealed loaded-cargo
    # original. Repeated construction or another useful route cannot replace
    # this actual paid excavation lineage and physical Surface settlement.
    for row in selection["records"]:
        cfg = row["configuration"]
        assert cfg in CONFIGURATIONS and cfg not in records
        path = ROOT / row["report"]
        report = json.loads(path.read_text(encoding="utf-8"))
        assert report["configuration"] == cfg and report["focus"] == "delivery"
        assert report["native_version"] == 434
        assert report["tag"] == "[.tube-tbm-useful-delivery]"
        opposite = next(writer for writer in CONFIGURATIONS if writer != cfg)
        original = prepared["records"][opposite]
        assert report["preparation_receipt"] == prep_ref
        assert report["preparation_report"] == original["report"]
        assert report["original_input"] == original["output"]
        assert report["original_owners"] == original["original_owners"]
        assert report["useful_participants"] == original["observations"]
        assert report["controller"]["path"] == "src/tests/multilayer/tube_tbm_delivery.py"
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        assert [ref["path"] for ref in report["sources"]] == list(SOURCES)
        for ref in [report["controller"], report["native_loader"], report["binary"], *report["sources"], report["original_input"]]:
            checked(ref, cache)
        raw = checked(report["native"]["log"], cache).read_text(encoding="utf-8")
        assert "NATIVE_EXIT_CODE=0" in raw and "Filters: [.tube-tbm-useful-delivery]" in raw
        assert "capsule_return=1 freight_return=1 crossed_surface=1" in raw
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", raw)
        assert counts is not None
        assert int(counts[1]) == report["native"]["assertions"] > 0
        assert int(counts[2]) == report["native"]["cases"] == 1
        observations = report["observations"]
        assert min(observations.values()) > 0
        assert observations["useful"] >= original["observations"]["loaded"]
        assert observations["remaining"] + observations["extracted"] == original["observations"]["opening"]
        assert set(report["outputs"]) == {"useful-final", "useful-final-next"}
        worlds[cfg] = {}
        # Keep the actual useful final and its untouched whole next native
        # opportunity separately;no chunk or revision field is ignored here.
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
        "_documentation": "r2mlvs17-underground-transport-tubes.md16C/original16.2;actual original paid non-rectangular TBM clearing,spoil completion,whole withdrawal and tube construction followed by native finite cargo transfer/useful Surface freight/whole empty returns",
        "controller": identity(__file__), "selection": identity(selection_path), "native_version": 434,
        "preparation_handoff": prep_ref, "records": records,
        "native_cases": 2, "native_assertions": sum(row["native"]["assertions"] for row in records.values()),
        "actual_useful_observations": records["Debug"]["observations"],
        "complete_equal126_chunk_pairs": 2,
        "original_chunk_sha256": {stage: {name: hashlib.sha256(raw).hexdigest() for name, raw in data.items()} for stage, data in worlds["Debug"].items()},
        "build_logs": {cfg: identity(ROOT / selection["build_logs"][cfg]) for cfg in CONFIGURATIONS},
        "classification": "All subsequent machinery/source/route/fleet are paid native commands in the exact original excavated world. Native StateGameLoop ordering includes clock,tile loop,vehicles and landscape station/industry/company/link maintenance;no floor,stock,rating,arrivals or private phases are supplied.",
        "remaining": "Plan17-16D actual original competing queues/reservations,ordinary files/full-next ledgers and paid-grid loss/restoration;whole master10.8 and human/network/operator gates remain pending.",
    }
    output = (ROOT / output_path).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "native_assertions": result["native_assertions"], "actual_useful_observations": result["actual_useful_observations"]}), flush=True)
    return result


def main():
    """Select exact current both-reader actual useful deliveries and an absent receipt.

    @return None;changed originals,stale sources,missing useful income/returns or unequal files refuse.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.output)


if __name__ == "__main__":
    main()
