"""Verify actual dense-owner stress samples without turning short route queries into a full planner bound.

@file ground_stress_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native logs,count limits,pure owner checks,OS peaks and frozen budgets remain authoritative.
"""

import argparse
import json
import re

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_performance import parse_fields
from ground_stress import BASELINE_SHA, TAGS, percentiles, selected_budget
from ground_stress_matrix import selections


def verify_record(actual, baseline):
    """Recheck one real native observation against actual stdout,resource samples and prior budget."""
    cfg, kind, axis, jobs, scenario = (actual[name] for name in ("configuration", "kind", "axis", "jobs", "scenario"))
    assert actual["status"] == "observed" and actual["failure"] is None and actual["children_reaped"] is True
    assert actual["classification"] == "prepared_ground_owner_performance" and actual["bootstrap_claim"] is False
    for ref in (actual["controller"], actual["binary"], actual["config"], *actual["sources"]):
        verify_reference(ref)
    native = actual["native"]
    assert native["argv"] == [str(verify_reference(actual["binary"])), TAGS[kind]] and native["cwd"] == "build"
    assert native["environment_selectors"] == {"OTTD_GROUND_STRESS_AXIS": str(axis), "OTTD_GROUND_STRESS_JOBS": str(jobs), "OTTD_GROUND_STRESS_ROUTE": scenario}
    assert native["exit_status"] == 0 and native["cases"] == 1
    text = verify_reference(native["log"]).read_text(encoding="utf-8")
    match = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
    assert match and int(match[1]) == native["assertions"] > 0 and int(match[2]) == 1
    marker = {"job": "GROUND_JOB", "route": "GROUND_ROUTE", "command": "GROUND_COMMAND"}[kind]
    assert actual["complete"] == parse_fields(text, marker + "_COMPLETE") and len(actual["complete"]) == 1
    assert actual["samples"] == parse_fields(text, marker + "_SAMPLE")
    complete = actual["complete"][0]
    assert complete["warmup"] == 1 and complete["intervals"] == 5
    budget = selected_budget(baseline, cfg, axis)
    assert actual["budget"] == budget and actual["budget_excesses"] == []
    if kind == "job":
        assert complete["width"] == complete["height"] == axis and complete["jobs"] == jobs
        assert complete["claims"] == jobs * 9 and complete["partial_ticks"] == 384
        assert complete["generated_earth"] == complete["generated_rock"] == complete["cash_changed"] == 0
        assert complete["terrain_bytes"] == axis * axis // 2 and complete["map_packed_payload_estimate"] == axis * axis * 48
        assert complete["reserve_refused"] == int(jobs == 128) and complete["rounds"] == 64
        assert len(actual["samples"]) == 640
        for operation, calls, limit in (("work", jobs, "simulation_p99_ns"), ("reserve-estimate", 1, "command_p99_ns")):
            rows = [row for row in actual["samples"] if row["operation"] == operation]
            assert [row["index"] for row in rows] == list(range(320)) and all(row["calls"] == calls for row in rows)
            assert actual["timings"][operation] == percentiles([row["ns"] for row in rows])
            assert actual["timings"][operation]["p99_ns"] <= budget[limit]
        assert actual["timings"]["work"]["maximum_ns"] <= budget["simulation_max_ns"]
        assert actual["lookup"] == parse_fields(text, "GROUND_JOB_LOOKUP") and len(actual["lookup"]) == 5
        assert [row["index"] for row in actual["lookup"]] == list(range(5))
        assert all(row["queries"] == 10000 and row["hits"] == actual["lookup"][0]["hits"] for row in actual["lookup"])
        assert max(row["ns"] for row in actual["lookup"]) <= budget["route_p99_ns"]
    else:
        count = 80 if kind == "route" else 320
        assert len(actual["samples"]) == count and complete["samples"] == count
        assert [row["index"] for row in actual["samples"]] == list(range(count))
        assert actual["timings"] == percentiles([row["ns"] for row in actual["samples"]])
        assert actual["timings"]["p99_ns"] <= budget["route_p99_ns" if kind == "route" else "command_p99_ns"]
        assert complete["pure"] == 1
        if kind == "route":
            assert complete["width"] == axis and complete["jobs"] == jobs and complete["scenario"] == scenario
            assert complete["rounds"] == 16 and complete["terrain_bytes"] == axis * axis // 2
            assert complete["expansions"] <= (1 if scenario == "limited" else 8192)
            assert all(row["expansions"] == complete["expansions"] and row["segments"] == complete["segments"] for row in actual["samples"])
            if scenario in ("straight", "irregular"):
                assert complete["result"] == 0 and complete["partial"] == 0 and complete["segments"] > 0
            else:
                assert complete["result"] != 0
        else:
            assert complete["operation"] == "pause-estimate" and complete["rounds"] == 64
            assert all(row["cost"] == 0 for row in actual["samples"])
    memory = json.loads(verify_reference(actual["memory_samples"]).read_text(encoding="utf-8"))
    samples = memory["samples"]
    protocol = json.loads(verify_reference(baseline["protocol"]).read_text(encoding="utf-8"))
    assert samples and actual["peak_commit_bytes"] == max(row["process"]["lifetime_peak_commit_bytes"] for row in samples)
    assert actual["peak_working_bytes"] == max(row["process"]["lifetime_peak_working_bytes"] for row in samples)
    assert actual["peak_commit_bytes"] <= min(protocol["maximum_child_private_bytes"], budget["peak_commit_bytes"])
    assert all(row["system"]["physical_available"] >= protocol["minimum_physical_available_bytes"] and
               row["system"]["commit_available"] >= protocol["minimum_commit_available_bytes"] for row in samples)
    return complete


