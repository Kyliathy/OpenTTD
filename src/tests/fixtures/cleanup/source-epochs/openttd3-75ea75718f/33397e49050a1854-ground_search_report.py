"""Verify whole-budget planner/current-hardness measurements and exact earlier dense owner evidence.

@file ground_search_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Historical overwritten test/binary bindings stay dated;physical evidence and current production remain exact.
"""

import argparse
import json
import re

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_performance import parse_fields
from ground_stress import percentiles, selected_budget
from ground_search_matrix import DENSE_SHA, selections


def verify_search(actual, baseline):
    """Require real repeated whole queries,pure deterministic outcomes,OS peaks and unchanged earlier budgets."""
    assert actual["kind"] == "search" and actual["status"] == "observed" and actual["failure"] is None
    assert actual["bootstrap_claim"] is False and actual["children_reaped"] is True
    for ref in (actual["controller"], actual["binary"], actual["config"], *actual["sources"]):
        verify_reference(ref)
    cfg, axis, jobs, scenario, hardness = (actual[key] for key in ("configuration", "axis", "jobs", "scenario", "hardness"))
    native = actual["native"]
    assert native["exit_status"] == 0 and native["cases"] == 1
    assert native["argv"] == [str(verify_reference(actual["binary"])), "[.ground-search-performance]"] and native["cwd"] == "build"
    assert native["environment_selectors"] == {"OTTD_GROUND_STRESS_AXIS": str(axis), "OTTD_GROUND_STRESS_JOBS": str(jobs),
                                               "OTTD_GROUND_STRESS_ROUTE": scenario, "OTTD_GROUND_STRESS_HARDNESS": str(hardness)}
    text = verify_reference(native["log"]).read_text(encoding="utf-8")
    match = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
    assert match and int(match[1]) == native["assertions"] > 0 and int(match[2]) == 1
    assert actual["complete"] == parse_fields(text, "GROUND_SEARCH_COMPLETE") and len(actual["complete"]) == 1
    assert actual["samples"] == parse_fields(text, "GROUND_SEARCH_SAMPLE") and [r["index"] for r in actual["samples"]] == list(range(5))
    complete = actual["complete"][0]
    assert (complete["width"], complete["jobs"], complete["scenario"], complete["hardness_version"]) == (axis, jobs, scenario, hardness)
    assert complete["warmup"] == 1 and complete["samples"] == 5 and complete["pure"] == 1
    assert complete["terrain_bytes"] == axis * axis // 2 and complete["depth_limit"] == 256
    assert complete["requested"] == 1 and complete["authorized"] <= 4096
    if scenario == "regional-short":
        assert hardness == 1 and complete["result"] == 0 and complete["partial"] == 0 and complete["segments"] > 0
        assert 0 <= complete["selected_candidate"] < 8
    else:
        assert hardness == 0 and complete["result"] != 0 and complete["expansions"] > 1000
        assert complete["authorized"] == 2009
    assert (complete["state_limit"], complete["expansion_limit"]) == ((8192, 32768) if scenario == "remote-maximum" else (4096, 8192))
    assert complete["expansions"] <= complete["expansion_limit"]
    assert all(r["expansions"] == complete["expansions"] and r["segments"] == complete["segments"] for r in actual["samples"])
    assert actual["timings"] == percentiles([row["ns"] for row in actual["samples"]])
    budget = selected_budget(baseline, cfg, axis)
    assert actual["budget"] == budget and actual["budget_excesses"] == []
    assert actual["timings"]["maximum_ns"] <= budget["route_p99_ns"]
    memory = json.loads(verify_reference(actual["memory_samples"]).read_text(encoding="utf-8"))
    samples = memory["samples"]
    protocol = json.loads(verify_reference(baseline["protocol"]).read_text(encoding="utf-8"))
    assert samples and actual["peak_commit_bytes"] == max(r["process"]["lifetime_peak_commit_bytes"] for r in samples)
    assert actual["peak_commit_bytes"] <= min(protocol["maximum_child_private_bytes"], budget["peak_commit_bytes"])
    assert all(r["system"]["physical_available"] >= protocol["minimum_physical_available_bytes"] and
               r["system"]["commit_available"] >= protocol["minimum_commit_available_bytes"] for r in samples)
    return complete


