"""Measure whole native default/maximum searches and admitted current-hardness routes serially.

@file ground_search_matrix.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Earlier dense short-query evidence remains an exact dated prerequisite.
"""

import argparse
import json

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_stress import run_one

DENSE = "logs/performance/ground-maps/20261019-native-01/dense-owner-verification.json"
DENSE_SHA = "3852f24153ff5c60eeab42bb29df37d8b1a77d482d074b95f00a01001d408b04"


def selections():
    """Declare full queries at small/large maps and empty/maximum job density,plus current-hardness success."""
    for cfg in ("Debug", "RelWithDebInfo"):
        for axis in (512, 2048):
            for jobs in (0, 128):
                for scenario in ("remote-default", "remote-maximum"):
                    yield cfg, "search", axis, jobs, scenario, 0
            yield cfg, "search", axis, 128, "regional-short", 1


def main():
    """Resume only exact current successful probes;retain any measured budget excess for owning diagnosis."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--prior", action="append", default=[])
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    dense_ref = identity(ROOT / DENSE)
    assert dense_ref["sha256"] == DENSE_SHA
    expected, records, done = set(selections()), [], set()
    for name in args.prior:
        reference = identity(ROOT / name)
        actual = json.loads(verify_reference(reference).read_text(encoding="utf-8"))
        key = tuple(actual[field] for field in ("configuration", "kind", "axis", "jobs", "scenario", "hardness"))
        assert key in expected and key not in done and actual["status"] == "observed" and actual["failure"] is None
        for ref in (actual["controller"], actual["binary"], actual["config"], *actual["sources"]):
            verify_reference(ref)
        done.add(key)
        records.append({"key": list(key), "report": reference, "budget_excesses": actual["budget_excesses"]})
    index = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21DC;mutable full-query/current-hardness selection,not acceptance",
             "controller": identity(__file__), "dense_subset": dense_ref, "records": records, "complete": False}
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    for key in selections():
        if key in done:
            continue
        report = run_one(*key)
        actual = json.loads(report.read_text(encoding="utf-8"))
        records.append({"key": list(key), "report": identity(report), "budget_excesses": actual["budget_excesses"]})
        target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    assert len(records) == 20
    index["complete"] = True
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"selection": identity(target), "native_cases": 20, "budget_excesses": sum(len(r["budget_excesses"]) for r in records)}), flush=True)


if __name__ == "__main__":
    main()
