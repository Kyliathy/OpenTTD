"""Measure native shared utility settlement with exact joules,company expenses and Windows peaks.

@file utility_performance.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21DDB.
Prepared shared-owner workloads do not claim physical tube movement or normal bootstrap.
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
from ground_performance import hardware_identity, memory_status, parse_fields, process_memory
from ground_stress import BASELINE, BASELINE_SHA, percentiles, selected_budget

TAG = "[.utility-settlement-performance]"
SOURCES = ("src/tests/utility_power_performance.cpp", "src/tests/utility_power_fixture.h", "src/tests/excavation_fixture.h",
           "src/utility_power.cpp", "src/utility_power.h", "src/multilayer_utility.cpp", "src/multilayer_utility.h",
           "src/company_cmd.cpp", "src/tests/multilayer/ground_performance.py", "src/tests/multilayer/ground_stress.py",
           "src/tests/multilayer/guarded_native_build.py", "src/tests/multilayer/support/runner.py")


def verify_measurement(actual, baseline):
    """Independently reparse native output and OS observations;metadata cannot replace quantities or slow samples."""
    assert actual["status"] == "observed" and actual["failure"] is None and actual["children_reaped"] is True
    assert actual["classification"] == "prepared_utility_owner_performance" and actual["bootstrap_claim"] is False
    for ref in (actual["controller"], actual["binary"], actual["config"], *actual["sources"]):
        verify_reference(ref)
    cfg, count, fanout = (actual[key] for key in ("configuration", "accounts", "fanout"))
    assert cfg in ("Debug", "RelWithDebInfo") and count in (0, 32, 256, 4096) and fanout in (1, 4)
    native = actual["native"]
    assert native["argv"] == [str(verify_reference(actual["binary"])), TAG] and native["cwd"] == "build"
    assert native["environment_selectors"] == {"OTTD_UTILITY_ACCOUNTS": str(count), "OTTD_UTILITY_FANOUT": str(fanout)}
    text = verify_reference(native["log"]).read_text(encoding="utf-8")
    match = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
    assert match and int(match[1]) == native["assertions"] > 0 and int(match[2]) == native["cases"] == 1 and native["exit_status"] == 0
    complete = parse_fields(text, "UTILITY_SETTLEMENT_COMPLETE")
    samples = parse_fields(text, "UTILITY_SETTLEMENT_SAMPLE")
    assert complete == actual["complete"] and len(complete) == 1 and samples == actual["samples"]
    assert [row["index"] for row in samples] == list(range(5))
    end = complete[0]
    expected = {"accounts": count, "eligibility_edges": count * fanout, "physical_links": 0, "warmup": 1, "samples": 5,
                "settled_intervals": 6, "unused_expired_intervals": 1, "retired_intervals": 1, "generated_j": count * 216,
                "consumed_j": count * 162, "expiry_j": count * 27, "retired_released_j": count * 27,
                "expense": count * 30, "exact": 1, "rendering": 0}
    assert all(end[key] == value for key, value in expected.items())
    assert end["generated_j"] == end["consumed_j"] + end["expiry_j"] + end["retired_released_j"]
    budget = selected_budget(baseline, cfg, 128)
    assert actual["budget"] == budget
    timings = {op: percentiles([row[op + "_ns"] for row in samples]) for op in ("publication", "allocation", "settlement", "rotation")}
    assert actual["timings"] == timings
    excesses = [op + ".p99_ns" for op, values in timings.items() if values["p99_ns"] > budget["simulation_p99_ns"]]
    for op in ("unused_expiry", "retirement"):
        if end[op + "_ns"] > budget["command_p99_ns"]:
            excesses.append(op + ".ns")
    if end["setup_ns"] > budget["startup_or_load_ns"]:
        excesses.append("setup.ns")
    memory = json.loads(verify_reference(actual["memory_samples"]).read_text(encoding="utf-8"))
    observations = memory["samples"]
    protocol = json.loads(verify_reference(baseline["protocol"]).read_text(encoding="utf-8"))
    assert observations
    assert actual["peak_commit_bytes"] == max(row["process"]["lifetime_peak_commit_bytes"] for row in observations)
    assert actual["peak_working_bytes"] == max(row["process"]["lifetime_peak_working_bytes"] for row in observations)
    for system in [memory["opening"], memory["closing"], *(row["system"] for row in observations)]:
        assert system["physical_available"] >= protocol["minimum_physical_available_bytes"]
        assert system["commit_available"] >= protocol["minimum_commit_available_bytes"]
    assert actual["peak_commit_bytes"] <= protocol["maximum_child_private_bytes"]
    assert actual["wall_seconds"] <= protocol["child_timeout_seconds"]
    if actual["peak_commit_bytes"] > budget["peak_commit_bytes"]:
        excesses.append("peak_commit_bytes")
    assert actual["budget_excesses"] == excesses
    return end


def run_one(configuration, accounts, fanout):
    """Own exactly one bounded native child;retain failed attempts and reap before returning or raising."""
    assert configuration in ("Debug", "RelWithDebInfo") and accounts in (0, 32, 256, 4096) and fanout in (1, 4)
    baseline_ref = identity(ROOT / BASELINE)
    assert baseline_ref["sha256"] == BASELINE_SHA
    baseline = json.loads(verify_reference(baseline_ref).read_text(encoding="utf-8"))
    protocol = json.loads(verify_reference(baseline["protocol"]).read_text(encoding="utf-8"))
    budget = selected_budget(baseline, configuration, 128)
    context = create_run_context("performance", "utility-settlement")
    context.prepare_runtime()
    binary = identity(ROOT / f"build/{configuration}/openttd_test.exe")
    controller, sources = identity(__file__), [identity(ROOT / name) for name in SOURCES]
    selectors = {"OTTD_UTILITY_ACCOUNTS": str(accounts), "OTTD_UTILITY_FANOUT": str(fanout)}
    command = [str(verify_reference(binary)), TAG]
    log = context.logs / "native.log"
    before = memory_status()
    assert before["physical_available"] >= protocol["minimum_physical_available_bytes"] and before["commit_available"] >= protocol["minimum_commit_available_bytes"]
    started, child, samples, failure = time.perf_counter(), None, [], None
    print(json.dumps({"phase": "utility_started", "configuration": configuration, "accounts": accounts, "fanout": fanout, "log": str(log.relative_to(ROOT))}), flush=True)
    try:
        with log.open("wb") as sink:
            child = start_native(context, command, cwd=ROOT / "build", env=dict(os.environ, **selectors), stdout=sink,
                                 stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
            while child.poll() is None:
                elapsed, system, process = time.perf_counter() - started, memory_status(), process_memory(child)
                samples.append({"seconds": elapsed, "system": system, "process": process})
                assert system["physical_available"] >= protocol["minimum_physical_available_bytes"] and system["commit_available"] >= protocol["minimum_commit_available_bytes"], "system reserve crossed"
                assert max(process["private_bytes"], process["lifetime_peak_commit_bytes"]) <= protocol["maximum_child_private_bytes"], "owned memory ceiling crossed"
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
                child.kill(); child.wait(timeout=5)
        assert identity(ROOT / binary["path"]) == binary and identity(__file__) == controller
        assert all(identity(ROOT / ref["path"]) == ref for ref in sources)
    elapsed = time.perf_counter() - started
    memory_file = context.output("reports", "memory-samples.json")
    memory_file.write_text(json.dumps({"opening": before, "samples": samples, "closing": memory_status()}, indent=2) + "\n", encoding="utf-8")
    result = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21DDB;prepared utility owner,not physical transport/bootstrap",
              "classification": "prepared_utility_owner_performance", "bootstrap_claim": False, "configuration": configuration,
              "accounts": accounts, "fanout": fanout, "baseline": baseline_ref, "budget": budget,
              "budget_mapping": "smallest128 baseline applied to prepared64map;no relaxed larger-map budget",
              "controller": controller, "sources": sources, "binary": binary, "config": identity(context.config_path), "hardware": hardware_identity(),
              "native": {"argv": command, "cwd": "build", "environment_selectors": selectors, "log": identity(log), "exit_status": child.returncode if child else None},
              "memory_samples": identity(memory_file), "children_reaped": child is not None and child.poll() is not None,
              "wall_seconds": elapsed, "failure": failure, "status": "failed" if failure else "observed"}
    if not failure:
        text = log.read_text(encoding="utf-8", errors="replace")
        match = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert match and int(match[2]) == 1
        result["native"].update(assertions=int(match[1]), cases=int(match[2]))
        result["complete"] = parse_fields(text, "UTILITY_SETTLEMENT_COMPLETE")
        result["samples"] = parse_fields(text, "UTILITY_SETTLEMENT_SAMPLE")
        result["peak_commit_bytes"] = max(row["process"]["lifetime_peak_commit_bytes"] for row in samples)
        result["peak_working_bytes"] = max(row["process"]["lifetime_peak_working_bytes"] for row in samples)
        result["timings"] = {op: percentiles([row[op + "_ns"] for row in result["samples"]]) for op in ("publication", "allocation", "settlement", "rotation")}
        result["budget_excesses"] = [op + ".p99_ns" for op, values in result["timings"].items() if values["p99_ns"] > budget["simulation_p99_ns"]]
        for op in ("unused_expiry", "retirement"):
            if result["complete"][0][op + "_ns"] > budget["command_p99_ns"]:
                result["budget_excesses"].append(op + ".ns")
        if result["complete"][0]["setup_ns"] > budget["startup_or_load_ns"]:
            result["budget_excesses"].append("setup.ns")
        if result["peak_commit_bytes"] > budget["peak_commit_bytes"]:
            result["budget_excesses"].append("peak_commit_bytes")
        verify_measurement(result, baseline)
    target = context.output("reports", f"utility-{configuration}-{accounts}-{fanout}.json")
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "status": result["status"], "failure": failure,
                      "excesses": result.get("budget_excesses"), "timings": result.get("timings")}), flush=True)
    if failure:
        raise RuntimeError(failure + ";retained=" + str(target))
    return target


def main():
    """Run an explicit account/eligibility selection with unchanged fixed budgets."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--configuration", required=True, choices=("Debug", "RelWithDebInfo"))
    parser.add_argument("--accounts", required=True, type=int, choices=(0, 32, 256, 4096))
    parser.add_argument("--fanout", required=True, type=int, choices=(1, 4))
    args = parser.parse_args()
    run_one(args.configuration, args.accounts, args.fanout)


if __name__ == "__main__":
    main()