def accept(selection, output):
    """Seal the complete explicit68-case subset and ten existing focused producer cases."""
    selection_ref = identity(ROOT / selection)
    index = json.loads(verify_reference(selection_ref).read_text(encoding="utf-8"))
    assert index["complete"] is True and index["baseline"]["sha256"] == BASELINE_SHA
    verify_reference(index["controller"])
    baseline = json.loads(verify_reference(index["baseline"]).read_text(encoding="utf-8"))
    expected, records, assertions = set(selections()), {}, 0
    for row in index["records"]:
        key = tuple(row["key"])
        assert key in expected and key not in records and row["budget_excesses"] == []
        actual = json.loads(verify_reference(row["report"]).read_text(encoding="utf-8"))
        assert key == tuple(actual[name] for name in ("configuration", "kind", "axis", "jobs", "scenario"))
        assert actual["baseline"] == index["baseline"]
        complete = verify_record(actual, baseline)
        records[key] = {"report": row["report"], "actual": actual, "counts": complete}
        assertions += actual["native"]["assertions"]
    assert set(records) == expected and len(records) == 68
    regressions = []
    for ref in index["regressions"]:
        actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))
        assert actual["configuration"] not in {r["actual"]["configuration"] for r in regressions}
        for reference in (actual["controller"], actual["binary"], actual["config"], actual["memory_samples"], *actual["sources"]):
            verify_reference(reference)
        text = verify_reference(actual["log"]).read_text(encoding="utf-8")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) == actual["native_assertions"] > 0 and int(counts[2]) == actual["native_cases"] == 5
        assert actual["children_reaped"] is True and actual["exit_status"] == 0
        regressions.append({"report": ref, "actual": actual})
    assert {r["actual"]["configuration"] for r in regressions} == {"Debug", "RelWithDebInfo"}
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21DC;bounded dense owner/short-query subset;near-limit planner remains pending",
              "classification": "prepared_ground_owner_bound", "bootstrap_claim": False, "controller": identity(__file__),
              "selection": selection_ref, "baseline": index["baseline"], "records": list(records.values()), "regressions": regressions,
              "stress_native_cases": 68, "stress_native_assertions": assertions,
              "regression_native_cases": 10, "regression_native_assertions": sum(r["actual"]["native_assertions"] for r in regressions),
              "scope": "explicit uniform-rock0/32/128job owners,512/1024/2048maps,pure short/irregular/tight/1-state-limited route queries,native acquired-project pause estimates",
              "remaining": "near-limit/default-budget planner/current hardness measurement before21DC/21D.1close;21DDutility/tube/view scaling;21Efiles/joins;21Fbudget attribution"}
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "stress_native_cases": 68, "stress_native_assertions": assertions,
                      "regression_native_cases": 10, "regression_native_assertions": result["regression_native_assertions"]}), flush=True)


def main():
    """Verify the explicit actual dense selection without replacing any retained observation."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.output)


if __name__ == "__main__":
    main()
