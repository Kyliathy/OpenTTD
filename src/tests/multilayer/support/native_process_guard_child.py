"""Provide isolated short-lived process trees for native harness ownership checks.

@file native_process_guard_child.py
@see docs/zz-reach2/architecture/archi-historical-debug.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
No game, save, graphics decoder or user process is opened. PID observations are
written only to the current project's generated review/cache owners.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[4]


def main():
    """Execute an explicitly selected failing or waiting child with bounded fixture participants."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("exit", "sleep", "tree", "tree-fail"))
    parser.add_argument("--seconds", type=float, default=30)
    parser.add_argument("--pid-file", type=Path)
    arguments = parser.parse_args()
    if arguments.mode == "exit":
        print("owned child completed with explicit fixture failure", flush=True)
        return 7
    child = None
    if arguments.mode in ("tree", "tree-fail"):
        child = subprocess.Popen([sys.executable, __file__, "sleep", "--seconds", str(arguments.seconds)],
                                 stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                 creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    if arguments.pid_file is not None:
        target = arguments.pid_file.resolve()
        # PID observations cannot overwrite original fixtures or escape this
        # checkout; only generated fixture output owners are admitted.
        if not any(target.is_relative_to(ROOT / owner) for owner in ("mlm/reviews", "test-cache")):
            raise ValueError("process fixture PID output is outside generated owners")
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(json.dumps({"_documentation": __doc__, "root": os.getpid(), "child": child.pid if child else None}) + "\n", encoding="utf-8")
    print("owned waiting child ready", flush=True)
    if arguments.mode == "tree-fail":
        # Deliberately leave the fixture leaf to the launch owner's job. This
        # proves a failed root cannot hide descendants by exiting before cleanup.
        return 7
    try:
        time.sleep(arguments.seconds)
    finally:
        # Normal fixture completion retires only this actually created leaf;
        # the watchdog deliberately exercises forced whole-tree retirement too.
        if child is not None and child.poll() is None:
            child.terminate()
            child.wait(timeout=10)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
