"""Run shared utility bounds serially with the existing memory reserve.

@file utility_performance_matrix.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DDB.
Only exact explicitly selected current probes can be reused.
"""

import argparse
import json
import os
import re
import subprocess
import time

from ground_network import ROOT
from runner import create_run_context, start_native
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_performance import memory_status, process_memory
from utility_performance import BASELINE, BASELINE_SHA, SOURCES, run_one, verify_measurement


def selections():
    """Declare actual account/eligibility counts before execution."""
    for cfg in ("Debug", "RelWithDebInfo"):
        for count in (0, 32, 256, 4096):
            for fanout in (1, 4):
                yield cfg, count, fanout


def regression(cfg, protocol):
    """Retain the existing sparse allocator/retirement producer after adjacent test additions."""
    context = create_run_context("performance", "utility-producer-regression")
    context.prepare_runtime()
    binary, sources = identity(ROOT / f"build/{cfg}/openttd_test.exe"), [identity(ROOT / path) for path in SOURCES]
    command = [str(verify_reference(binary)), "[utility-power-performance]"]
    log, child, observations = context.logs / "native.log", None, []
    started, opening = time.perf_counter(), memory_status()
    assert opening["physical_available"] >= protocol["minimum_physical_available_bytes"] and opening["commit_available"] >= protocol["minimum_commit_available_bytes"]
    try:
        with log.open("wb") as sink:
            child = start_native(context, command, cwd=ROOT / "build", env=dict(os.environ), stdout=sink,
                                 stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
            while child.poll() is None:
                system, process = memory_status(), process_memory(child)
                observations.append({"system": system, "process": process})
                assert system["physical_available"] >= protocol["minimum_physical_available_bytes"] and system["commit_available"] >= protocol["minimum_commit_available_bytes"]
                assert max(process["private_bytes"], process["lifetime_peak_commit_bytes"]) <= protocol["maximum_child_private_bytes"]
                assert time.perf_counter() - started < protocol["child_timeout_seconds"]
                try:
                    child.wait(timeout=.01)
                except subprocess.TimeoutExpired:
                    pass
            observations.append({"system": memory_status(), "process": process_memory(child)})
            assert child.returncode == 0
    finally:
        if child is not None and child.poll() is None:
            child.terminate()
            try:
                child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                child.kill(); child.wait(timeout=5)
        assert identity(ROOT / binary["path"]) == binary and all(identity(ROOT / ref["path"]) == ref for ref in sources)
    match = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", log.read_text(encoding="utf-8"))
    assert match and int(match[2]) == 1
    memory_file = context.output("reports", "memory-samples.json")
    memory_file.write_text(json.dumps({"opening": opening, "samples": observations, "closing": memory_status()}, indent=2) + "\n", encoding="utf-8")
    report = {"configuration": cfg, "controller": identity(__file__), "binary": binary, "sources": sources,
              "config": identity(context.config_path), "argv": command, "cwd": "build", "log": identity(log),
              "assertions": int(match[1]), "cases": 1, "exit_status": child.returncode, "children_reaped": child.poll() is not None,
              "memory_samples": identity(memory_file)}
    target = context.output("reports", f"utility-producer-{cfg}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Checkpoint each actual observation;no missing workload or excess silently becomes a pass."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--prior", action="append", default=[])
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    baseline_ref = identity(ROOT / BASELINE)
    assert baseline_ref["sha256"] == BASELINE_SHA
    baseline = json.loads(verify_reference(baseline_ref).read_text(encoding="utf-8"))
    protocol = json.loads(verify_reference(baseline["protocol"]).read_text(encoding="utf-8"))
    expected, done, records = set(selections()), set(), []
    for path in args.prior:
        ref = identity(ROOT / path)
        actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))
        key = tuple(actual[field] for field in ("configuration", "accounts", "fanout"))
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
        index["regressions"].append(identity(regression(cfg, protocol)))
        target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    assert len(records) == 16 and len(index["regressions"]) == 2
    index["complete"] = True
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"selection": identity(target), "stress_cases": 16, "excesses": sum(len(row["budget_excesses"]) for row in records)}), flush=True)


if __name__ == "__main__":
    main()