def accept(selection, checks, output):
    """Close selected dense ground owner/planning scope only after all exact current and historical evidence passes."""
    selection_ref = identity(ROOT / selection)
    index = json.loads(verify_reference(selection_ref).read_text(encoding="utf-8"))
    assert index["complete"] is True and index["dense_subset"]["sha256"] == DENSE_SHA
    verify_reference(index["controller"])
    dense = json.loads(verify_reference(index["dense_subset"]).read_text(encoding="utf-8"))
    baseline = json.loads(verify_reference(dense["baseline"]).read_text(encoding="utf-8"))
    # Exact sealed old observations retain their dated implementation meaning.
    # Their physical report/log/config/memory bytes and unchanged production
    # owners must still exist;today's extended tests/binaries are not old bytes.
    for entry in dense["records"]:
        old = json.loads(verify_reference(entry["report"]).read_text(encoding="utf-8"))
        assert old == entry["actual"]
        for ref in (old["native"]["log"], old["config"], old["memory_samples"]):
            verify_reference(ref)
        for ref in old["sources"]:
            if not ref["path"].startswith("src/tests/"):
                verify_reference(ref)
    expected, records, assertions = set(selections()), {}, 0
    for row in index["records"]:
        key = tuple(row["key"])
        assert key in expected and key not in records and row["budget_excesses"] == []
        actual = json.loads(verify_reference(row["report"]).read_text(encoding="utf-8"))
        assert key == tuple(actual[name] for name in ("configuration", "kind", "axis", "jobs", "scenario", "hardness"))
        assert actual["baseline"] == dense["baseline"]
        complete = verify_search(actual, baseline)
        records[key] = {"report": row["report"], "actual": actual, "counts": complete}
        assertions += actual["native"]["assertions"]
    assert set(records) == expected and len(records) == 20
    original_ref = identity(ROOT / "logs/performance/ground-maps/20261019-native-01/original-performance-verification.json")
    assert original_ref["sha256"] == "5cd7cfe3987218597147bfdbcadc4a596cde33d28fc279ac96e8f309825063bc"
    dated_checks_ref = identity(ROOT / "logs/performance/ground-maps/20261019-native-01/dense-refusal-checks.log")
    assert dated_checks_ref["sha256"] == "0dddec0d048be26b634d16ba41bdf340cad6693c54a8d8e3385a5b7cde69cbb1"
    checks_ref = identity(ROOT / checks)
    data = verify_reference(checks_ref).read_bytes()
    text = data.decode("utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8")
    assert re.search(r"Ran 6 tests in [\d.]+s\s+OK\s*$", text)
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21DC;complete selected dense owner/planner/command scope,not full performance",
              "classification": "ground_dense_owner_planning_acceptance", "bootstrap_claim": False, "controller": identity(__file__),
              "selection": selection_ref, "dense_subset": index["dense_subset"], "baseline": dense["baseline"], "records": list(records.values()),
              "original_full_loop": original_ref, "dated_dense_focused_checks": dated_checks_ref,
              "current_search_native_cases": 20, "current_search_native_assertions": assertions,
              "historical_dense_stress_cases": dense["stress_native_cases"], "historical_dense_stress_assertions": dense["stress_native_assertions"],
              "historical_regression_cases": dense["regression_native_cases"], "historical_regression_assertions": dense["regression_native_assertions"],
              "focused_checks": {"script": identity(ROOT / "src/tests/multilayer/ground_search_checks.py"), "log": checks_ref, "tests": 6},
              "uncertainty": "five full heavy queries report median/observed maximum;p99equalsmaximum,not robust tail estimate;prepared owner workloads differ from real full-loop21DB",
              "remaining": "21DDutility/tube/view scaling;21Eactualfiles/joins;21Fattribution/master11.6"}
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "current_search_cases": 20, "current_search_assertions": assertions}), flush=True)


def main():
    """Consume only the explicit complete query selection and actual focused-test log."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--checks", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.checks, args.output)


if __name__ == "__main__":
    main()
