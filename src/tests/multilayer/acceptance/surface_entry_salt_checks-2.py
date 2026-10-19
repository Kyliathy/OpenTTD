"""Verify bounded original portal/TBM and spoil producer contracts before genuine Salt mutation.

@file surface_entry_salt_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BE.
These existing component fixtures prove public owner boundaries only. Their
prepared maps/company balances never replace the authentic game's ancestry.
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
    """Run one focused existing producer burst, never a full regression.

    @param configuration Existing complete native test executable.
    @param phase Portal/acquisition/commissioning or original spoil accounting boundaries.
    @return Exact native component process, case/assertion counts and public source identities.
    """
    context = create_run_context("space", "surface-entry-salt-producers-" + phase)
    context.prepare_runtime()
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    tags = ("[tbm-acquisition],[portal-bootstrap],[tbm-commission],[underground-area-quote]" if phase == "portal-tbm" else
            "[spoil-receiver-definition],[spoil-receiver-intake],[spoil-receiver-delivery],[spoil-receiver-accounting],[spoil-receiver-leg]")
    expected = ("Whole-consist preparation", "Sealed portal issues", "A legal Surface commissioning", "Commissioning area preview") if phase == "portal-tbm" else (
        "Both receivers install", "An Earth receiver", "Unloading at the origin", "Spoil transferred onward", "Spoil leaves its wagons")
    log = context.logs / "producers.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), tags, "--durations", "yes"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration))
    terminal = log.read_text(errors="replace")
    match = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", terminal)
    assert result.returncode == 0 and match and all(name in terminal for name in expected), log
    assert int(match[2]) == (11 if phase == "portal-tbm" else 13), log
    sources = ("src/layer_portal_cmd.h", "src/layer_portal_bootstrap.cpp", "src/tbm.h", "src/tbm_cmd.h", "src/tbm_acquisition.cpp", "src/tbm_commission.cpp",
               "src/tests/tbm_acquisition.cpp", "src/tests/portal_bootstrap.cpp", "src/tests/tbm_commission.cpp") if phase == "portal-tbm" else (
                   "src/spoil_receiver.h", "src/spoil_receiver.cpp", "src/tests/spoil_receiver_definition.cpp", "src/tests/spoil_receiver_intake.cpp",
                   "src/tests/spoil_receiver_delivery.cpp", "src/tests/spoil_receiver_accounting.cpp", "src/tests/spoil_receiver_leg.cpp")
    return dict(application=identity(application), terminal=identity(log), assertions=int(match[1]), cases=int(match[2]),
                native_processes=[identity(p) for p in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), *[identity(ROOT / source) for source in sources]],
                component_fixture=True, genuine_world_predecessor=False)


def main():
    """Select one focused native producer burst and retain fresh exact membership."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--phase", choices=("portal-tbm", "spoil"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, phase=args.phase,
                                     operation=execute(args.configuration, args.phase)), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
