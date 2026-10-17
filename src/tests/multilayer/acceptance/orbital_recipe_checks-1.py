"""Verify the actual shared atomic recipe and utility producers before orbital inventory integration.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10
Isolated unit facilities verify the shared producer; actual orbital infrastructure and procurement remain later acceptance.
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
    """Execute focused complete-batch, electrical transaction and actual shared ledger boundaries.

    @param configuration Matching current test executable configuration.
    @param scope Existing producer prerequisites or new shared staged-work boundary with its required producers.
    @return Retained actual assertion/process proof, with no invented orbital input.
    """
    context = current_run_context("space", "orbital-recipe-" + scope); context.prepare_runtime(); log = context.logs / (scope + ".log")
    tags = "[layer-recipe-batch],[utility-power-recipe],[utility-power-ledger],[utility-power-mutation],[utility-power-effects],[utility-power-stage]"
    if scope == "work":
        tags += ",[layer-recipe-work]"
    with log.open("wb") as sink:
        result = run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"),
                            tags], cwd=ROOT / "build",
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration), stdout=sink, stderr=subprocess.STDOUT, timeout=90)
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(encoding="utf-8", errors="replace")); assert result.returncode == 0 and match, log
    target = context.reports / ("orbital-recipe-" + scope + ".json")
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group10; delivered shared all-input/output recipe and actual utility allocation/atomic mutation/effect/saved-retry fixtures. The explicit scope distinguishes prerequisite checks from shared staged-work tests; no orbital runtime claim.",
                                     configuration=configuration, scope=scope, assertions=int(match[1]), cases=int(match[2]), native_reports=[identity(p) for p in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Run one explicit producer-boundary scope; keep all output in invocation-owned repository artifacts."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True); parser.add_argument("--scope", choices=("prerequisites", "work"), default="prerequisites"); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists(); proof = focused(args.configuration, args.scope)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group10; explicit shared recipe producer scope membership.", configuration=args.configuration,
                                          scope=args.scope, complete=True, reports=[proof.relative_to(ROOT).as_posix()]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
