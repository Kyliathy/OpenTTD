"""Verify the existing paid model15 shell, explicit tuning and exact interrupted mining continuation.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md paid deployment handoff
Every native child has a bounded runtime and its own retained invocation evidence.
"""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess

from space_preview_checks import ROOT, chunks, identity, ordinary
from launch_logistics_checks import native as logistics_native
from launch_flight_checks import native as flight_native
from runner import current_run_context, run_native

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]


def native(context, configuration, source, resume=False):
    """Run one actual paid-hull collection or its interrupted continuation.

    @param context Isolated runtime and artifact owner.
    @param configuration Completed native build configuration.
    @param source Exact paid launch or partially extracted native save.
    @param resume Whether the source already holds two tonnes and37 work ticks.
    @return Completed actual native save and assertion count.
    """
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("probe_shell_test\n", encoding="utf-8")
    environment = dict(os.environ, OTTD_PROBE_SHELL_SOURCE=str(source))
    environment.pop("OTTD_PROBE_SHELL_RESUME", None)
    if resume:
        environment["OTTD_PROBE_SHELL_RESUME"] = "1"
    log = context.logs / "native-probe-shell.log"
    with log.open("wb") as sink:
        run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.probe-shell-native]"],
                   cwd=ROOT / "build", env=environment, stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
    evidence = log.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", evidence)
    assert passed and "PROBE_SHELL native=true model=15 same_hull=true tonnes=20" in evidence, log
    return context.saves / "probe-shell-complete-424.sav", int(passed.group(1))


def verify(configuration):
    """Execute the cross-plan launch producer before consuming its hull and compare retained owners.

    @param configuration Completed product/test build, never tested during linking.
    @return Accepted focused report containing exact producer, native and ordinary-load identities.
    """
    context = current_run_context("space", "probe-shell-verify")
    focused = context.for_feature("space", "probe-shell-focused")
    log = focused.logs / "probe-shell-focused.log"
    with log.open("wb") as sink:
        run_native(focused, [str(ROOT / "build" / configuration / "openttd_test.exe"),
                            "[probe-shell],[space-content],[space-native],[asteroid-motion]"],
                   cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=110)
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", log.read_text(encoding="utf-8"))
    assert passed, log
    source = ROOT / "src/tests/fixtures/demos/space/space-launch-hub-1990-408.sav"
    original = identity(source)
    logistics = context.for_feature("space", "probe-shell-paid-logistics")
    logistics_native(logistics, configuration, "prepare", source)
    flight = context.for_feature("space", "probe-shell-paid-flight")
    flight_native(flight, configuration, "prepare", logistics.saves / "logistics-complete-414.sav", "delivery")
    deployed = flight.saves / "flight-complete-414.sav"
    mining = context.for_feature("space", "probe-shell-native")
    complete, assertions = native(mining, configuration, deployed)
    partial = mining.saves / "probe-shell-partial-424.sav"
    replay = context.for_feature("space", "probe-shell-resume")
    resumed, resumed_assertions = native(replay, configuration, partial, True)
    owners = (focused, logistics, flight, mining, replay)
    ordinary_saves = {}
    # Both exact boundaries retain every public owner, not just the probe's reserve or cargo summary; ordinary load must preserve paid receipts and unrelated freight.
    for label, saved in (("partial", partial), ("complete", complete)):
        ordinary_owner = context.for_feature("space", "probe-shell-ordinary-" + label)
        ordinary(ordinary_owner, {"name": "probe-shell-" + label, "snapshot": identity(saved)}, configuration)
        resaved = ordinary_owner.saves / "ordinary-resave.sav"
        expected_version, expected = chunks(saved)
        actual_version, actual = chunks(resaved)
        assert expected_version == 424 and actual_version == 424
        for family in EXACT:
            assert expected[family] == actual[family], (label, family)
        ordinary_saves[label] = identity(resaved)
        owners += (ordinary_owner,)
    # Interrupted work and per-tick operating remainders must converge to the same packets, company ledger and physical owners as uninterrupted collection.
    expected, actual = chunks(complete)[1], chunks(resumed)[1]
    for family in EXACT:
        assert expected[family] == actual[family], ("resume", family)
    assert identity(source) == original
    report = context.reports / "probe-shell.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs10 Group8; same paid model15 hull, typed physical tuning and finite native extraction.",
        "configuration": configuration, "focused_assertions": int(passed.group(1)), "focused_cases": int(passed.group(2)),
        "native_assertions": assertions, "resumed_assertions": resumed_assertions, "exact_owner_families": len(EXACT),
        "source": original, "deployed": identity(deployed), "partial": identity(partial), "complete": identity(complete),
        "resumed": identity(resumed), "ordinary_resaves": ordinary_saves,
        "native_reports": [identity(path) for owner in owners for path in sorted(owner.reports.glob("native-*.json"))],
        "focused_log": identity(log), "content_version": 2, "save_version": 424, "full_regression": False}, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one completed configuration and print its accepted bounded report path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    args = parser.parse_args()
    print(verify(args.configuration).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
