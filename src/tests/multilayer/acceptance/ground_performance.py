"""Measure real native ground maps with retained tick samples and a Windows memory reserve.

@file ground_performance.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21C;master11.6.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Explicit headless performance worlds never become genuine-bootstrap evidence.
"""

import argparse
import ctypes
from ctypes import wintypes
import json
import math
import os
import re
import subprocess
import sys
import time
import winreg

sys.dont_write_bytecode = True
from normal_ground_merge import ROOT, read, checked
from native_build import verify_source_epoch
from runner import create_run_context, start_native
from artifact_rebuild import identity as _identity
from functools import partial
identity = partial(_identity, root=ROOT)  # Bind the existing canonical identity helper, never a parallel file owner.
from native_build import memory_status

PROTOCOL = "src/tests/fixtures/performance/openttd3-75ea75718f/ground-workloads.json"
SOURCES = ("src/tests/ground_performance_fixture.h", "src/tests/ground_performance_merge.cpp",
           "src/tests/normal_ground_runtime_fixture.h", "src/tests/multilayer/support/runner.py",
           "src/openttd.cpp", "src/map.cpp", "src/geology_generation.cpp")


class PerformanceRefusal(RuntimeError):
    """An actual bounded refused workload,with its reaped process and exact retained failure artifact."""

    def __init__(self, message, report):
        """@param message Exact actual refusal. @param report Retained failed native observation path."""
        super().__init__(message)
        self.report = report


def hardware_identity():
    """@return Actual Windows CPU, logical threads and physical memory observed outside native timing."""
    with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"HARDWARE\DESCRIPTION\System\CentralProcessor\0") as key:
        cpu = winreg.QueryValueEx(key, "ProcessorNameString")[0].strip()
    return {"cpu": cpu, "logical_processors": os.cpu_count(), "physical_total_bytes": memory_status()["physical_total"],
            "windows_version": list(sys.getwindowsversion()), "python": sys.version}


class ProcessMemory(ctypes.Structure):
    """Native PROCESS_MEMORY_COUNTERS_EX;the OS retains lifetime working/commit peaks."""

    _fields_ = [("cb", wintypes.DWORD), ("PageFaultCount", wintypes.DWORD),
                *[(name, ctypes.c_size_t) for name in ("PeakWorkingSetSize", "WorkingSetSize", "QuotaPeakPagedPoolUsage",
                  "QuotaPagedPoolUsage", "QuotaPeakNonPagedPoolUsage", "QuotaNonPagedPoolUsage", "PagefileUsage",
                  "PeakPagefileUsage", "PrivateUsage")]]


def process_memory(child):
    """@param child Actual owned native process. @return Windows current/lifetime working and private memory counters."""
    query = ctypes.windll.psapi.GetProcessMemoryInfo
    query.argtypes = (wintypes.HANDLE, ctypes.POINTER(ProcessMemory), wintypes.DWORD)
    query.restype = wintypes.BOOL
    counters = ProcessMemory()
    counters.cb = ctypes.sizeof(counters)
    if not query(wintypes.HANDLE(int(child._handle)), ctypes.byref(counters), counters.cb):
        raise ctypes.WinError()
    return {"working_bytes": counters.WorkingSetSize, "private_bytes": counters.PrivateUsage,
            "lifetime_peak_working_bytes": counters.PeakWorkingSetSize,
            "lifetime_peak_commit_bytes": counters.PeakPagefileUsage, "page_faults": counters.PageFaultCount}


def parse_fields(text, marker):
    """@param text Actual native terminal. @param marker Exact observation prefix. @return Original named integer/string rows; duplicates refuse."""
    rows = []
    for match in re.finditer(r"^" + re.escape(marker) + r" ([^\r\n]+)$", text, re.MULTILINE):
        row = {}
        for item in match[1].split():
            key, value = item.split("=", 1)
            assert key not in row
            row[key] = int(value) if value.isdigit() else value
        rows.append(row)
    return rows


def summarize(samples):
    """@param samples Actual640native tick durations. @return Nearest-rank percentiles and five separate128tick means."""
    ordered = sorted(samples)
    assert len(samples) == 640
    return {"samples": len(samples), "p50_ns": ordered[math.ceil(len(samples) * .50) - 1],
            "p95_ns": ordered[math.ceil(len(samples) * .95) - 1], "p99_ns": ordered[math.ceil(len(samples) * .99) - 1],
            "maximum_ns": ordered[-1], "mean_ns": sum(samples) / len(samples),
            "interval_mean_ns": [sum(samples[offset:offset + 128]) / 128 for offset in range(0, 640, 128)]}


