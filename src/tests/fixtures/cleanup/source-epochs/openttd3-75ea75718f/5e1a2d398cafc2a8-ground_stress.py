"""Measure real bounded ground owners,planning and native command estimates under the Windows reserve.

@file ground_stress.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Prepared owner stress is explicit;real full application work remains the21DB receipt.
"""

import argparse
import json
import math
import os
import re
import subprocess
import time

from ground_network import ROOT
from runner import create_run_context, start_native
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_performance import hardware_identity, memory_status, parse_fields, process_memory

BASELINE = "logs/performance/ground-maps/20261019-native-01/ordinary-baseline-verification.json"
BASELINE_SHA = "27c6131b6952f0174b9155d6fdc7d484fb92c46dbc90c69ac63a9200d68db264"
TAGS = {"job": "[.ground-job-performance]", "route": "[.ground-route-performance]", "command": "[.ground-command-performance]", "search": "[.ground-search-performance]"}
SOURCES = ("src/tests/ground_job_fixture.h", "src/tests/excavation_budget.cpp", "src/tests/excavation_route.cpp", "src/tests/tbm_area.cpp",
           "src/tests/excavation_fixture.h", "src/tests/tbm_fixture.h", "src/excavation.cpp", "src/excavation_jobs.cpp",
           "src/excavation_cut.cpp", "src/excavation_route.cpp", "src/tbm_project.cpp", "src/tests/multilayer/ground_performance.py",
           "src/tests/multilayer/guarded_native_build.py", "src/tests/multilayer/support/runner.py")


def percentiles(values):
    """Nearest-rank observations retain actual samples;no elapsed timing chooses native work."""
    ordered = sorted(values)
    assert ordered and ordered[0] >= 0
    return {"samples": len(values), "p50_ns": ordered[math.ceil(len(values) * .50) - 1],
            "p95_ns": ordered[math.ceil(len(values) * .95) - 1], "p99_ns": ordered[math.ceil(len(values) * .99) - 1],
            "maximum_ns": ordered[-1], "mean_ns": sum(values) / len(values)}


def selected_budget(baseline, configuration, axis):
    """Use the exact prior shape or a stricter largest completed smaller-map budget;never increase after observing stress."""
    candidates = [row for row in baseline["budgets"] if row["configuration"] == configuration and row["axis"] <= axis]
    assert candidates
    return max(candidates, key=lambda row: row["axis"])


