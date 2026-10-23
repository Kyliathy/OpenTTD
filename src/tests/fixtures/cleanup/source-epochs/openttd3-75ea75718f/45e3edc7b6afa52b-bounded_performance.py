"""Observe one native performance process with actual Windows peaks and retained failures.

@file bounded_performance.py
@see r2mlvs16-integrated-acceptance-and-balance.md Groups21DD-21E.
This owns process lifetime and evidence only;native owners define every simulated quantity.
"""

import json
import os
import re
import subprocess
import time

from ground_network import ROOT
from runner import create_run_context, start_native
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_performance import hardware_identity, memory_status, process_memory
from ground_stress import BASELINE, BASELINE_SHA, selected_budget


def observe(configuration, tag, selectors, source_paths, scenario, controller, prepare=None):
    """Run one immutable explicit selection,write its OS observations and reap before returning success or failure."""
    assert configuration in ("Debug", "RelWithDebInfo")
    baseline_ref = identity(ROOT / BASELINE)
    assert baseline_ref["sha256"] == BASELINE_SHA
    baseline = json.loads(verify_reference(baseline_ref).read_text(encoding="utf-8"))
    protocol = json.loads(verify_reference(baseline["protocol"]).read_text(encoding="utf-8"))
    context = create_run_context("performance", scenario)
    context.prepare_runtime()
    if prepare is not None:
        prepare(context)
    binary = identity(ROOT / f"build/{configuration}/openttd_test.exe")
    controller_ref = identity(controller)
    paths = {*source_paths, "src/tests/multilayer/bounded_performance.py", "src/tests/multilayer/ground_performance.py",
             "src/tests/multilayer/guarded_native_build.py", "src/tests/multilayer/support/runner.py"}
    sources = [identity(ROOT / path) for path in sorted(paths)]
    command = [str(verify_reference(binary)), tag]
    log = context.logs / "native.log"
    before = memory_status()
    assert before["physical_available"] >= protocol["minimum_physical_available_bytes"] and before["commit_available"] >= protocol["minimum_commit_available_bytes"]
    started, child, samples, failure = time.perf_counter(), None, [], None
    print(json.dumps({"phase": "native_performance_started", "configuration": configuration, "tag": tag,
                      "selectors": selectors, "log": str(log.relative_to(ROOT))}), flush=True)
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
        assert identity(ROOT / binary["path"]) == binary and identity(controller) == controller_ref
        assert all(identity(ROOT / ref["path"]) == ref for ref in sources)
    memory_file = context.output("reports", "memory-samples.json")
    memory_file.write_text(json.dumps({"opening": before, "samples": samples, "closing": memory_status()}, indent=2) + "\n", encoding="utf-8")
    result = {"configuration": configuration, "baseline": baseline_ref, "controller": controller_ref, "sources": sources,
              "binary": binary, "config": identity(context.config_path), "hardware": hardware_identity(),
              "native": {"argv": command, "cwd": "build", "environment_selectors": selectors, "log": identity(log),
                         "exit_status": child.returncode if child else None},
              "memory_samples": identity(memory_file), "children_reaped": child is not None and child.poll() is not None,
              "wall_seconds": time.perf_counter() - started, "failure": failure, "status": "failed" if failure else "observed"}
    if not failure:
        match = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", log.read_text(encoding="utf-8", errors="replace"))
        assert match and int(match[1]) > 0
        result["native"].update(assertions=int(match[1]), cases=int(match[2]))
        result["peak_commit_bytes"] = max(row["process"]["lifetime_peak_commit_bytes"] for row in samples)
        result["peak_working_bytes"] = max(row["process"]["lifetime_peak_working_bytes"] for row in samples)
    return context, result, baseline


def verify_observation(actual, baseline, tag, selectors):
    """Recheck exact process selection,source bindings,native pass counts and raw OS ceilings."""
    assert actual["status"] == "observed" and actual["failure"] is None and actual["children_reaped"] is True
    for ref in (actual["controller"], actual["binary"], actual["config"], *actual["sources"]):
        verify_reference(ref)
    native = actual["native"]
    assert native["argv"] == [str(verify_reference(actual["binary"])), tag] and native["cwd"] == "build"
    assert native["environment_selectors"] == selectors and native["exit_status"] == 0
    text = verify_reference(native["log"]).read_text(encoding="utf-8")
    match = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
    assert match and int(match[1]) == native["assertions"] > 0 and int(match[2]) == native["cases"]
    memory = json.loads(verify_reference(actual["memory_samples"]).read_text(encoding="utf-8"))
    protocol = json.loads(verify_reference(baseline["protocol"]).read_text(encoding="utf-8"))
    assert memory["samples"] and actual["wall_seconds"] <= protocol["child_timeout_seconds"]
    assert actual["peak_commit_bytes"] == max(row["process"]["lifetime_peak_commit_bytes"] for row in memory["samples"])
    assert actual["peak_working_bytes"] == max(row["process"]["lifetime_peak_working_bytes"] for row in memory["samples"])
    for system in [memory["opening"], memory["closing"], *(row["system"] for row in memory["samples"])]:
        assert system["physical_available"] >= protocol["minimum_physical_available_bytes"] and system["commit_available"] >= protocol["minimum_commit_available_bytes"]
    assert actual["peak_commit_bytes"] <= protocol["maximum_child_private_bytes"]
    return text
