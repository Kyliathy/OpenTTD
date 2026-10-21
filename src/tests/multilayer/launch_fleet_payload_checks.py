"""Run one focused real-application full-pixel loader qualification per configuration.

@file launch_fleet_payload_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups62I.2/62JC.1.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
Each native process gets an isolated data home and a60second timeout. No paid
save is loaded; the four original game/general-test applications stay unchanged.
"""

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT/"media/multilayer"))
from tools import cargo_rocket_art as art
from support.artifact_paths import create_run_context

OUTPUT = ROOT/"logs/proofs/mlm/native-art/space/launch-fleet-packing-v1"
EXPECTED = ROOT/"logs/proofs/mlm/native-art/expected/launch-fleet-packed-sprites.bin"
MARKER = "LAUNCH_FLEET_PACKED_ART_LOADED sprites=7764 protected=6116 new=1648 families=2 native_start=6143 native_bound=7791 full_pixel_bytes=exact"


def preserved_applications():
    """Verify original applications still exist byte-for-byte at their accepted paths.

    @return Original four application identities, independently checked now.
    @throws ValueError On an overwritten original game or general-test executable.
    """
    originals = art.read_json(OUTPUT/"preexisting-applications.json")["applications"]
    # A new isolated graphics qualification must not replace financial/world
    # producer applications or silently rebase their historical proof identities.
    for reference in originals:
        if art.source.identity(ROOT/reference["path"]) != reference:
            raise ValueError("original application changed during launch/fleet graphics qualification")
    return originals


def run(configuration, current=False):
    """Qualify every protected/new native pixel through one actual engine startup process.

    @param configuration Debug or RelWithDebInfo, using the separate existing art target.
    @param current Keep the earlier packing receipts/logs immutable while qualifying the integrated renderer application.
    @return Completed proof bound to actual source expectations, application and payload.
    @throws ValueError On stale data, failed/timeout native decoding or changed originals.
    """
    if configuration not in ("Debug", "RelWithDebInfo"):
        raise ValueError("unsupported launch/fleet native configuration")
    originals = preserved_applications()
    application = ROOT/"build"/configuration/"openttd_space_art_test.exe"
    expectations = art.read_json(OUTPUT/"full-pixel-expectations.json")
    # Full native expectations must remain bound to the actual source producer;
    # an arbitrary binary or a changed fixture cannot supply packed-art acceptance.
    if (art.source.identity(EXPECTED) != expectations["fixture"]
            or art.source.identity(ROOT/expectations["producer"]["path"]) != expectations["producer"]
            or expectations["protected_sprites"] != 6116 or expectations["new_sprites"] != 1648
            or expectations["total_sprites"] != 7764 or expectations["native_bound"] != 7791):
        raise ValueError("launch/fleet full-pixel source expectations changed")
    # The actual retained prefix expectations and staged new source files
    # must still agree with the one source snapshot used for this loader run.
    for reference in expectations["protected_expected_fixtures"]+expectations["source_outputs"]:
        if art.source.identity(ROOT/reference["path"]) != reference:
            raise ValueError("launch/fleet historical source expectation or staged input changed")
    source_payload = art.source.identity(ROOT/"media/baseset/openttd.grf")
    copied_payload = art.source.identity(ROOT/"build/baseset/openttd.grf")
    if copied_payload["sha256"] != source_payload["sha256"]:
        raise ValueError("materialize the actual current shared GRF through baseset_files before native loading")
    application_before = art.source.identity(application)
    # The shared artifact owner reserves the invocation atomically before a
    # native child inherits it; environment strings alone grant no data home.
    context = create_run_context("multilayer-art", "launch-fleet-packed-load", run_id="launch-fleet-"+str(time.time_ns()),
                                 root=ROOT, profile="launch-fleet-art", process=configuration.lower())
    runtime = context.runtime_dir
    environment = context.environment()
    label = "launch-fleet-current-native-payload" if current else "launch-fleet-native-payload"
    terminal = ROOT/("logs/"+label+"-"+configuration+".log")
    arguments = [str(application), "[.launch-fleet-packed-art-load]"]
    started = time.monotonic()
    # One explicit hidden loader case executes in its own application process.
    # The timeout prevents a graphical startup failure from silently waiting
    # beyond the user's one-minute harness allowance; no paid world is supplied.
    try:
        with terminal.open("w", encoding="utf-8") as output:
            result = subprocess.run(arguments, cwd=ROOT/"build", env=environment, stdout=output,
                                    stderr=subprocess.STDOUT, timeout=60, check=False)
    except subprocess.TimeoutExpired as error:
        raise ValueError("launch/fleet isolated native loader exceeded60seconds") from error
    text = terminal.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \(([\d,]+) assertions? in 1 test case\)", text)
    # Neither a zero exit code alone nor a source success flag proves native
    # coverage. Require the actual Catch result and the complete-byte marker.
    if result.returncode != 0 or passed is None or MARKER not in text:
        raise ValueError("launch/fleet actual native pixel/anchor decoding failed; see "+terminal.relative_to(ROOT).as_posix())
    actual_runtime_payload = art.source.identity(runtime/"baseset/openttd.grf")
    # Verify all repository discovery copies before/after actual execution so
    # a stale cached base-set cannot masquerade as this newly built payload.
    if (actual_runtime_payload["sha256"] != source_payload["sha256"]
            or art.source.identity(ROOT/source_payload["path"]) != source_payload
            or art.source.identity(ROOT/copied_payload["path"]) != copied_payload
            or art.source.identity(application) != application_before):
        raise ValueError("launch/fleet actual native application/data bytes changed during observation")
    preserved_applications()
    proof = dict(_documentation=__doc__, complete=True, configuration=configuration, cases=1,
                 assertions=int(passed.group(1).replace(",", "")), elapsed_seconds=round(time.monotonic()-started, 3),
                 application=application_before, arguments=arguments[1:], terminal=art.source.identity(terminal),
                 invocation=context.run_id,
                 fixture=art.source.identity(EXPECTED), expectations=art.source.identity(OUTPUT/"full-pixel-expectations.json"),
                 producer=art.source.identity(Path(__file__)), native_test=art.source.identity(ROOT/"src/tests/multilayer_packed_load.cpp"),
                 payload=source_payload, copied_payload=copied_payload, runtime_payload=actual_runtime_payload,
                 source_handoff=expectations["source_handoff"], source_qualification=expectations["source_qualification"],
                 protected_sprites=6116, new_sprites=1648, total_sprites=7764, native_bound=7791,
                 full_pixel_bytes_exact=True, anchors_exact=True, paid_world_loaded=False,
                 original_applications=originals, runtime_activated=False)
    destination = OUTPUT.parent/"launch-fleet-runtime-v1" if current else OUTPUT
    destination.mkdir(parents=True, exist_ok=True)
    (destination/("native-"+configuration+".json")).write_text(json.dumps(proof, indent=2)+"\n", encoding="utf-8")
    return proof


def main():
    """Run one bounded native configuration; invoke separately for the other configuration.

    @return Zero only after real complete native loading passes for the selected build.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--current", action="store_true")
    args = parser.parse_args()
    proof = run(args.configuration, args.current)
    print(json.dumps({name: proof[name] for name in ("configuration", "complete", "cases", "assertions", "total_sprites", "full_pixel_bytes_exact", "elapsed_seconds")}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
