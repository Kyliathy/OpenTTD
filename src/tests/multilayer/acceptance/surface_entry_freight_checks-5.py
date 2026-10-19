"""Replay exactly3000earned native game ticks from actual original or ordinary-resaved products.

@file surface_entry_freight_checks-5.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BD.3.
The real StateGameLoop joins restored routing workers and owns every earned
clock/production/vehicle/payment mutation; no saved tick is synthesized.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, identity
from artifact_paths import create_run_context
from runner import run_native

OBSERVATIONS = runpy.run_path(str(Path(__file__).with_name("surface_entry_freight_checks-3.py")))
REVISION = runpy.run_path(str(Path(__file__).with_name("surface_entry_checks-7.py")))


def execute(configuration, source, scenario):
    """Earn exact real native simulation time while preserving original physical owners.

    @param configuration Independently linked focused application using original game objects.
    @param source Actual original productive output or its ordinary paused resave.
    @param scenario Explicit original/resaved process role, not a replacement world identity.
    @return Exact original input/output, actual tick counts, products and native process.
    """
    context = create_run_context("space", "surface-entry-replay-" + scenario)
    context.prepare_runtime()
    retained = context.directory / "harness.py"
    retained.write_bytes(Path(__file__).read_bytes())
    (context.runtime_dir / "scripts/game_start.scr").write_text("surface_entry_production\n", encoding="utf-8")
    application = ROOT / "build" / configuration / "openttd_surface_supply_test.exe"
    original = identity(source)
    log = context.logs / "replay.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(application), "[.surface-entry-production]"], cwd=ROOT / "build",
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration, OTTD_SURFACE_ENTRY_SOURCE=str(source)))
    terminal = log.read_text(errors="replace")
    assert result.returncode == 0 and "All tests passed" in terminal and "SURFACE_ENTRY_REPLAY_COMPLETE" in terminal, log
    assert identity(source) == original
    before = context.saves / "surface-entry-before-replay-440.sav"
    output = context.saves / "surface-entry-replayed-440.sav"
    first, final = [OBSERVATIONS["observe"](path) for path in (source, output)]
    assert OBSERVATIONS["observe"](before) == first
    assert first["ready"] and final["ready"]
    assert first["parked_steel"] == final["parked_steel"] == 36
    assert first["parked_goods"] == final["parked_goods"] == 14
    assert first["debt"] == final["debt"] == 2000000
    assert first["cash"] - final["cash"] == final["expense"] - first["expense"]
    dates = [OBSERVATIONS["saved_table"](path, b"DATE")[0] for path in (source, before, output)]
    assert dates[0]["tick_counter"] == dates[1]["tick_counter"]
    assert dates[2]["tick_counter"] - dates[0]["tick_counter"] == 3000
    assert dates[0]["id"] == dates[1]["id"] == dates[2]["id"]
    # The focused application's exact revision already exists in the genuine
    # source history. Loading appends only that known Load/Revision, preserving
    # every earlier action; no arbitrary settings or cheat history is excluded.
    if OBSERVATIONS["saved_table"](source, b"GLOG") != OBSERVATIONS["saved_table"](before, b"GLOG"):
        REVISION["known_revision"](source, before)
    calls, waits = [int(value) for value in re.findall(r"earned=3000 calls=(\d+) waits=(\d+)", terminal)[0]]
    assert calls >= 3000 and waits >= 0
    return dict(source=original, output=identity(output), before=identity(before), application=identity(application), terminal=identity(log),
                initial=first, final=final, earned_ticks=3000, native_loop_calls=calls, worker_waits=waits,
                retained_harness=identity(retained), native_processes=[identity(p) for p in sorted(context.reports.glob("native-*.json"))],
                sources=[identity(Path(__file__)), identity(ROOT / "src/tests/surface_entry_production.cpp"),
                         identity(ROOT / "src/tests/multilayer/acceptance/surface_entry_freight_checks-3.py")])


def main():
    """Run one bounded exact native continuation and publish its original membership."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--scenario", choices=("original", "resaved"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source, target = [(ROOT / item).resolve() for item in (args.source, args.output)]
    assert source.is_relative_to(ROOT) and source.is_file()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=args.configuration, scenario=args.scenario,
                                     operation=execute(args.configuration, source, args.scenario)), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
