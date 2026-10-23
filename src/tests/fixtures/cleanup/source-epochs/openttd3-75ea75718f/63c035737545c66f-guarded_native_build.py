"""Build one native Windows target with a memory reserve and one compiler worker by default.

@file guarded_native_build.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group6DCDB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The user reported system RAM exhaustion;only this invocation's owned build tree may be stopped.
"""

import argparse
import ctypes
from ctypes import wintypes
import json
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
GIB = 1024 ** 3
CMAKE = Path("C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe")


class MemoryStatus(ctypes.Structure):
    """Native MEMORYSTATUSEX:available physical and commit memory,with no guessed process sums."""
    _fields_ = [("length", wintypes.DWORD), ("load", wintypes.DWORD),
                ("physical_total", ctypes.c_ulonglong), ("physical_available", ctypes.c_ulonglong),
                ("commit_total", ctypes.c_ulonglong), ("commit_available", ctypes.c_ulonglong),
                ("virtual_total", ctypes.c_ulonglong), ("virtual_available", ctypes.c_ulonglong),
                ("extended_available", ctypes.c_ulonglong)]


def memory_status():
    """Read actual Windows memory availability;failure stops admission to a heavy build."""
    status = MemoryStatus()
    status.length = ctypes.sizeof(status)
    if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(status)):
        raise ctypes.WinError()
    return {key: int(getattr(status, key)) for key in ("physical_total", "physical_available", "commit_total", "commit_available")}


def stop_owned_tree(child):
    """Terminate only the still-live Popen build root and its children,then reap that exact root."""
    if child.poll() is not None:
        return
    subprocess.run(["taskkill.exe", "/PID", str(child.pid), "/T", "/F"], check=False,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, creationflags=subprocess.CREATE_NO_WINDOW, timeout=15)
    if child.poll() is None:
        child.kill()
    child.wait(timeout=15)


def build(configuration, log_name, workers=1):
    """Keep6GiB physical and8GiB commit available;abort our build if either measured reserve is crossed."""
    assert sys.platform == "win32" and workers in (1, 2)
    log = ROOT / log_name
    assert log.resolve().is_relative_to(ROOT / "logs") and not log.exists()
    receipt = log.with_suffix(".memory.json")
    assert not receipt.exists()
    log.parent.mkdir(parents=True, exist_ok=True)
    command = [str(CMAKE), "--build", "build", "--config", configuration, "--target", "openttd_test", "-j", "1", "--",
               f"/p:CL_MPCount={workers}", "/nodeReuse:false", "/p:LinkIncremental=false"]
    before = memory_status()
    physical_min, commit_min = before["physical_available"], before["commit_available"]
    reserve_physical, reserve_commit = 6 * GIB, 8 * GIB
    assert physical_min >= reserve_physical and commit_min >= reserve_commit, before
    print(json.dumps({"phase": "build_admitted", "configuration": configuration, "workers": workers, "memory": before}), flush=True)
    child = None
    stopped = False
    samples = 0
    started = time.monotonic()
    try:
        with log.open("wb") as output:
            child = subprocess.Popen(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
            print(json.dumps({"phase": "build_started", "pid": child.pid, "log": str(log.relative_to(ROOT))}), flush=True)
            while child.poll() is None:
                status = memory_status()
                samples += 1
                physical_min = min(physical_min, status["physical_available"])
                commit_min = min(commit_min, status["commit_available"])
                if status["physical_available"] < reserve_physical or status["commit_available"] < reserve_commit:
                    stopped = True
                    print(json.dumps({"phase": "memory_reserve_crossed", "memory": status}), flush=True)
                    stop_owned_tree(child)
                    break
                try:
                    child.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    pass
    finally:
        if child is not None and child.poll() is None:
            stop_owned_tree(child)
        record = {"configuration": configuration, "command": command, "workers": workers, "opening": before, "closing": memory_status(),
                  "physical_reserve": reserve_physical, "commit_reserve": reserve_commit, "minimum_physical_available": physical_min,
                  "minimum_commit_available": commit_min, "samples": samples, "seconds": round(time.monotonic() - started, 3),
                  "stopped_for_memory": stopped, "exit_code": child.returncode if child is not None else None,
                  "root_reaped": child is not None and child.poll() is not None}
        receipt.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"phase": "build_finished", "receipt": str(receipt.relative_to(ROOT)), **record}), flush=True)
    return 75 if stopped else child.returncode


def main():
    """Build a single selected configuration with the active memory/worker limits."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", required=True, choices=("Debug", "RelWithDebInfo"))
    parser.add_argument("--log", required=True)
    parser.add_argument("--workers", type=int, choices=(1, 2), default=1)
    args = parser.parse_args()
    raise SystemExit(build(args.config, args.log, args.workers))


if __name__ == "__main__":
    main()
