"""Qualify current compiled original launch producers and complete immutable packed pixels.

@file launch_fleet_runtime_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JC.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
Historical source/packing/component receipts remain immutable. This explicit
current compatibility boundary makes no ordinary paid-game acceptance claim.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import time
from support.artifact_paths import create_run_context
import launch_facility_selector_checks as selectors
import launch_fleet_payload_checks as payload

ROOT = selectors.ROOT
OUTPUT = payload.OUTPUT.parent / "launch-fleet-runtime-v1"
FILTER = "[launch-site],[launch-assembly-contract],[space-logistics-contract],[launch-facility-art],[launch-rocket-art]"


def run(configuration):
    """Execute original current producers and the immutable complete decoder separately.

    @param configuration Existing Debug or RelWithDebInfo isolated engine/art target.
    @return Current compatibility receipt preserving previous qualification artifacts.
    @throws ValueError On failed/partial/overdue tests, policy drift or changed originals.
    """
    policy, before = selectors.boundary()
    application = ROOT / "build" / configuration / "openttd_space_art_test.exe"
    executable = selectors.art.source.identity(application)
    context = create_run_context("multilayer-art", "launch-fleet-current-producers", run_id="launch-producers-" + str(time.time_ns()), root=ROOT, profile="launch-fleet-art", process=configuration.lower())
    terminal = ROOT / ("logs/launch-fleet-current-producers-" + configuration + ".log")
    started = time.monotonic()
    with terminal.open("w", encoding="utf-8") as output:
        result = subprocess.run([str(application), FILTER, "--reporter", "console", "--durations", "yes"], cwd=ROOT / "build", env=context.environment(), stdout=output, stderr=subprocess.STDOUT, timeout=60, check=False, creationflags=subprocess.CREATE_NO_WINDOW)
    text = terminal.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \(([\d,]+) assertions in 20 test cases\)", text)
    # All eleven original geometry/phase/custody producers and nine current
    # source selectors must execute; a partial successful filter is insufficient.
    if result.returncode != 0 or passed is None:
        raise ValueError("current original launch producers failed: " + str(terminal))
    comparisons = selectors.rocket.verify_observations(text, policy)
    refusals = selectors.rocket.verify_refusals(text, policy)
    decoded = payload.run(configuration, current=True)
    # The decoder runs in a fresh process, retaining actual sprite discovery;
    # component mock setup can never supply a passing native packed-cache proof.
    if selectors.boundary()[1] != before or selectors.art.source.identity(application) != executable:
        raise ValueError("current launch sources/application changed during qualification")
    sources = ("src/tests/launch_site.cpp", "src/tests/launch_production.cpp", "src/tests/space_logistics.cpp", "src/tests/launch_flight.cpp", "src/tests/space_descent.cpp", "src/tests/multilayer/launch_fleet_runtime_checks.py", "src/tests/multilayer/launch_fleet_payload_checks.py", "CMakeLists.txt")
    proof = dict(_documentation=__doc__, complete=True, configuration=configuration, application=executable,
                 cases=20, assertions=int(passed.group(1).replace(",", "")), elapsed_seconds=round(time.monotonic()-started,3),
                 terminal=selectors.art.source.identity(terminal), source_policy_comparisons=comparisons,
                 detached_policy_refusals=refusals, decoded_payload=selectors.art.source.identity(OUTPUT/("native-"+configuration+".json")),
                 sources=[selectors.art.source.identity(ROOT/name) for name in sources], boundary=before,
                 total_decoded_sprites=decoded["total_sprites"], full_pixel_bytes_exact=decoded["full_pixel_bytes_exact"],
                 ordinary_game_reviewed=False, public_runtime_activated=False)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    (OUTPUT/("producers-"+configuration+".json")).write_text(json.dumps(proof,indent=2)+"\n",encoding="utf-8")
    print(configuration,"current producer assertions",proof["assertions"],"cases",20,"decoded sprites",decoded["total_sprites"],flush=True)
    return proof


def main():
    """Execute one bounded current compatibility configuration."""
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration",choices=("Debug","RelWithDebInfo"),required=True)
    run(parser.parse_args().configuration)


if __name__ == "__main__":
    main()
