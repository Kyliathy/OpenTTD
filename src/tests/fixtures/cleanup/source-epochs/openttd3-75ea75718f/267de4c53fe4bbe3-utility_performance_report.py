"""Accept complete measured utility bounds without substituting allocator-only evidence.

@file utility_performance_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DDB.
Physical tube work,view comparisons and full performance closure remain separate.
"""

import argparse
import json
import re

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from utility_performance import BASELINE_SHA, verify_measurement
from utility_performance_matrix import selections


def verify_selection(index):
    """Require all current native selections and exact actual producer regressions."""
    assert index["complete"] is True and index["baseline"]["sha256"] == BASELINE_SHA
    verify_reference(index["controller"])
    baseline = json.loads(verify_reference(index["baseline"]).read_text(encoding="utf-8"))
    expected, records, assertions = set(selections()), {}, 0
    for row in index["records"]:
        key = tuple(row["key"])
        assert key in expected and key not in records and row["budget_excesses"] == []
        actual = json.loads(verify_reference(row["report"]).read_text(encoding="utf-8"))
        assert key == tuple(actual[field] for field in ("configuration", "accounts", "fanout")) and actual["baseline"] == index["baseline"]
        verify_measurement(actual, baseline)
        assert actual["budget_excesses"] == []
        records[key] = {"report": row["report"], "actual": actual}
        assertions += actual["native"]["assertions"]
    assert set(records) == expected and len(records) == 16
    regressions, regression_assertions = {}, 0
    protocol = json.loads(verify_reference(baseline["protocol"]).read_text(encoding="utf-8"))
    for ref in index["regressions"]:
        actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))
        cfg = actual["configuration"]
        assert cfg in ("Debug", "RelWithDebInfo") and cfg not in regressions
        for binding in (actual["controller"], actual["binary"], actual["config"], *actual["sources"]):
            verify_reference(binding)
        assert actual["argv"] == [str(verify_reference(actual["binary"])), "[utility-power-performance]"] and actual["cwd"] == "build"
        assert actual["exit_status"] == 0 and actual["children_reaped"] is True and actual["cases"] == 1
        text = verify_reference(actual["log"]).read_text(encoding="utf-8")
        match = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert match and int(match[1]) == actual["assertions"] > 0 and int(match[2]) == 1
        memory = json.loads(verify_reference(actual["memory_samples"]).read_text(encoding="utf-8"))
        assert memory["samples"]
        for sample in memory["samples"]:
            assert sample["system"]["physical_available"] >= protocol["minimum_physical_available_bytes"]
            assert sample["system"]["commit_available"] >= protocol["minimum_commit_available_bytes"]
            assert sample["process"]["lifetime_peak_commit_bytes"] <= protocol["maximum_child_private_bytes"]
        regressions[cfg] = {"report": ref, "actual": actual}
        regression_assertions += actual["assertions"]
    assert set(regressions) == {"Debug", "RelWithDebInfo"}
    return baseline, list(records.values()), list(regressions.values()), assertions, regression_assertions


def accept(selection, checks, output):
    """Seal exact current measurements only after native quantities,budgets and negative boundaries pass."""
    selection_ref = identity(ROOT / selection)
    index = json.loads(verify_reference(selection_ref).read_text(encoding="utf-8"))
    baseline, records, regressions, assertions, regression_assertions = verify_selection(index)
    checks_ref = identity(ROOT / checks)
    data = verify_reference(checks_ref).read_bytes()
    text = data.decode("utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8")
    assert re.search(r"Ran 8 tests in [\d.]+s\s+OK\s*$", text)
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21DDB;shared utility allocation/settlement/expiry/retirement only",
              "classification": "shared_utility_performance_acceptance", "bootstrap_claim": False, "controller": identity(__file__),
              "selection": selection_ref, "baseline": index["baseline"], "records": records, "regressions": regressions,
              "native_cases": 16, "native_assertions": assertions, "regression_cases": 2, "regression_assertions": regression_assertions,
              "focused_checks": {"script": identity(ROOT / "src/tests/multilayer/utility_performance_checks.py"), "log": checks_ref, "tests": 8},
              "scope": "0/32/256/4096 real sources and consumers;1/4 explicit eligibility sources;16384 eligibility edge ceiling,zero physical links;6settled+1expired+1retired intervals",
              "uncertainty": "five equal measured intervals after one full warmup;nearest-rank p99equalsmaximum,not robust tail;OSpeaks distinct from retained payload;headless prepared owners",
              "remaining": "21DDC physical tubes;21DDD view/combined;21E files/realjoins;21F/master11.6"}
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "cases": 18, "assertions": assertions + regression_assertions}), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--checks", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.checks, args.output)
