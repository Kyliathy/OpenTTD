"""Focused real packet/work adapter contracts.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10B
These isolated consumed-frame fixtures do not establish native paid processor procurement.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from space_preview_checks import ROOT, identity
from runner import current_run_context, run_native


def focused(configuration, scope):
    """Retain actual bounded contracts with executable and terminal identities.

    @param configuration Existing build configuration of the real test executable.
    @param scope Existing producer preflight or completed packet/work adapter scope.
    @return Invocation-owned proof report with exact assertion and process membership.
    """
    context = current_run_context("space", "orbital-inventory-" + scope)
    context.prepare_runtime()
    log = context.logs / (scope + ".log")
    tags = "[orbital-activation-contract],[orbital-service-contract],[orbital-freight-contract],[layer-recipe-batch],[utility-power-recipe],[layer-recipe-work]"
    if scope == "inventory":
        tags += ",[orbital-recipe-inventory]"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), tags],
                            cwd=ROOT / "build", env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration),
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8", errors="replace"))
    assert result.returncode == 0 and match, log
    target = context.reports / ("orbital-inventory-" + scope + ".json")
    target.write_text(json.dumps(dict(_documentation=__doc__, configuration=configuration, scope=scope,
                                     assertions=int(match[1]), cases=int(match[2]),
                                     native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Run one bounded scope and publish its explicit invocation report membership."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scope", choices=("prerequisites", "inventory"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    proof = focused(args.configuration, args.scope)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, configuration=args.configuration,
                                          scope=args.scope, complete=True, reports=[proof.relative_to(ROOT).as_posix()]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
