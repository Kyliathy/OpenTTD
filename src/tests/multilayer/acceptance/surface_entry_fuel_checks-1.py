"""Validate original bounded freight income before genuine Propellant finance.

@file surface_entry_fuel_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BI.1.
This existing prepared boundary proves only native payment semantics. It never
provides cargo, money or a predecessor to the authentic commercial network.
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
    """Run only the original positive-tariff versus paid-supply final-delivery case.

    @param configuration Independently linked focused native application.
    @return Exact successful process, native assertions and source identities.
    """
    context = create_run_context("space", "surface-entry-fuel-income-producer")
    context.prepare_runtime()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    selector = "Bought material final intake stays bounded while ordinary freight still earns income"
    log = context.logs / "producer.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), selector, "--durations", "yes"], cwd=ROOT / "build", stdout=sink,
                            stderr=subprocess.STDOUT, timeout=110, env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration))
    terminal = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and match and int(match[2]) == 1 and selector in terminal, log
    return dict(application=identity(application), terminal=identity(log), assertions=int(match[1]), cases=1,
                native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_supply_custody.cpp"), identity(ROOT / "src/economy.cpp")],
                component_fixture=True, genuine_world_predecessor=False)


def main():
    """Retain one focused existing payment producer receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    result = execute(args.configuration)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration,
                                     operation=result), indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(output=target.relative_to(ROOT).as_posix(), assertions=result["assertions"])), flush=True)


if __name__ == "__main__":
    main()
