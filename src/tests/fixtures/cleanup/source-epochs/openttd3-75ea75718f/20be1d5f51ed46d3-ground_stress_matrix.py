"""Run the explicit ground owner stress matrix serially and retain focused producer regressions.

@file ground_stress_matrix.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
One bounded native process at a time;all budget excesses remain visible.
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
from ground_stress import BASELINE, BASELINE_SHA, SOURCES, run_one
from ground_performance import memory_status, process_memory


def selections():
    """Declare every required fixed workload before observing results."""
    for cfg in ("Debug", "RelWithDebInfo"):
        for axis in (512, 1024, 2048):
            for jobs in (0, 32, 128):
                yield cfg, "job", axis, jobs, "straight"
            for jobs in (0, 128):
                for scenario in ("straight", "irregular", "limited", "blocked"):
                    yield cfg, "route", axis, jobs, scenario
        yield cfg, "command", 128, 0, "straight"


def run_regression(cfg):
    """Rerun the actual earlier producer boundaries after shared helper extraction,with the same memory/deadline guard."""
    context = create_run_context("performance", "ground-stress-regressions")
    context.prepare_runtime()
    executable = ROOT / f"build/{cfg}/openttd_test.exe"
    selector = "[excavation-runtime-budget],[excavation-route-search],[excavation-route-limits],[tbm-area]"
    command, binary = [str(executable), selector], identity(executable)
    sources = [identity(ROOT / path) for path in SOURCES]
    baseline = json.loads((ROOT / BASELINE).read_text(encoding="utf-8"))
    protocol = json.loads(verify_reference(baseline["protocol"]).read_text(encoding="utf-8"))
    log, child, samples = context.logs / "native.log", None, []
    started = time.perf_counter()
    opening = memory_status()
    assert opening["physical_available"] >= protocol["minimum_physical_available_bytes"] and opening["commit_available"] >= protocol["minimum_commit_available_bytes"]
    try:
        with log.open("wb") as sink:
            child = start_native(context, command, cwd=ROOT / "build", env=dict(os.environ), stdout=sink,
                                 stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
            while child.poll() is None:
                elapsed, system, process = time.perf_counter() - started, memory_status(), process_memory(child)
                samples.append({"seconds": elapsed, "system": system, "process": process})
                assert system["physical_available"] >= protocol["minimum_physical_available_bytes"] and system["commit_available"] >= protocol["minimum_commit_available_bytes"]
                assert max(process["private_bytes"], process["lifetime_peak_commit_bytes"]) <= protocol["maximum_child_private_bytes"]
                assert elapsed < 110
                try:
                    child.wait(timeout=.01)
                except subprocess.TimeoutExpired:
                    pass
            assert child.returncode == 0, log
            samples.append({"seconds": time.perf_counter() - started, "system": memory_status(), "process": process_memory(child)})
    finally:
        if child is not None and child.poll() is None:
            child.terminate()
            try:
                child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait(timeout=5)
        assert identity(executable) == binary and all(identity(ROOT / ref["path"]) == ref for ref in sources)
    text = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
    assert match and int(match[1]) > 0 and int(match[2]) == 5
    memory_file = context.output("reports", "memory-samples.json")
    memory_file.write_text(json.dumps({"opening": opening, "samples": samples}, indent=2) + "\n", encoding="utf-8")
    report = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21DC;existing focused producer regression after helper extraction",
              "configuration": cfg, "controller": identity(__file__), "binary": binary, "sources": sources,
              "config": identity(context.config_path), "argv": command, "cwd": "build", "log": identity(log),
              "native_assertions": int(match[1]), "native_cases": int(match[2]), "exit_status": child.returncode,
              "children_reaped": child.poll() is not None, "memory_samples": identity(memory_file)}
    target = context.output("reports", f"producer-regression-{cfg}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"regression": identity(target), "native_assertions": report["native_assertions"]}), flush=True)
    return target


def main():
    """Reuse only explicitly supplied exact current probes;complete every other workload and regression sequentially."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--prior", action="append", default=[])
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    baseline_ref = identity(ROOT / BASELINE)
    assert baseline_ref["sha256"] == BASELINE_SHA
    expected, records, done = set(selections()), [], set()
    for name in args.prior:
        reference = identity(ROOT / name)
        actual = json.loads(verify_reference(reference).read_text(encoding="utf-8"))
        key = tuple(actual[field] for field in ("configuration", "kind", "axis", "jobs", "scenario"))
        assert key in expected and key not in done and actual["status"] == "observed" and actual["failure"] is None
        for ref in (actual["controller"], actual["binary"], actual["config"], *actual["sources"]):
            verify_reference(ref)
        done.add(key)
        records.append({"key": list(key), "report": reference, "budget_excesses": actual["budget_excesses"]})
    index = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21DC;mutable sequential workload selection,not acceptance",
             "controller": identity(__file__), "baseline": baseline_ref, "records": records, "regressions": [], "complete": False}
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    for key in selections():
        if key in done:
            continue
        report = run_one(*key)
        actual = json.loads(report.read_text(encoding="utf-8"))
        records.append({"key": list(key), "report": identity(report), "budget_excesses": actual["budget_excesses"]})
        target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    for cfg in ("Debug", "RelWithDebInfo"):
        index["regressions"].append(identity(run_regression(cfg)))
        target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    assert len(records) == 68
    index["complete"] = True
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"selection": identity(target), "stress_cases": 68, "budget_excesses": sum(len(r["budget_excesses"]) for r in records)}), flush=True)


if __name__ == "__main__":
    main()
