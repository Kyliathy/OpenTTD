"""Select retained earned first-plant work for actual active/wagon-wait network joins.

@file ground_network_active_inputs.py
@see r2mlvs15-savegame-determinism-and-lifecycle.md Group18B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Only exact receipt-referenced originals are read;no fixture or gameplay state is created.
"""

import argparse
import json
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from construction_provider_return import identity
from save_tables import chunks, read_stream, saved_table

PAID = "logs/geothermal/first-plant/20261017-native-01/verification.json"
PAID_SHA = "0ba8cda739824125f949af3c51d254e0b632a3a7230244cbf47e5549146619ee"
WAIT_REPORT = "mlm/reviews/geothermal/first-plant/20261017-030241-2267816d97/reports/first-plant-Debug-reload.json"
WAIT_SHA = "a1f8ad33521aa26fa69eb51f06fd357b38b3589f692d5ad015fddd7f92616541"


def select_inputs(output_path):
    """Bind exact actual paid active/wait worlds and their native owner/clock identities.

    @param output_path Absent retained input-index JSON beneath repository logs.
    @return Input selection;original430 files remain byte-exact and immutable.
    """
    assert identity(ROOT / PAID)["sha256"] == PAID_SHA
    paid = json.loads((ROOT / PAID).read_text(encoding="utf-8"))
    assert identity(ROOT / WAIT_REPORT)["sha256"] == WAIT_SHA
    wait_report = json.loads((ROOT / WAIT_REPORT).read_text(encoding="utf-8"))
    assert any(row["path"] == WAIT_REPORT for row in paid["accepted_original_chain"])
    wait = next(row for row in wait_report["saves"].values() if row["path"].endswith("first-plant-continued.sav"))
    selected = {"standard-active": paid["fresh_originals"]["Debug"]["standard-partial"],
                "deep-active": paid["fresh_originals"]["Debug"]["deep-partial"], "wagon-wait": wait}
    records = {}
    # Each selected world must be an earned unfinished native job with its
    # original project/hauler/physical wagon references. A completed tube-world
    # cutter or an injected replacement cannot satisfy active-join admission.
    for scenario, reference in selected.items():
        path = ROOT / reference["path"]
        actual = identity(path)
        assert all(actual[key] == reference[key] for key in ("path", "bytes", "sha256"))
        version, stream = read_stream(path)
        assert version == 430
        assert len(list(chunks(stream))) == 124
        projects = saved_table(path, b"TBMP")
        assert set(projects) == {0}
        project = projects[0]
        jobs = saved_table(path, b"XJOB")
        job = next(row for row in jobs.values() if row["id"] == project["job"])
        assert job["company"] == project["company"] == 0
        assert job["state"] == 2
        assert 0 < job["cursor"] < job["routes"]
        haulers = saved_table(path, b"TBXA")
        assert set(haulers) == {0}
        hauler = haulers[0]
        assert hauler["project"] == project["id"]
        assert hauler["hauler"] == 0
        if scenario == "wagon-wait":
            assert project["phase"] == 5
            assert project["blocked"] == 17
            assert hauler["phase"] == 3
            assert hauler["exchanges"] == 5
        else:
            assert project["phase"] == 3
            assert project["blocked"] == 0
            assert job["cursor"] == 1
        records[scenario] = {"input": actual, "source_version": version, "source_chunks": 124,
                             "clock": saved_table(path, b"DATE")[0]["tick_counter"],
                             "project": project, "job": job, "hauler": hauler,
                             "wagon_sets": saved_table(path, b"TBXS"), "wagon_units": saved_table(path, b"TBXW"),
                             "requires_native_resume_before_join": scenario == "wagon-wait"}
    result = {"_documentation": "r2mlvs15-savegame-determinism-and-lifecycle.md18B;exact earned430 paid Standard/Deep unfinished excavation and real WaitForEmpties/loaded hauler originals for current native snapshot transfer;not network acceptance",
              "controller": identity(__file__), "paid_handoff": identity(ROOT / PAID),
              "wait_producer_report": identity(ROOT / WAIT_REPORT), "records": records,
              "remaining": "Current application afterload,public native Resume for the explicitly player-paused wait,actual fresh peer transfer and complete initial/next/agreed-frame comparisons in both configurations."}
    target = (ROOT / output_path).resolve()
    # The input index is new evidence,not permission to rewrite any original
    # or replace an earlier producer receipt.
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"selection": identity(target), "scenarios": list(records)}), flush=True)
    return result


def main():
    """Select only named retained paid originals without executing a native process.

    @return None;missing lineage,phase or ownership refuses the selection.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    select_inputs(args.output)


if __name__ == "__main__":
    main()
