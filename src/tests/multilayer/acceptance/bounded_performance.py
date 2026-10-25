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

from normal_ground_merge import ROOT, read, checked
from native_build import verify_source_epoch
from runner import create_run_context, start_native
from artifact_rebuild import identity as _identity
from functools import partial
identity = partial(_identity, root=ROOT)  # Bind the existing canonical identity helper, never a parallel file owner.
from normal_ground_merge import checked as _checked

def verify_reference(reference):
    """@param reference Exact current retained/generated identity. @return Verified contained bytes without old executable fallback."""
    return _checked(reference, ROOT)
from ground_performance import hardware_identity, memory_status, process_memory
BASELINE = "src/tests/fixtures/performance/openttd3-75ea75718f/ordinary-baseline.json"
BASELINE_SHA = "27c6131b6952f0174b9155d6fdc7d484fb92c46dbc90c69ac63a9200d68db264"
from ground_performance import PROTOCOL


def observe(build_reference, tag, selectors, source_paths, scenario, controller, prepare=None):
    """Observe one existing selected native workload and its actual Windows peaks.

    @param build_reference Current selected Tube build and source epoch identity.
    @param tag Exact native workload selector, never the complete target suite.
    @param selectors Explicit bounded scale/mode environment.
    @param source_paths Actual source owners for this workload.
    @param scenario Existing performance run scenario.
    @param controller Actual selecting producer path, not a prior receipt.
    @param prepare Optional explicit invocation preparation before child creation.
    @return Context, actual observation and independent frozen baseline budgets.
    """
    build = read(build_reference, ROOT)
    configuration = build["configuration"]
    epoch = read(build["source_epoch"], ROOT)
    verify_source_epoch(epoch, ROOT, build["tool_profile"]["path"])
    assert configuration in ("Debug", "RelWithDebInfo")
    baseline_ref = identity(ROOT / BASELINE)
    assert baseline_ref["sha256"] == BASELINE_SHA
    baseline = json.loads(verify_reference(baseline_ref).read_text(encoding="utf-8"))
    protocol = json.loads((ROOT / PROTOCOL).read_text())
    protocol["minimum_physical_available_bytes"] = 4 * 1024**3
    context = create_run_context("performance", scenario)
    context.prepare_runtime()
    if prepare is not None:
        prepare(context)
    binary = build["targets"]["openttd_tube_test"]
    controller_ref = identity(controller)
    paths = {*source_paths, "src/tests/multilayer/acceptance/bounded_performance.py", "src/tests/multilayer/acceptance/ground_performance.py",
             "src/tests/multilayer/support/native_build.py", "src/tests/multilayer/support/runner.py"}
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
            child = start_native(context, command, cwd=ROOT / "build", env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, **selectors), stdout=sink,
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
        # Only this invocation's still-live child may be terminated; every exit is reaped before reporting.
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
    result = {"build": build_reference, "active_physical_reserve_bytes": protocol["minimum_physical_available_bytes"], "configuration": configuration, "baseline": baseline_ref, "controller": controller_ref, "sources": sources,
              "binary": binary, "config": identity(context.config_path), "hardware": hardware_identity(),
              "native": {"argv": command, "cwd": "build", "environment_selectors": selectors, "log": identity(log),
                         "exit_status": child.returncode if child else None},
              "memory_samples": identity(memory_file), "children_reaped": child is not None and child.poll() is not None,
              "wall_seconds": time.perf_counter() - started, "failure": failure, "status": "failed" if failure else "observed"}
    if not failure:
        match = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", log.read_text(encoding="utf-8", errors="replace"))
        # Exit zero with no matching tests is a refused workload, never a
        # timing pass. Preserve the actual terminal and memory observation so
        # the caller can repair its target declaration without replaying peers.
        if match is None:
            result.update(status="failed", failure="selected native workload has no passing cases")
        else:
            result["native"].update(assertions=int(match[1]), cases=int(match[2]))
        result["peak_commit_bytes"] = max(row["process"]["lifetime_peak_commit_bytes"] for row in samples)
        result["peak_working_bytes"] = max(row["process"]["lifetime_peak_working_bytes"] for row in samples)
    verify_source_epoch(epoch, ROOT, build["tool_profile"]["path"])
    context.output("reports", "native-observation.json").write_text(json.dumps(dict(_documentation=__doc__, **result), indent=2) + "\n")
    return context, result, baseline