def run_one(configuration, kind, axis=512, jobs=128, scenario="straight", hardness=0):
    """Run one explicit real native stress process and retain actual samples,peaks,counts and fixed-budget excesses."""
    assert configuration in ("Debug", "RelWithDebInfo") and kind in TAGS
    assert (kind == "command" and axis == 128) or axis in (512, 1024, 2048)
    assert jobs in (0, 32, 128) and scenario in ("straight", "irregular", "limited", "blocked", "remote-default", "remote-maximum", "regional-short")
    assert hardness in (0, 1)
    baseline_ref = identity(ROOT / BASELINE)
    assert baseline_ref["sha256"] == BASELINE_SHA
    baseline = json.loads(verify_reference(baseline_ref).read_text(encoding="utf-8"))
    protocol = json.loads(verify_reference(baseline["protocol"]).read_text(encoding="utf-8"))
    budget = selected_budget(baseline, configuration, axis)
    context = create_run_context("performance", "ground-stress")
    context.prepare_runtime()
    executable = ROOT / f"build/{configuration}/openttd_test.exe"
    binary, sources, controller = identity(executable), [identity(ROOT / name) for name in SOURCES], identity(__file__)
    environment = dict(os.environ, OTTD_GROUND_STRESS_AXIS=str(axis), OTTD_GROUND_STRESS_JOBS=str(jobs), OTTD_GROUND_STRESS_ROUTE=scenario)
    selector_names = ["OTTD_GROUND_STRESS_AXIS", "OTTD_GROUND_STRESS_JOBS", "OTTD_GROUND_STRESS_ROUTE"]
    if kind == "search":
        environment["OTTD_GROUND_STRESS_HARDNESS"] = str(hardness)
        selector_names.append("OTTD_GROUND_STRESS_HARDNESS")
    command = [str(executable), TAGS[kind]]
    log = context.logs / "native.log"
    before = memory_status()
    assert before["physical_available"] >= protocol["minimum_physical_available_bytes"] and before["commit_available"] >= protocol["minimum_commit_available_bytes"]
    started, child, samples, failure = time.perf_counter(), None, [], None
    print(json.dumps({"phase": "ground_stress_started", "configuration": configuration, "kind": kind, "axis": axis,
                      "jobs": jobs, "scenario": scenario, "log": str(log.relative_to(ROOT))}), flush=True)
    try:
        with log.open("wb") as sink:
            child = start_native(context, command, cwd=ROOT / "build", env=environment, stdout=sink,
                                 stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
            while child.poll() is None:
                elapsed, system, process = time.perf_counter() - started, memory_status(), process_memory(child)
                samples.append({"seconds": elapsed, "system": system, "process": process})
                assert system["physical_available"] >= protocol["minimum_physical_available_bytes"] and system["commit_available"] >= protocol["minimum_commit_available_bytes"], "system memory reserve crossed"
                assert max(process["private_bytes"], process["lifetime_peak_commit_bytes"]) <= protocol["maximum_child_private_bytes"], "owned process memory ceiling crossed"
                assert elapsed < protocol["child_timeout_seconds"], "native110s deadline exceeded"
                try:
                    child.wait(timeout=protocol["memory_sample_seconds"])
                except subprocess.TimeoutExpired:
                    pass
            samples.append({"seconds": time.perf_counter() - started, "system": memory_status(), "process": process_memory(child)})
            assert child.returncode == 0, log
    except BaseException as error:
        failure = f"{type(error).__name__}: {error}"
    finally:
        if child is not None and child.poll() is None:
            child.terminate()
            try:
                child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait(timeout=5)
        assert identity(executable) == binary and identity(__file__) == controller
        assert all(identity(ROOT / ref["path"]) == ref for ref in sources)
    memory_file = context.output("reports", "memory-samples.json")
    memory_file.write_text(json.dumps({"opening": before, "samples": samples, "closing": memory_status()}, indent=2) + "\n", encoding="utf-8")
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21DC;actual prepared native owner workload,not bootstrap or full application substitute",
              "classification": "prepared_ground_owner_performance", "bootstrap_claim": False, "configuration": configuration, "kind": kind,
              "axis": axis, "jobs": jobs, "scenario": scenario, "hardness": hardness, "baseline": baseline_ref, "budget": budget,
              "budget_mapping": "exact shape" if budget["axis"] == axis else "stricter largest completed smaller-map baseline",
              "controller": controller, "sources": sources, "binary": binary, "config": identity(context.config_path),
              "hardware": hardware_identity(), "native": {"argv": command, "cwd": "build", "environment_selectors": {k: environment[k] for k in
                  selector_names}, "log": identity(log), "exit_status": child.returncode if child else None},
              "memory_samples": identity(memory_file), "children_reaped": child is not None and child.poll() is not None,
              "wall_seconds": time.perf_counter() - started, "failure": failure, "status": "failed" if failure else "observed"}
    if not failure:
        text = log.read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[1]) > 0 and int(counts[2]) == 1
        result["native"].update(assertions=int(counts[1]), cases=1)
        result["peak_commit_bytes"] = max(row["process"]["lifetime_peak_commit_bytes"] for row in samples)
        result["peak_working_bytes"] = max(row["process"]["lifetime_peak_working_bytes"] for row in samples)
        result["budget_excesses"] = []
        if result["peak_commit_bytes"] > budget["peak_commit_bytes"]:
            result["budget_excesses"].append("peak_commit_bytes")
        if kind == "job":
            result["complete"] = parse_fields(text, "GROUND_JOB_COMPLETE")
            result["samples"] = parse_fields(text, "GROUND_JOB_SAMPLE")
            result["lookup"] = parse_fields(text, "GROUND_JOB_LOOKUP")
            assert len(result["complete"]) == 1 and len(result["samples"]) == 640 and len(result["lookup"]) == 5
            result["timings"] = {operation: percentiles([r["ns"] for r in result["samples"] if r["operation"] == operation]) for operation in ("work", "reserve-estimate")}
            for operation, limit in (("work", "simulation_p99_ns"), ("reserve-estimate", "command_p99_ns")):
                if result["timings"][operation]["p99_ns"] > budget[limit]:
                    result["budget_excesses"].append(operation + ".p99_ns")
        else:
            marker = {"route": "GROUND_ROUTE", "command": "GROUND_COMMAND", "search": "GROUND_SEARCH"}[kind]
            result["complete"] = parse_fields(text, marker + "_COMPLETE")
            result["samples"] = parse_fields(text, marker + "_SAMPLE")
            assert len(result["complete"]) == 1 and len(result["samples"]) == {"route": 80, "command": 320, "search": 5}[kind]
            result["timings"] = percentiles([r["ns"] for r in result["samples"]])
            if result["timings"]["p99_ns"] > budget["route_p99_ns" if kind in ("route", "search") else "command_p99_ns"]:
                result["budget_excesses"].append("p99_ns")
            if kind == "search":
                result["timing_policy"] = "one full-query warmup,five equal full queries;nearest-rank p99 equals observed maximum,not a robust tail estimate"
    target = context.output("reports", f"ground-stress-{kind}-{configuration}-{axis}-{jobs}-{scenario}.json")
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "status": result["status"], "failure": failure,
                      "excesses": result.get("budget_excesses"), "timings": result.get("timings")}), flush=True)
    if failure:
        raise RuntimeError(failure + ";retained=" + str(target))
    return target


def main():
    """Measure one exact source-bound stress selection;all native children are bounded and reaped."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--configuration", required=True, choices=("Debug", "RelWithDebInfo"))
    parser.add_argument("--kind", required=True, choices=tuple(TAGS))
    parser.add_argument("--axis", type=int, default=512)
    parser.add_argument("--jobs", type=int, default=128)
    parser.add_argument("--scenario", choices=("straight", "irregular", "limited", "blocked", "remote-default", "remote-maximum", "regional-short"), default="straight")
    parser.add_argument("--hardness", type=int, choices=(0, 1), default=0)
    args = parser.parse_args()
    run_one(args.configuration, args.kind, args.axis, args.jobs, args.scenario, args.hardness)


if __name__ == "__main__":
    main()
