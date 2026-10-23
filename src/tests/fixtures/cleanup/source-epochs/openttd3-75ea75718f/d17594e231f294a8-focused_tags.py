"""Run Catch2 tags one at a time and record each tag's result, so a failure is classified without cross-case leakage.

Owner plan: docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, Group 10R (re-baseline after the demo fixes).
Architecture: docs/zz-reach2/architecture/map-layers/archi-map-layers.md, "Paid boring and real spoil haulage".

R2DF2 found that combined runs leak state between test cases, so a tag can pass or fail depending on its
neighbours (r2df2-c-execution-notes.md, Group 10 "Lesson"). This runner therefore starts a fresh
openttd_test.exe process for every tag. It writes one JSON report under
logs/underground-rebaseline/<scenario>/<run-id>/ and never touches docs or fixture directories.

Usage (from the checkout root):
    python src/tests/multilayer/tbm/focused_tags.py --config RelWithDebInfo --scenario focused-failures tbm-project tbm-loop
"""
import argparse
import datetime
import hashlib
import json
import re
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
FAILURE = re.compile(r"^(?P<path>.+?)\((?P<line>\d+)\): failed: (?P<text>.*)$")
SUMMARY = re.compile(r"^(Passed|Failed|All tests passed|No tests ran|No test cases matched)")


def run_tag(executable, tag, timeout):
    """Run one tag in its own test process and parse the compact reporter output.

    @param executable Path of openttd_test.exe; it runs with the build directory as working directory,
                      because the generated-language fixtures are resolved from there.
    @param tag Catch2 tag name without brackets, for example "tbm-project".
    @param timeout Seconds before the process is killed and the tag is recorded as timed out.
    @return Dictionary with the tag, exit code, wall seconds, summary line and the failing file:line entries.
    """
    started = time.monotonic()
    try:
        done = subprocess.run([str(executable), f"[{tag}]", "-r", "compact"], cwd=executable.parent.parent,
                              capture_output=True, timeout=timeout)
        exit_code = done.returncode
        output = (done.stdout + done.stderr).decode("utf-8", errors="replace").replace("\x00", "")
    except subprocess.TimeoutExpired as expired:
        exit_code = "timeout"
        output = ((expired.stdout or b"") + (expired.stderr or b"")).decode("utf-8", errors="replace").replace("\x00", "")
    failures = []
    summary = ""
    # Keep only the lines that classify the run: each failed assertion, and the closing summary.
    for line in output.splitlines():
        match = FAILURE.match(line.strip())
        if match:
            failures.append({"file": Path(match["path"]).name, "line": int(match["line"]), "text": match["text"][:300]})
        elif SUMMARY.match(line.strip()):
            summary = line.strip()
    return {"tag": tag, "exit": exit_code, "seconds": round(time.monotonic() - started, 1), "summary": summary,
            "failures": failures}


def main():
    """Parse the arguments, run every requested tag alone and write the JSON report.

    @return Process exit code: 0 when every tag passed, 1 otherwise.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", default="RelWithDebInfo", help="Build configuration directory under build/.")
    parser.add_argument("--scenario", default="focused-tags", help="Report scenario folder under logs/underground-rebaseline/.")
    parser.add_argument("--timeout", type=int, default=110, help="Seconds allowed per tag (split tags that need more).")
    parser.add_argument("tags", nargs="+", help="Tags to run, with or without their brackets.")
    args = parser.parse_args()
    args.tags = [tag.removeprefix("[").removesuffix("]") for tag in args.tags]  # "[x]" and "x" name the same tag.
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")  # Assertion text may hold non-ASCII; never crash the console print.

    executable = ROOT / "build" / args.config / "openttd_test.exe"
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    run_id = f"{stamp}-{hashlib.sha1(' '.join(args.tags).encode()).hexdigest()[:10]}"
    out_dir = ROOT / "logs" / "underground-rebaseline" / args.scenario / run_id
    out_dir.mkdir(parents=True, exist_ok=True)

    results = []
    for tag in args.tags:
        result = run_tag(executable, tag, args.timeout)
        results.append(result)
        print(f"{tag}: exit={result['exit']} {result['seconds']}s {result['summary']}")
        for failure in result["failures"]:
            print(f"    {failure['file']}:{failure['line']}: {failure['text'][:160]}")

    report = {"configuration": args.config, "executable_mtime": executable.stat().st_mtime, "run_id": run_id,
              "results": results}
    (out_dir / "focused-tags.json").write_text(json.dumps(report, indent=1), encoding="utf-8")
    print(f"report: {out_dir.relative_to(ROOT) / 'focused-tags.json'}")
    # A tag that matched no case proves nothing, even though Catch exits 0 for it.
    return 0 if all(result["exit"] == 0 and not result["summary"].startswith(("No tests ran", "No test cases matched")) for result in results) else 1


if __name__ == "__main__":
    sys.exit(main())
