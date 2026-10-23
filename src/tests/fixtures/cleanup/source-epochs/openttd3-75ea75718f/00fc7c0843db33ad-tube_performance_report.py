"""Accept complete physical tube performance only after every fixed budget passes.

@file tube_performance_report.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DDC.
No receipt erases a prior failed workload or substitutes a calculated demand for native work.
"""

import argparse
import json
import re

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_stress import BASELINE_SHA
from bounded_performance import verify_observation
from tube_performance import FIELDS, verify_measurement
from tube_performance_matrix import selections


def verify_selection(index):
    """Require exact full selection,physical conservation and real focused producer returns."""
    assert index["complete"] is True and index["baseline"]["sha256"] == BASELINE_SHA
    verify_reference(index["controller"])
    baseline = json.loads(verify_reference(index["baseline"]).read_text(encoding="utf-8"))
    expected, records, assertions = set(selections()), {}, 0
    for row in index["records"]:
        key = tuple(row["key"])
        assert key in expected and key not in records and row["budget_excesses"] == []
        actual = json.loads(verify_reference(row["report"]).read_text(encoding="utf-8"))
        assert key == tuple(actual[field] for field in ("configuration", *FIELDS)) and actual["baseline"] == index["baseline"]
        verify_measurement(actual, baseline)
        assert actual["budget_excesses"] == []
        records[key] = {"report": row["report"], "actual": actual}
        assertions += actual["native"]["assertions"]
    assert set(records) == expected and len(records) == 26
    regressions, regression_assertions, regression_cases = {}, 0, 0
    for ref in index["regressions"]:
        actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))
        cfg, tag = actual["configuration"], actual["native"]["argv"][1]
        assert cfg in ("Debug", "RelWithDebInfo") and tag in ("[transport-tube-power]", "[transport-tube-meter]")
        assert (cfg, tag) not in regressions
        verify_observation(actual, baseline, tag, {})
        regressions[cfg, tag] = {"report": ref, "actual": actual}
        regression_assertions += actual["native"]["assertions"]; regression_cases += actual["native"]["cases"]
    assert len(regressions) == 4
    return list(records.values()), list(regressions.values()), assertions, regression_assertions, regression_cases


def accept(selection, checks, output):
    """Seal physical sources,actual service/movement and declared partial-job work separately from later views/files/joins."""
    selection_ref = identity(ROOT / selection)
    index = json.loads(verify_reference(selection_ref).read_text(encoding="utf-8"))
    records, regressions, assertions, regression_assertions, regression_cases = verify_selection(index)
    checks_ref = identity(ROOT / checks)
    data = verify_reference(checks_ref).read_bytes()
    text = data.decode("utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8")
    assert re.search(r"Ran 8 tests in [\d.]+s\s+OK\s*$", text)
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21DDC;bounded physical sources/capsules/declared partial jobs",
              "classification": "physical_tube_performance_acceptance", "bootstrap_claim": False, "controller": identity(__file__),
              "selection": selection_ref, "baseline": index["baseline"], "records": records, "regressions": regressions,
              "native_cases": 26, "native_assertions": assertions, "regression_cases": regression_cases, "regression_assertions": regression_assertions,
              "focused_checks": {"script": identity(ROOT / "src/tests/multilayer/tube_performance_checks.py"), "log": checks_ref, "tests": 8},
              "scope": "256physicalconnections;128cellsperpath;16384aggregatecells;4096actualcapsulebindings;all5models9..13;native repairs with funded/waiting counts;actual ordered movement;0/128partial jobs",
              "uncertainty": "prepared floor/Connected portals/funding/boundary-loaded packets;full warmup then5equalintervals;all320measuredmotionticks or5repair/source ticks retained;small heavy sample p99equalsmax;headless native work,not raster timing",
              "remaining": "21DDD actual view/combined acceptance;21E files/realjoins;21F/master11.6"}
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "cases": 26 + regression_cases, "assertions": assertions + regression_assertions}), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--checks", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.checks, args.output)
