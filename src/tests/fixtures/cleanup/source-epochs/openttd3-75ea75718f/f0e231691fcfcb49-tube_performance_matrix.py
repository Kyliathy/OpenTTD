"""Execute declared physical tube boundaries and focused native producer returns serially.

@file tube_performance_matrix.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DDC.
The maximum account,path and connection limits are independently explicit selections.
"""

import argparse
import json

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_stress import BASELINE, BASELINE_SHA
from bounded_performance import observe
from tube_performance import FIELDS, SOURCES, run_one, verify_measurement


def selections():
    """Fix separate source,repair,full request and native movement loads before observing their budgets."""
    workloads = ((0, 2, 0, 0, 0), (32, 2, 0, 0, 0), (256, 2, 0, 0, 0),
                 (128, 128, 0, 0, 0), (256, 64, 0, 0, 0),
                 (1, 2, 32, 0, 0), (32, 2, 32, 0, 0), (256, 64, 256, 0, 0),
                 (256, 64, 4096, 0, 0), (128, 128, 4096, 128, 0),
                 (32, 2, 32, 0, 1), (32, 2, 32, 128, 1), (256, 2, 256, 128, 1))
    for cfg in ("Debug", "RelWithDebInfo"):
        for values in workloads:
            yield cfg, *values


def main():
    """Checkpoint each native result and preserve every measured budget excess for owning diagnosis."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--prior", action="append", default=[])
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    baseline_ref = identity(ROOT / BASELINE)
    assert baseline_ref["sha256"] == BASELINE_SHA
    baseline = json.loads(verify_reference(baseline_ref).read_text(encoding="utf-8"))
    expected, done, records = set(selections()), set(), []
    for path in args.prior:
        ref = identity(ROOT / path)
        actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))
        key = tuple(actual[field] for field in ("configuration", *FIELDS))
        assert key in expected and key not in done
        verify_measurement(actual, baseline)
        records.append({"key": list(key), "report": ref, "budget_excesses": actual["budget_excesses"]}); done.add(key)
    index = {"controller": identity(__file__), "baseline": baseline_ref, "records": records, "regressions": [], "complete": False}
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    for key in selections():
        if key not in done:
            report = run_one(*key)
            actual = json.loads(report.read_text(encoding="utf-8"))
            records.append({"key": list(key), "report": identity(report), "budget_excesses": actual["budget_excesses"]})
            target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    for cfg in ("Debug", "RelWithDebInfo"):
        for tag in ("[transport-tube-power]", "[transport-tube-meter]"):
            context, report, _ = observe(cfg, tag, {}, SOURCES, "tube-producer-regression", __file__)
            target_report = context.output("reports", f"tube-producer-{cfg}.json")
            target_report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
            index["regressions"].append(identity(target_report))
            target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
            assert report["failure"] is None
    assert len(records) == 26 and len(index["regressions"]) == 4
    index["complete"] = True
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"selection": identity(target), "stress_cases": 26, "excesses": sum(len(row["budget_excesses"]) for row in records)}), flush=True)


if __name__ == "__main__":
    main()
