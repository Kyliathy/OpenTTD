"""Validate only the chemical producer boundaries newly consumed by Plan11.

@file surface_entry_chemical_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.1.
Prepared component worlds prove admission, complete batches and packet delivery;
they never supply the authentic game's materials, capital or earned work.
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
    """Run one focused burst in the existing complete native test application.

    @param configuration Independently linked native application configuration.
    @return Exact executable, source, terminal and original process membership.
    """
    context = create_run_context("space", "surface-entry-chemical-producers")
    context.prepare_runtime()
    application = ROOT / "build" / configuration / "openttd_test.exe"
    log = context.logs / "producers.log"
    selector = "[layer-processor-definition],[layer-processor],[layer-processor-delivery],[layer-recipe-batch]"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), selector, "--durations", "yes"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration))
    terminal = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    required = ("Selected Surface processor definitions fund", "Native Surface processors execute every frozen",
                "Native processor source refusal retains", "Shared recipe commit preserves exact all-input")
    # All four public boundaries are necessary: factory geometry, actual work,
    # source-eligible physical delivery and atomic inventory settlement.
    assert result.returncode == 0 and match and all(name in terminal for name in required), log
    assert int(match[2]) == 11, terminal
    files = ("layer_processor_definition.cpp", "layer_processor.cpp", "layer_processor_delivery.cpp", "layer_recipe_batch.cpp",
             "processor_fixture.h")
    return dict(application=identity(application), terminal=identity(log), assertions=int(match[1]), cases=int(match[2]),
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), *[identity(ROOT / "src/tests" / name) for name in files]],
                component_fixture=True, genuine_world_predecessor=False, full_regression=False)


def main():
    """Publish one bounded chemical producer receipt without rebuilding applications."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    # Evidence is immutable and lives under this repository's logs directory.
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), assertions=result["assertions"], cases=result["cases"])), flush=True)


if __name__ == "__main__":
    main()
