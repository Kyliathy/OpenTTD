"""Seal exact completed long peer observations before bounding a slower workload's later comparison horizon.

@file network_performance_prefix.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21EC.
This is a dated partial prerequisite,never complete performance acceptance.
"""

import argparse
import json

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from network_performance import inputs
from network_performance_report import verify_measurement


def main():
    """Freeze six exact successful original512-frame pairs and retain the following110s workflow failure."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--failure", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    selection_ref = identity(ROOT / args.selection)
    index = json.loads(verify_reference(selection_ref).read_text(encoding="utf-8"))
    assert index["complete"] is False and len(index["records"]) == 6
    verify_reference(index["controller"])
    baseline = json.loads(verify_reference(index["baseline"]).read_text(encoding="utf-8"))
    expected, records, assertions = inputs(), [], 0
    for row in index["records"]:
        key = tuple(row["key"])
        actual = json.loads(verify_reference(row["report"]).read_text(encoding="utf-8"))
        assert key == (actual["configuration"], actual["label"]) and key in expected
        assert verify_measurement(actual, baseline, expected[key])["budget_excesses"] == []
        records.append({"key": list(key), "report": row["report"], "actual": actual})
        assertions += sum(peer["assertions"] for peer in actual["native"].values())
    failure_ref = identity(ROOT / args.failure)
    failure = json.loads(verify_reference(failure_ref).read_text(encoding="utf-8"))
    assert failure["configuration"] == "Debug" and failure["label"] == "ordinary-1024"
    assert failure["status"] == "failed" and "pair110s deadline exceeded" in failure["failure"] and failure["children_reaped"] is True
    memory = json.loads(verify_reference(failure["memory_samples"]).read_text(encoding="utf-8"))
    events = {row["event"]: row["seconds"] for row in memory["events"]}
    first_work = round((events["client_first_work_snapshots_seen"] - events["client_launched"]) * 1e9)
    assert first_work <= failure["budget"]["join_reconstruction_ns"]
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21EC;dated completed long-horizon subset only",
              "classification": "network_performance_closed_long_subset", "controller": identity(__file__), "selection": selection_ref,
              "baseline": index["baseline"], "file_acceptance": index["file_acceptance"], "records": records,
              "native_cases": 12, "native_assertions": assertions, "peer_pairs": 6,
              "retained_workflow_failure": {"report": failure_ref, "actual": failure, "completed_join_upper_bound_ns": first_work,
                                            "diagnosis": "first native reconstruction/snapshots meet45s;later512-frame stress exceeds110s process policy;new focused8/16/24/32-tick case preserves all6native checkpoints and fixed budgets"},
              "remaining": "ten selected pairs need bounded later-frame observers;complete16pair acceptance remains pending"}
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "peer_pairs": 6, "native_cases": 12, "assertions": assertions,
                      "failed_case_completed_join_upper_bound_ns": first_work}), flush=True)


if __name__ == "__main__":
    main()
