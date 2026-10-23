"""Verify focused canonical motion and its unchanged generation/content producers.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group3
One bounded configuration invocation; no full regression or paid mission claim.
"""
import argparse
import json
import re
import subprocess

from space_preview_checks import ROOT, identity
from runner import current_run_context, run_native


def verify(configuration):
    """Run exact motion and directly consumed field/content producer fixtures.

    @param configuration Completed build configuration, never linked concurrently.
    @return Accepted focused report path with native executable provenance.
    """
    context = current_run_context("space", "asteroid-motion-verify")
    log = context.logs / "asteroid-motion-focused.log"
    with log.open("wb") as sink:
        run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"),
                            "[asteroid-motion],[asteroid-generation],[asteroid-catalogue],[space-preview]"],
                   cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", evidence)
    assert passed and "ASTEROID_MOTION first_32_ticks=632" in evidence, log
    ledger = re.search(r"idle_ticks=(\d+) travel_cost=(\d+) cost_remainder=(\d+) two_return_total=(\d+)", evidence)
    assert ledger, log
    native = sorted(context.reports.glob("native-*.json"))
    assert len(native) == 1
    report = context.reports / "asteroid-motion.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs10 Group3; canonical integer motion and numerical pursuit geometry only.",
        "configuration": configuration, "assertions": int(passed.group(1)), "cases": int(passed.group(2)),
        "native_reports": [identity(p) for p in native], "log": identity(log),
        "heading_speed_combinations": 24, "first_32_tile_pursuit_ticks": 632,
        "geometry_two_return_ledger": dict(zip(("idle_ticks", "travel_cost", "cost_remainder", "total"), map(int, ledger.groups()))),
        "generation_stream_unchanged": True, "full_regression": False}, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Parse the completed configuration and print its accepted bounded proof path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    args = parser.parse_args()
    print(verify(args.configuration).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
