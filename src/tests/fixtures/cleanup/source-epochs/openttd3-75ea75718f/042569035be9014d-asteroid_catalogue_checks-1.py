"""Verify the immutable asteroid catalogue and its focused field/profile producers.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group1
This bounded invocation records native executable/log identities and independent
canonical-table checks. It makes no paid launch, delivery, artwork or network claim.
"""
import argparse
import json
import re
import subprocess
import time

from space_preview_checks import ROOT, identity
from runner import current_run_context, run_native


def verify(configuration):
    """Run one focused configuration and retain its exact native producer report.

    @param configuration Built RelWithDebInfo or Debug configuration to execute.
    @return Path to the invocation's accepted catalogue report.
    """
    context = current_run_context("space", "asteroid-catalogue-verify")
    log = context.logs / "asteroid-catalogue.log"
    started = time.monotonic()
    with log.open("wb") as sink:
        run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"),
                            "[asteroid-catalogue],[space-preview],[multilayer-content-contract]"],
                   cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT,
                   check=True, timeout=110)
    elapsed = time.monotonic() - started
    evidence = log.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", evidence)
    assert passed, log
    assert "NORE=95050 WICE=99715 RARE=4018 total=198783" in evidence, log
    native_reports = list(context.reports.glob("native-*.json"))
    assert len(native_reports) == 1, native_reports
    native = json.loads(native_reports[0].read_text(encoding="utf-8"))
    assert native["configuration"] == configuration and native["exit_status"] == 0, native
    report = context.reports / "asteroid-catalogue.json"
    report.write_text(json.dumps({
        "_documentation": "Owner: r2mlvs10-asteroids-and-mining-probes.md Group1; immutable definitions and focused field/profile acceptance only.",
        "configuration": configuration, "elapsed_seconds": elapsed,
        "assertions": int(passed.group(1)), "cases": int(passed.group(2)),
        "native_reports": [identity(native_reports[0])], "log": identity(log),
        "size_percentiles": 100, "reserve_endpoint_combinations": 18,
        "full_hold_tonnes": 20, "full_hold_work_ticks": 740,
        "mean_reserves_tonnes": {"NORE": 1080, "WICE": 2160, "RARE": 270},
        "unchanged_seed": "0x20261012", "actual_reserves_tonnes": {"NORE": 95050, "WICE": 99715, "RARE": 4018},
        "scope": "Industry exposes canonical immutable class/size records; Classic exposes none; inclusive sampling preserves feature RNG ownership and the existing seed ledger.",
    }, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Parse the built configuration and publish the bounded invocation's accepted report path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    args = parser.parse_args()
    report = verify(args.configuration)
    print(report.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
