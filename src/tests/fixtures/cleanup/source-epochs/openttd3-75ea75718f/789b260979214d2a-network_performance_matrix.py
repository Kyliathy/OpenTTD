"""Run every selected real peer reconstruction sequentially,one guarded pair at a time.

@file network_performance_matrix.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21EC.
Exact current probes are reusable;every later native pair retains its actual first-work timing and peaks.
"""

import argparse
import json

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_stress import BASELINE, BASELINE_SHA
from network_performance import FILES, FILES_SHA, inputs, run_one
from network_performance_report import PREFIX, PREFIX_SHA, verify_long_prefix, verify_measurement


def main():
    """Checkpoint the complete declared16pair set without dropping slow or failed measurements."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--prior", action="append", default=[])
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    baseline_ref, file_ref = identity(ROOT / BASELINE), identity(ROOT / FILES)
    assert baseline_ref["sha256"] == BASELINE_SHA and file_ref["sha256"] == FILES_SHA
    baseline = json.loads(verify_reference(baseline_ref).read_text(encoding="utf-8"))
    expected, done, records = inputs(), set(), []
    prefix_ref = identity(ROOT / PREFIX)
    assert prefix_ref["sha256"] == PREFIX_SHA
    historical = verify_long_prefix(prefix_ref, expected)
    for key, entry in historical.items():
        actual = entry["actual"]
        records.append({"key": list(key), "report": entry["report"], "historical": True,
                        "join_upper_bound_ns": actual["join_upper_bound_ns"], "budget_excesses": actual["budget_excesses"]}); done.add(key)
    for path in args.prior:
        ref = identity(ROOT / path); actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))
        key = actual["configuration"], actual["label"]
        assert key in expected and key not in done and actual["baseline"] == baseline_ref
        observed = verify_measurement(actual, baseline, expected[key])
        records.append({"key": list(key), "report": ref, "historical": False, **observed}); done.add(key)
    index = {"controller": identity(__file__), "baseline": baseline_ref, "file_acceptance": file_ref, "historical_prefix": prefix_ref, "records": records, "complete": False}
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    for key in sorted(expected):
        if key not in done:
            report = run_one(*key)
            actual = json.loads(report.read_text(encoding="utf-8"))
            observed = verify_measurement(actual, baseline, expected[key])
            records.append({"key": list(key), "report": identity(report), "historical": False, **observed})
            target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    assert len(records) == 16
    index["complete"] = True
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"selection": identity(target), "peer_pairs": 16, "budget_excesses": sum(len(row["budget_excesses"]) for row in records)}), flush=True)


if __name__ == "__main__":
    main()