def run_one(build_reference, profile, axis, input_reference=None, view=None):
    """Run one existing bounded native generation/load workload.

    @param build_reference Current selected Tube build and source epoch identity.
    @param profile Explicit Classic or multilayer industry workload.
    @param axis Actual source or requested generated map axis.
    @param input_reference Optional exact ordinary merged native saved world.
    @param view Optional original surface/underground/hidden/headless native view role.
    @return Actual observation report path after owned child reaping; failed reports are retained.
    """
    build = read(build_reference, ROOT)
    configuration = build["configuration"]
    epoch = read(build["source_epoch"], ROOT)
    verify_source_epoch(epoch, ROOT, build["tool_profile"]["path"])
    input_path = input_reference["path"] if input_reference is not None else None
    protocol_ref = identity(ROOT / PROTOCOL)
    protocol = json.loads((ROOT / PROTOCOL).read_text(encoding="utf-8"))
    assert configuration in protocol["configurations"] and profile in protocol["profiles"]
    # The retained original64map is a load-only selection, never a generated
    # substitute for the original128/512benchmark protocol.
    assert axis in [protocol["compact_axis"], protocol["representative_axis"], *protocol["large_candidates"]] or (input_reference is not None and axis == 64)
    # The user-authorized operational reserve replaces only historical RAM
    # admission. Original native workload/timing budgets remain untouched.
    protocol["minimum_physical_available_bytes"] = 4 * 1024**3
    context = create_run_context("performance", "ground-maps")
    context.prepare_runtime()
    context.config_path.write_text(
        "; Plan16-21C;actual ordinary map baseline,headless measured dispatch.\n"
        f"[game_creation]\nmap_x = {axis.bit_length() - 1}\nmap_y = {axis.bit_length() - 1}\n"
        f"landscape = temperate\ngameplay_profile = {profile}\n"
        "[gui]\npause_on_newgame = true\nautosave_on_exit = false\n", encoding="utf-8")
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.write_text("# Plan16-21C;ordinary native startup.\nground_performance_start\n", encoding="utf-8")
    executable = checked(build["targets"]["openttd_tube_test"], ROOT)
    binary, sources = identity(executable), [identity(ROOT / name) for name in SOURCES]
    environment = dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_GROUND_PERFORMANCE_PROFILE=profile)
    assert view is None or (view in ("surface", "underground", "hidden", "headless") and input_path is not None)
    if view is None:
        environment.pop("OTTD_GROUND_PERFORMANCE_VIEW", None)
    else:
        environment["OTTD_GROUND_PERFORMANCE_VIEW"] = view
    original = None
    if input_path is not None:
        source = (ROOT / input_path).resolve()
        assert source.is_relative_to(ROOT) and source.is_file()
        original = identity(source)
        environment["OTTD_GROUND_PERFORMANCE_INPUT"] = str(source)
    else:
        environment.pop("OTTD_GROUND_PERFORMANCE_INPUT", None)
    command = [str(executable), "[.ground-performance-map]"]
    log = context.logs / "native.log"
    before = memory_status()
    assert before["physical_available"] >= protocol["minimum_physical_available_bytes"], before
    assert before["commit_available"] >= protocol["minimum_commit_available_bytes"], before
    child, samples, failure = None, [], None
    started = time.perf_counter()
    print(json.dumps({"phase": "performance_started", "configuration": configuration, "profile": profile,
                      "axis": axis, "mode": "load" if original else "generation", "log": str(log.relative_to(ROOT))}), flush=True)
    try:
        with log.open("wb") as sink:
            child = start_native(context, command, cwd=ROOT / "build", env=environment, stdout=sink,
                                 stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
            while child.poll() is None:
                status, process = memory_status(), process_memory(child)
                elapsed = time.perf_counter() - started
                samples.append({"seconds": elapsed, "system": status, "process": process})
                # Preserve the active user-authorized RAM/commit reserve while the measured process is alive.
                if status["physical_available"] < protocol["minimum_physical_available_bytes"] or status["commit_available"] < protocol["minimum_commit_available_bytes"]:
                    raise RuntimeError("native workload crossed the required free-memory reserve")
                if process["private_bytes"] > protocol["maximum_child_private_bytes"]:
                    raise RuntimeError("native workload crossed its individual private-memory ceiling")
                if elapsed >= protocol["child_timeout_seconds"]:
                    raise TimeoutError("native workload exceeded its110-second admission policy")
                try:
                    child.wait(timeout=protocol["memory_sample_seconds"])
                except subprocess.TimeoutExpired:
                    pass
            assert child.returncode == 0, log
            # Query the retained owned process handle after exit as well;OS
            # lifetime peaks include native shutdown and the last sample gap.
            samples.append({"seconds": time.perf_counter() - started, "system": memory_status(), "process": process_memory(child)})
    except BaseException as error:
        failure = f"{type(error).__name__}: {error}"
    finally:
        # Only this invocation's still-live child may be terminated; every exit is reaped before reporting.
        if child is not None and child.poll() is None:
            child.terminate()
            try:
                child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait(timeout=5)
        assert identity(executable) == binary and all(identity(ROOT / ref["path"]) == ref for ref in sources)
        assert original is None or identity(ROOT / original["path"]) == original
    memory_path = context.output("reports", "memory-samples.json")
    memory_path.write_text(json.dumps({"opening": before, "samples": samples, "closing": memory_status()}, indent=2) + "\n", encoding="utf-8")
    report = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21C;actual ground map performance observation,not acceptance",
              "classification": "native_ground_performance_observation", "bootstrap_claim": False,
              "build": build_reference, "active_physical_reserve_bytes": protocol["minimum_physical_available_bytes"],
              "configuration": configuration, "profile": profile, "axis": axis, "mode": "load" if original else "generation", "view_role": view,
              "hardware": hardware_identity(),
              "protocol": protocol_ref, "controller": identity(__file__), "binary": binary, "sources": sources,
              "config": identity(context.config_path), "startup": identity(startup), "input": original,
              "native": {"argv": command, "cwd": "build", "log": identity(log), "exit_status": child.returncode if child else None},
              "children_reaped": child is not None and child.poll() is not None, "wall_seconds": time.perf_counter() - started,
              "memory_samples": identity(memory_path), "status": "refused" if failure else "observed", "failure": failure}
    if not failure:
        text = log.read_text(encoding="utf-8", errors="replace")
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", text)
        assert counts and int(counts[2]) == 1 and int(counts[1]) > 0, log
        ready, owners = parse_fields(text, "GROUND_PERFORMANCE_READY"), parse_fields(text, "GROUND_PERFORMANCE_COUNTS")
        ticks, saves = parse_fields(text, "GROUND_PERFORMANCE_TICK"), parse_fields(text, "GROUND_PERFORMANCE_SAVE")
        assert len(ready) == 1 and len(owners) == 2 and len(ticks) == 640 and len(saves) == 2
        assert [row["index"] for row in ticks] == list(range(640))
        assert [row["phase"] for row in owners] == ["ready", "final"] and owners[1]["tick"] - owners[0]["tick"] == 768
        assert all(row["width"] == row["height"] == axis and row["map_layers"] == 4 for row in owners)
        assert ready[0]["profile"] == profile and ready[0]["mode"] == report["mode"]
        assert "GROUND_PERFORMANCE_COMPLETE warmup=128 intervals=5 ticks_per_interval=128 measured_ticks=640 rendering=false" in text
        report["native"].update(assertions=int(counts[1]), cases=1)
        saved = {name: identity(context.saves / f"{name}.sav") for name in ("initial-world", "initial-repeat", "final-world", "final-repeat")}
        # Native synchronous repeated writes are compressed deterministically.
        # Exact file equality is stronger than decompressed owner equality and
        # avoids holding two maximum-map decompressed worlds in Python memory.
        for phase in ("initial", "final"):
            assert all(saved[f"{phase}-world"][key] == saved[f"{phase}-repeat"][key] for key in ("sha256",)), (phase, saved)
        report.update(ready=ready[0], owners=owners, synchronous_saves=saves, saves=saved,
                      first_resumed_work=parse_fields(text, "GROUND_PERFORMANCE_RESUMED"),
                      view_observations=parse_fields(text, "GROUND_PERFORMANCE_VIEW"),
                      tick_samples=ticks, commands=parse_fields(text, "GROUND_PERFORMANCE_COMMAND"),
                      timings={field: summarize([row[field] for row in ticks]) for field in ("simulation_ns", "input_ns", "windows_ns")},
                      peak_working_bytes=max(row["process"]["lifetime_peak_working_bytes"] for row in samples),
                      peak_commit_bytes=max(row["process"]["lifetime_peak_commit_bytes"] for row in samples),
                      sampled_peak_private_bytes=max(row["process"]["private_bytes"] for row in samples),
                      minimum_physical_available_bytes=min(row["system"]["physical_available"] for row in samples),
                      minimum_commit_available_bytes=min(row["system"]["commit_available"] for row in samples))
    verify_source_epoch(epoch, ROOT, build["tool_profile"]["path"])
    target = context.output("reports", f"ground-map-{configuration}-{profile}-{axis}.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(target), "status": report["status"], "failure": failure,
                      "simulation": report.get("timings", {}).get("simulation_ns"), "peak_commit_bytes": report.get("peak_commit_bytes")}), flush=True)
    if failure:
        raise PerformanceRefusal(failure + ";retained report=" + str(target), target)
    return target

