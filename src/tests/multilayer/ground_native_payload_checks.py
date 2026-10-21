"""Build or run isolated actual native selected-underground loader qualification.

@file ground_native_payload_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79E.2/.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Each native observation gets a repository-owned runtime context and60second cap.
The separate target preserves all preexisting game and test applications.
"""

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
import time

from ground_pack_check_support import ROOT, handoff
from ground_payload_build_checks import PROOF, execute, identity, preserved
from ground_payload_fixture import FIXTURE, SUMMARY, checked_build
from support.artifact_paths import create_run_context

MARKER = "GROUND_PACKED_ART_LOADED sprites=22018 protected=7764 new=14254 families=47 native_start=7791 native_bound=22045 full_pixel_bytes=exact"


def build(configuration, output):
    """Compile only the new observer target/shared engine and materialize its baseset.

    @param configuration Native Debug or RelWithDebInfo build configuration.
    @param output Repository log directory for exact configure/build diagnostics.
    @return Actual isolated executable identity and command results.
    """
    _scope, before, _public = handoff.read_scope()
    preserved(before)
    cache = dict(re.findall(r"^([^#/:=]+):[^=]+=(.*)$", (ROOT / "build/CMakeCache.txt").read_text(encoding="utf-8"), re.MULTILINE))
    cmake = cache["CMAKE_COMMAND"]
    commands = [execute([cmake, "-S", str(ROOT), "-B", str(ROOT / "build")], ROOT, output / "configure.log")]
    commands.append(execute([cmake, "--build", str(ROOT / "build"), "--config", configuration, "--target",
                             "openttd_ground_art_test", "baseset_files", "--parallel", "2", "--", "/nodeReuse:false", "/p:CL_MPCount=2"],
                            ROOT, output / "build.log"))
    preserved(before)
    result = {"_documentation": __doc__, "configuration": configuration, "commands": commands,
              "application": identity(ROOT / "build" / configuration / "openttd_ground_art_test.exe"),
              "native_test": identity(ROOT / "src/tests/ground_packed_load.cpp"), "root_cmake": identity(ROOT / "CMakeLists.txt")}
    (output / "build.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


def observe(configuration, output, cached):
    """Compare every prior/new pixel through one actual engine startup process.

    @param configuration Native Debug or RelWithDebInfo executable configuration.
    @param output Repository log directory for this exact process and receipt.
    @param cached Whether this is the explicitly generator-disabled packaging observation.
    @return Actual terminal/application/runtime/payload-bound native loading proof.
    """
    checked_build()
    _scope, before, _public = handoff.read_scope()
    preserved(before)
    summary = handoff.read_json(SUMMARY.relative_to(ROOT).as_posix())
    if identity(FIXTURE) != summary["fixture"] or identity(PROOF) != summary["build_proof"]:
        # Complete current source expectations and build proof remain the same
        # actual owners; an unrelated or stale binary fixture cannot qualify art.
        raise ValueError("ground native source expectations changed")
    for row in summary["source_outputs"]:
        if identity(handoff.bound(row["path"])) != row:
            raise ValueError("ground native staged source changed")
    payload = identity(ROOT / "media/baseset/openttd.grf")
    copied = identity(ROOT / "build/baseset/openttd.grf")
    if payload["sha256"] != copied["sha256"] or payload != summary["payload"]:
        # Normal startup must consume this exact current container through
        # distributed discovery; a cached older payload cannot stand in for it.
        raise ValueError("ground materialized payload does not match current qualification")
    compiled = ROOT / "build" / configuration / "openttd_ground_art_test.exe"
    compiled_before = identity(compiled)
    application = PROOF.parent / "bin" / configuration / compiled.name
    # Later selected-consumer groups legitimately relink this separate target.
    # Retain and invoke its exact packing-qualification image so that those
    # later builds cannot invalidate this completed native-loading evidence.
    if application.exists() and handoff.digest(application) != compiled_before["sha256"]:
        raise ValueError("ground retained packing application differs from the current loader build")
    application.parent.mkdir(parents=True, exist_ok=True)
    if not application.exists():
        application.write_bytes(compiled.read_bytes())
    application_before = identity(application)
    context = create_run_context("multilayer-art", "ground-packed-load", run_id="ground-" + str(time.time_ns()),
                                 root=ROOT, profile="ground-art", process=configuration.lower())
    terminal = output / "native.log"
    started = time.monotonic()
    arguments = [str(application), "[.ground-packed-art-load]"]
    try:
        with terminal.open("w", encoding="utf-8") as stream:
            result = subprocess.run(arguments, cwd=ROOT / "build", env=context.environment(), stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=60, check=False)
    except subprocess.TimeoutExpired as error:
        raise ValueError("ground actual native observation exceeded60seconds") from error
    text = terminal.read_text(encoding="utf-8", errors="replace")
    passed = re.search(r"All tests passed \(([\d,]+) assertions? in 1 test case\)", text)
    if result.returncode != 0 or passed is None or MARKER not in text:
        # Require the actual complete-byte marker and Catch terminal success,
        # because a zero exit or a source-only report does not prove loading.
        raise ValueError("ground native complete-byte observation failed; inspect " + str(terminal))
    runtime_payload = identity(context.runtime_dir / "baseset/openttd.grf")
    if runtime_payload["sha256"] != payload["sha256"] or identity(application) != application_before:
        # Bind observation to the exact runtime-discovered payload and original
        # observer executable, even if another build is active in the checkout.
        raise ValueError("ground actual observer application/runtime payload changed")
    preserved(before)
    proof = {"_documentation": __doc__, "complete": True, "configuration": configuration, "cases": 1,
             "assertions": int(passed.group(1).replace(",", "")), "exit_code": result.returncode,
             "seconds": round(time.monotonic() - started, 3), "application": application_before,
             "compiled_application": compiled_before,
             "arguments": arguments[1:], "terminal": identity(terminal), "invocation": context.run_id,
             "fixture": identity(FIXTURE), "expectations": identity(SUMMARY), "build_proof": identity(PROOF),
             "native_test": identity(ROOT / "src/tests/ground_packed_load.cpp"), "producer": identity(Path(__file__)),
             "payload": payload, "copied_payload": copied, "runtime_payload": runtime_payload,
             "protected_sprites": 7764, "new_sprites": 14254, "total_sprites": 22018, "native_bound": 22045,
             "full_pixel_bytes_exact": True, "anchors_exact": True, "generator_disabled_observation": cached,
             "paid_world_loaded": False, "runtime_activated": False}
    target = PROOF.parent / (("cached-native-" if cached else "native-") + configuration + ".json")
    target.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    return proof


def main():
    """Build or observe one configuration per invocation to keep native bursts bounded.

    @return None; loader success grants no selected consumer or paid-world rendering claim.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("build", "observe"), required=True)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--run", required=True, help="Repository logs directory for the exact invocation")
    parser.add_argument("--cached", action="store_true", help="Record a generator-disabled packaging observation")
    args = parser.parse_args()
    output = handoff.bound(args.run)
    if not output.is_relative_to(ROOT / "logs"):
        raise ValueError("ground native diagnostics belong in repository logs")
    output.mkdir(parents=True, exist_ok=True)
    result = build(args.configuration, output) if args.mode == "build" else observe(args.configuration, output, args.cached)
    print(json.dumps({key: result[key] for key in ("configuration", "complete", "cases", "assertions", "seconds") if key in result}))


if __name__ == "__main__":
    main()
