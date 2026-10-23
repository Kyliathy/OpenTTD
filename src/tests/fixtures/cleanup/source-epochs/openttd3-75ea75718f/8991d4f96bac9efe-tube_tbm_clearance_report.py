"""Seal actual non-rectangular paid clearing and both exact original tube construction worlds.

@file tube_tbm_clearance_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group16B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md Group21.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Original useful freight and queue/power file continuation remain the following groups.
"""

import argparse
import hashlib
import json
import re
import sys

sys.dont_write_bytecode = True
from tube_tbm_return import CASES, SOURCES, ROOT
from construction_provider_return import identity
from portal_consumer_report import CONFIGURATIONS, checked
from save_tables import chunks, read_stream
from tube_tbm_useful import SOURCES as USEFUL_SOURCES

PREVIOUS = ("logs/tbm/nonrail-reuse/20261019-native-01/verification.json",
            "ebd7968021620078041ead008c1441aa95ff99417686bbbc0d700c80ca005516")


def accept(selection_path, output_path):
    """Verify both current clearing cases and every complete original native save chunk.

    @param selection_path Exact both-reader report/build index, never a latest-run search.
    @param output_path Absent retained acceptance receipt beneath logs.
    @return Complete original paid construction handoff for subsequent ordinary-loaded freight.
    """
    selection_path = ROOT / selection_path
    selection = json.loads(selection_path.read_text(encoding="utf-8"))
    assert selection["complete"] is True
    seen, cache, records, worlds = set(), {}, {}, {}
    # Each real reader must independently earn the whole same paid excavation
    # and construction. A source-matching report alone cannot substitute a run.
    for row in selection["records"]:
        cfg = row["configuration"]
        assert cfg in CONFIGURATIONS and cfg not in seen
        seen.add(cfg)
        path = ROOT / row["report"]
        report = json.loads(path.read_text(encoding="utf-8"))
        assert (report["configuration"], report["focus"], report["tag"]) == (cfg, "clearing", CASES["clearing"])
        assert report["native_version"] == 434
        assert report["controller"]["path"] == "src/tests/multilayer/tube_tbm_return.py"
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        assert [ref["path"] for ref in report["sources"]] == list(SOURCES)
        for ref in [report["controller"], report["boundary_runner"], report["binary"], *report["sources"]]:
            checked(ref, cache)
        raw = checked(report["native"]["log"], cache).read_text(encoding="utf-8")
        assert "NATIVE_EXIT_CODE=0" in raw and f"Filters: {CASES['clearing']}" in raw
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", raw)
        assert counts is not None
        assert int(counts[1]) == report["native"]["assertions"] > 0
        assert int(counts[2]) == report["native"]["cases"] == 1
        assert report["metrics"]["extensions"] == 1
        assert min(report["metrics"].values()) > 0
        version, stream = read_stream(checked(report["original_ready"], cache))
        assert version == 434
        worlds[cfg] = {row["name"].decode("ascii"): row["raw"] for row in chunks(stream)}
        assert len(worlds[cfg]) == 126
        records[cfg] = {"report": identity(path), **report}
    assert seen == set(CONFIGURATIONS)
    assert records["Debug"]["metrics"] == records["RelWithDebInfo"]["metrics"]
    assert records["Debug"]["owners"] == records["RelWithDebInfo"]["owners"]
    assert worlds["Debug"] == worlds["RelWithDebInfo"]
    previous = identity(ROOT / PREVIOUS[0])
    assert previous["sha256"] == PREVIOUS[1]
    catalogue = {}
    # The explicit catalogue return remains optional for dated16B-only
    # reproduction. When supplied,both ordinary opposite readers must pass;
    # no fixture availability repair can fill an omitted native date probe.
    for row in selection.get("catalogue_records", []):
        cfg = row["configuration"]
        assert cfg in CONFIGURATIONS and cfg not in catalogue
        path = ROOT / row["report"]
        report = json.loads(path.read_text(encoding="utf-8"))
        assert report["configuration"] == cfg
        assert report["focus"] == "catalogue" and report["tag"] == "[.tube-tbm-catalogue]"
        assert report["native_version"] == 434
        opposite = next(writer for writer in CONFIGURATIONS if writer != cfg)
        assert report["construction_report"] == records[opposite]["report"]
        assert report["original_input"] == records[opposite]["original_ready"]
        assert report["controller"]["path"] == "src/tests/multilayer/tube_tbm_catalogue.py"
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        assert [ref["path"] for ref in report["sources"]] == list(USEFUL_SOURCES)
        for ref in [report["controller"], report["native_loader"], report["binary"], *report["sources"], report["original_input"]]:
            checked(ref, cache)
        raw = checked(report["native"]["log"], cache).read_text(encoding="utf-8")
        assert "NATIVE_EXIT_CODE=0" in raw
        assert "Filters: [.tube-tbm-catalogue]" in raw
        assert "TUBE_TBM_CATALOGUE native_public_dates=1 future_locked=1 unchanged_world=1" in raw
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", raw)
        assert counts is not None
        assert int(counts[1]) == report["native"]["assertions"] > 0
        assert int(counts[2]) == report["native"]["cases"] == 1
        catalogue[cfg] = {"report": identity(path), **report}
    if catalogue:
        assert set(catalogue) == set(CONFIGURATIONS)
    result = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md16B/original16.1;Plan4-21.4 actual tube replacement;paid L-shaped native cuts/spoil/withdrawal before actual curved tube construction and exact original-world handoff",
        "controller": identity(__file__), "selection": identity(selection_path), "native_version": 434,
        "native_cases": 2 + len(catalogue), "native_assertions": sum(row["native"]["assertions"] for row in [*records.values(), *catalogue.values()]),
        "records": records, "actual_paid_observations": records["Debug"]["metrics"],
        "original_owners": records["Debug"]["owners"], "complete_equal_original_chunks": 126,
        "original_chunk_sha256": {name: hashlib.sha256(raw).hexdigest() for name, raw in worlds["Debug"].items()},
        "build_logs": {cfg: identity(ROOT / selection["build_logs"][cfg]) for cfg in CONFIGURATIONS},
        "retained_safe_reuse_paid_plant_handoff": previous,
        "catalogue_probes": catalogue,
        "classification": "Generated company/world/commissioned initial spoil logistics are disclosed inputs;all actual future tube floor,work,spoil,withdrawal and native construction are paid original-world behavior. No painted target floor,private movement or supplied cargo establishes this acceptance.",
        "remaining": "Continue Plan17-16C useful actual cargo then16D original queue/files/paid-grid loss recovery;whole master10.8 and human/network/operator gates remain unchecked.",
    }
    output = (ROOT / output_path).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "native_assertions": result["native_assertions"], "complete_equal_original_chunks": 126}), flush=True)
    return result


def main():
    """Select original both-reader construction evidence and an absent receipt.

    @return None; altered sources,missing work,unequal worlds or stale inputs refuse.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.output)


if __name__ == "__main__":
    main()
