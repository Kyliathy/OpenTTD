#!/usr/bin/env python3
"""Run one bounded native lifecycle/storage component case.

@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group33.
@see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 8: the native executable resolves on Linux single-configuration builds too.
One case per invocation keeps every native child below110 seconds. Diagnostic
time describes these components only. Logs belong in logs/space-fixes and the
native operation/storage sample in test-cache/space-fixes.
"""
import argparse
import json
import os
import re
import subprocess
import time

from space_fixes_checks import ROOT, identity, native_executable


def main():
    """Execute exactly one explicit actual-population fixture without a full regression.

    @return Actual native child status, retaining hash-bound artifacts on success or failure.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", required=True, choices=("small", "normal", "largest-normal", "largest-dense", "clustered", "narrow-tall", "narrow-wide"))
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--run", required=True)
    args = parser.parse_args()
    if not re.fullmatch(r"[a-z0-9-]+", args.run):
        raise ValueError("use an unused lowercase artifact identity")
    output = ROOT / "test-cache/space-fixes" / args.run
    output.mkdir(parents=True, exist_ok=False)
    sample = output / "native-lifecycle-work.json"
    executable = native_executable(args.configuration, "openttd_space_fixes_test")
    log = ROOT / "logs/space-fixes" / (args.run + ".log")
    environment = dict(os.environ, OTTD_ASTEROID_WORK_CASE=args.case, OTTD_LIFECYCLE_WORK_OUTPUT=str(sample))
    started = time.monotonic()
    with log.open("w", encoding="utf-8") as sink:
        result = subprocess.run([str(executable), "[.asteroid-lifecycle-work]", "--warn", "NoTests"], cwd=ROOT / "build",
                                env=environment, stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    receipt = {"_documentation": __doc__, "configuration": args.configuration, "case": args.case,
               "executable": identity(executable), "log": identity(log), "exit_code": result.returncode,
               "diagnostic_elapsed_seconds": round(time.monotonic() - started, 3),
               "sample": identity(sample) if sample.is_file() else None,
               "source": identity(ROOT / "src/tests/asteroid_lifecycle_work.cpp")}
    (ROOT / "logs/space-fixes" / (args.run + "-work.json")).write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    assert result.returncode == 0 and sample.is_file(), log
    terminal = log.read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in terminal and "LIFECYCLE_WORK case=" + args.case in terminal, log
    detail = json.loads(sample.read_text())
    print("LIFECYCLE_WORK", args.case, "initial=" + str(detail["initial_population"]), "high_cycles=64 empty_intervals=16")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
