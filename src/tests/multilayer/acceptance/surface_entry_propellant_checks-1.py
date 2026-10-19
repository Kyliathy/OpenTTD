"""Verify newly consumed native PCHM selection and whole recipe work boundaries.

@file surface_entry_propellant_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BID.1.
Prepared component worlds prove contracts only; they cannot supply the selected
genuine company's chemicals, factory, money, construction or processing work.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from space_preview_checks import ROOT, identity
from artifact_paths import create_run_context
from runner import run_native


def execute(configuration):
    """@param configuration Existing independently linked complete test application. @return Focused producer evidence."""
    context = create_run_context("space", "surface-entry-propellant-producers")
    context.prepare_runtime()
    application = ROOT / "build" / configuration / "openttd_test.exe"
    selector = "Native Surface processors execute every frozen material alternative,Native processor recipe command preserves stock and enforces control"
    log = context.logs / "producers.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), selector, "--durations", "yes"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration))
    terminal = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and match and int(match[2]) == 2, log
    assert "Native Surface processors execute every frozen" in terminal and "Native processor recipe command preserves" in terminal
    return dict(application=identity(application), terminal=identity(log), assertions=int(match[1]), cases=2,
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                component_fixture=True, genuine_world_predecessor=False, full_regression=False,
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/layer_processor.cpp"),
                         identity(ROOT / "src/tests/processor_fixture.h"), identity(ROOT / "src/multilayer_recipe.cpp"),
                         identity(ROOT / "src/layer_industry_processor.cpp"), identity(ROOT / "src/industry_cmd.cpp")])


def main():
    """Retain only the active group's newly consumed component boundaries without rebuilding the complete application."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    operation = execute(args.configuration)
    output.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, operation=operation), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=output.relative_to(ROOT).as_posix(), assertions=operation["assertions"], cases=2)), flush=True)


if __name__ == "__main__":
    main()
