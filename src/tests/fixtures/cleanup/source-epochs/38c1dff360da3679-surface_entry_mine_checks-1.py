"""Run only newly consumed finite-mine and retained-cutter public producer gates.

@file surface_entry_mine_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BF.1.
Prepared component worlds prove behavior only. They never become the genuine
game's predecessor or provide it with floor, vehicles, stock or company funds.
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


def execute(configuration, phase):
    """Run one bounded existing producer boundary burst.

    @param configuration Independently linked focused native application.
    @param phase Mine definition/admission or finite work/dispatch or actual cutter reuse.
    @return Original process membership, case counts and tested source identities.
    """
    context = create_run_context("space", "surface-entry-mine-producers-" + phase)
    context.prepare_runtime()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    selectors = dict(admission="[layer-industry-admission],[layer-mine-geology],[layer-mine-funding],[layer-mine-definition],[layer-mine-art-geometry]",
                     production="[layer-mine-production],[layer-mine-dispatch]", reuse="[tbm-depot-recommission]")
    required = dict(admission=("Mine admission checks every", "Selected native mines require complete"),
                    production=("Every native mine produces", "Finite mine output remains"), reuse=("A withdrawn native machine recommissions",))
    log = context.logs / "producers.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), selectors[phase], "--durations", "yes"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration))
    terminal = log.read_text(errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and match and all(name in terminal for name in required[phase]), log
    files = dict(admission=("layer_industry_admission.cpp", "layer_mine_definition.cpp"), production=("layer_mine_production.cpp", "layer_mine_dispatch.cpp"),
                 reuse=("tbm_depot_workflow.cpp", "underground_depot_fixture.h"))
    return dict(application=identity(application), terminal=identity(log), assertions=int(match[1]), cases=int(match[2]),
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), *[identity(ROOT / "src/tests" / name) for name in files[phase]]],
                component_fixture=True, genuine_world_predecessor=False)


def main():
    """Retain one narrowly selected original public producer receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("admission", "production", "reuse"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration, args.phase)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase,
                                     operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), assertions=result["assertions"], cases=result["cases"])), flush=True)


if __name__ == "__main__":
    main()
