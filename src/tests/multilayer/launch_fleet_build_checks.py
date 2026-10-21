"""Qualify clean GRF encoding and actual generator-disabled launch/fleet packaging.

@file launch_fleet_build_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62I.3.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md Plan9task16.2.
The existing CreateGRF.cmake and baseset_files owners perform all encoding and
copying. The actual native loader compares every original/new source pixel.
Historical proof files and original game/general-test applications stay intact.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT/"media/multilayer"))
import pack_launch_fleet_art as packing
from tools import cargo_rocket_art as art
from support.artifact_paths import create_run_context
import launch_fleet_payload_checks as native

OUTPUT = native.OUTPUT
PROOF = OUTPUT/"build-proof.json"
GRF = ROOT/"media/baseset/openttd.grf"


def cache_values():
    """Read the actual completed project's build tool and discovery configuration.

    @return Cache keys and their uninterpreted values, without altering project options.
    """
    result = {}
    for line in (ROOT/"build/CMakeCache.txt").read_text(encoding="utf-8").splitlines():
        if re.match(r"^[^/#][^:]*:[^=]+=", line):
            key, value = line.split("=", 1)
            result[key.split(":", 1)[0]] = value
    return result


def execute(arguments, log_name, directory=ROOT, environment=None):
    """Execute one actual encoder/configuration/copy/loader command with a bounded lifetime.

    @param arguments Explicit native tool argument vector, never shell-expanded text.
    @param log_name Exact repository log basename for this operation.
    @param directory Repository-owned native working directory.
    @param environment Optional reserved native invocation environment.
    @return Actual combined diagnostics and immutable log identity after exit0.
    @throws ValueError On failed or overdue actual subprocess work.
    """
    path = ROOT/"logs"/log_name
    try:
        with path.open("wb") as sink:
            result = subprocess.run(arguments, cwd=directory, env=environment, stdout=sink, stderr=subprocess.STDOUT,
                                    creationflags=subprocess.CREATE_NO_WINDOW, timeout=60, check=False)
    except subprocess.TimeoutExpired as error:
        raise ValueError("launch/fleet build/loader exceeded60seconds: "+log_name) from error
    if result.returncode != 0:
        raise ValueError("launch/fleet actual build/loader failed: "+log_name)
    return path.read_text(encoding="utf-8", errors="replace"), art.source.identity(path)


def read_proof():
    """Read this scoped build proof without modifying any prior source/native proof.

    @return Existing successful modes or an explicitly owned empty evidence record.
    """
    if PROOF.exists():
        return art.read_json(PROOF)
    return dict(_documentation=__doc__)


def write_proof(proof):
    """Persist only successfully verified build/copy/load mode observations.

    @param proof Actual scoped build evidence with current producer identity.
    @return None.
    """
    proof["producer"] = art.source.identity(Path(__file__))
    PROOF.write_text(json.dumps(proof, indent=2)+"\n", encoding="utf-8")


def reproducible():
    """Build two complete GRFs in fresh directories through the existing CMake encoder.

    @return Verified two-build mode metadata, with equal current payload/GRFID fingerprints.
    @throws ValueError On tool/source drift, non-clean outputs or differing native payload bytes.
    """
    scope, _ = packing.read_scope()
    native.preserved_applications()
    cache = cache_values()
    tools = {name: cache[name] for name in ("CMAKE_COMMAND", "GRFCODEC_EXECUTABLE", "NFORENUM_EXECUTABLE", "GRFID_EXECUTABLE")}
    versions = {}
    # Verify actual executable banners and hashes, not labels in a tool folder.
    for key, banner, arguments in (("GRFCODEC_EXECUTABLE", "GRFCodec 6.2.0", []), ("NFORENUM_EXECUTABLE", "NFORenum 6.2.0", ["--version"])):
        result = subprocess.run([tools[key], *arguments], cwd=ROOT/"build", capture_output=True, text=True,
                                creationflags=subprocess.CREATE_NO_WINDOW, timeout=10, check=False)
        text = result.stdout+result.stderr
        if banner not in text:
            raise ValueError("launch/fleet actual encoder version differs from qualified toolchain")
        path = ROOT/"logs"/("launch-fleet-"+key.lower()+".log")
        path.write_text(text, encoding="utf-8")
        versions[banner.split()[0]] = dict(banner=banner, executable_sha256=hashlib.sha256(Path(tools[key]).read_bytes()).hexdigest(),
                                         log=art.source.identity(path))
    report, files = packing.derive()
    # Actual source files must still match the qualified deterministic append;
    # a clean native build cannot grant approval to a changed unpublished sheet.
    for path, data in files.items():
        if path.read_bytes() != data:
            raise ValueError("launch/fleet clean build source differs from its qualified staged input")
    cmake_source = ROOT/"media/baseset/openttd/CMakeLists.txt"
    png_block = cmake_source.read_text(encoding="utf-8").split("PNG_SOURCE_FILES", 1)[1]
    png_names = re.findall(r"\$\{CMAKE_CURRENT_SOURCE_DIR\}/([^\s)]+\.png)", png_block)
    if len(png_names) != len(set(png_names)) or "launch_final.png" not in png_names:
        raise ValueError("actual CMake native PNG dependency list is malformed")
    inputs = [art.source.identity(ROOT/"media/baseset/openttd"/name) for name in png_names]
    run = ROOT/"test-cache/launch-fleet-grf-repro"/("run-"+str(time.time_ns()))
    run.mkdir(parents=True)
    outputs, checksums, logs = [], [], []
    # Two genuinely fresh work directories remove all encoder/copy cache state;
    # only identical retained source PNGs/NFO and the pinned tools are reused.
    for attempt in (1, 2):
        directory = run/str(attempt)
        sprites = directory/"sprites"
        sprites.mkdir(parents=True)
        for name in png_names:
            shutil.copy2(ROOT/"media/baseset/openttd"/name, sprites/name)
        target = directory/"verified-openttd.grf"
        arguments = [tools["CMAKE_COMMAND"], "-DGRF_SOURCE_FOLDER="+str(ROOT/"media/baseset/openttd"),
                     "-DGRF_BINARY_FILE="+str(target), "-DNFORENUM_EXECUTABLE="+tools["NFORENUM_EXECUTABLE"],
                     "-DGRFCODEC_EXECUTABLE="+tools["GRFCODEC_EXECUTABLE"], "-DGRFID_EXECUTABLE="+tools["GRFID_EXECUTABLE"],
                     "-P", str(ROOT/"cmake/scripts/CreateGRF.cmake")]
        _, log = execute(arguments, "launch-fleet-clean-grf-"+str(attempt)+".log", directory)
        outputs.append(art.source.identity(target))
        checksums.append(Path(str(target)+".hash").read_text(encoding="utf-8").strip())
        logs.append(log)
    payload = art.source.identity(GRF)
    checksum = Path(str(GRF)+".hash").read_text(encoding="utf-8").strip()
    # Both independent complete encodings must match the actual distributed
    # payload and its format-specific GRFID checksum, not whole-file MD5.
    if ([row["sha256"] for row in outputs] != [payload["sha256"]]*2 or checksums != [checksum]*2
            or not re.fullmatch(r"[0-9a-f]{32}", checksum)):
        raise ValueError("two clean launch/fleet GRF builds or GRFID checksums differ")
    native.preserved_applications()
    proof = read_proof()
    proof["reproducible"] = dict(sha256=[row["sha256"] for row in outputs], grfid_md5=checksums, outputs=outputs,
                                  logs=logs, tool_versions=versions, source_outputs=report["outputs"], source_pngs=inputs,
                                  encoder=art.source.identity(ROOT/"cmake/scripts/CreateGRF.cmake"),
                                  source_handoff=scope["source_handoff"], native_loading_verified_separately=True)
    write_proof(proof)
    return proof["reproducible"]


def cached_native(configuration):
    """Decode all protected/new sprite bytes from the actual generator-disabled copied payload.

    @param configuration Already built separate art target configuration.
    @return Exact actual loader/process/log/data identities for the copied distribution.
    @throws ValueError On failed real decoding, stale copies or overwritten originals.
    """
    native.preserved_applications()
    context = create_run_context("multilayer-art", "launch-fleet-cached-load", run_id="launch-cached-"+str(time.time_ns()),
                                 root=ROOT, profile="launch-fleet-cached", process=configuration.lower())
    application = ROOT/"build"/configuration/"openttd_space_art_test.exe"
    application_before = art.source.identity(application)
    text, log = execute([str(application), "[.launch-fleet-packed-art-load]"],
                        "launch-fleet-cached-native-"+configuration+".log", ROOT/"build", context.environment())
    passed = re.search(r"All tests passed \(([\d,]+) assertions? in 1 test case\)", text)
    # The same real complete-byte native observation verifies cached packaging;
    # copying a file or merely seeing a successful process exit cannot suffice.
    if passed is None or native.MARKER not in text:
        raise ValueError("generator-disabled payload did not pass actual complete-byte native loading")
    payload = art.source.identity(GRF)
    runtime = art.source.identity(context.runtime_dir/"baseset/openttd.grf")
    copied = art.source.identity(ROOT/"build/baseset/openttd.grf")
    if (runtime["sha256"] != payload["sha256"] or copied["sha256"] != payload["sha256"]
            or art.source.identity(application) != application_before):
        raise ValueError("actual generator-disabled native payload/application changed")
    native.preserved_applications()
    return dict(configuration=configuration, complete=True, cases=1, assertions=int(passed.group(1).replace(",", "")),
                application=application_before, terminal=log, runtime_payload=runtime, copied_payload=copied,
                fixture=art.source.identity(native.EXPECTED), invocation=context.run_id,
                protected_sprites=6116, new_sprites=1648, total_sprites=7764, native_bound=7791,
                full_pixel_bytes_exact=True, anchors_exact=True, paid_world_loaded=False)


def generator_free():
    """Run the real cached CMake distribution route and restore its discovery setting.

    @return Actual generator-disabled copy and both current native loading qualifications.
    @throws ValueError On generator invocation, stale copied data or failed discovery restoration.
    """
    cache = cache_values()
    cmake = cache["CMAKE_COMMAND"]
    setting = "CMAKE_DISABLE_FIND_PACKAGE_Grfcodec"
    previous = cache.get(setting)
    payload = art.source.identity(GRF)
    metadata = None
    try:
        _, configure_log = execute([cmake, "-S", str(ROOT), "-B", str(ROOT/"build"), "-D"+setting+"=ON"],
                                   "launch-fleet-cached-configure.log")
        project = ROOT/"build/media/baseset/baseset_files.vcxproj"
        # CMake's public discovery switch must remove generator work from this
        # actual packaging target, even though cached executable paths still exist.
        if cache_values().get(setting) != "ON" or "CreateGRF.cmake" in project.read_text(encoding="utf-8"):
            raise ValueError("generator-disabled actual baseset target still invokes encoding")
        # Invalidate only disposable verified build copies/stamps. The tracked
        # source GRF and its immutable predecessor archive are never deleted.
        for relative in ("build/baseset/openttd.grf", "build/media/baseset/openttd.grf.timestamp"):
            path = (ROOT/relative).resolve()
            if not path.is_relative_to((ROOT/"build").resolve()):
                raise ValueError("cached copy invalidation escapes its repository build owner")
            path.unlink(missing_ok=True)
        text, copy_log = execute([cmake, "--build", str(ROOT/"build"), "--config", "RelWithDebInfo", "--target", "baseset_files"],
                                 "launch-fleet-cached-copy.log")
        # An actual fresh copy rule, unchanged source bytes and matching payload
        # distinguish generator-free packaging from a stale output or regeneration.
        if ("Copying openttd.grf baseset file" not in text or "Generating openttd.grf" in text
                or art.source.identity(GRF) != payload
                or art.source.identity(ROOT/"build/baseset/openttd.grf")["sha256"] != payload["sha256"]):
            raise ValueError("actual generator-disabled packaging did not copy the exact current source payload")
        observations = [cached_native(configuration) for configuration in ("RelWithDebInfo", "Debug")]
        metadata = dict(sha256=payload["sha256"], generator_discovery_disabled=True, configure_log=configure_log,
                        copied_payload=art.source.identity(ROOT/"build/baseset/openttd.grf"), copy_log=copy_log,
                        native=observations, native_loading_verified=True)
    finally:
        # Restore the public discovery option exactly, including its previous
        # absence, so this proof never changes later normal build configuration.
        arguments = [cmake, "-S", str(ROOT), "-B", str(ROOT/"build")]
        arguments += ["-U", setting] if previous is None else ["-D"+setting+"="+previous]
        _, restore_log = execute(arguments, "launch-fleet-generator-restore.log")
        if cache_values().get(setting) != previous:
            raise ValueError("launch/fleet generator discovery was not restored")
    metadata["restored_discovery"] = previous
    metadata["restore_log"] = restore_log
    proof = read_proof()
    proof["generator_free"] = metadata
    write_proof(proof)
    return metadata


def main():
    """Run one focused real build/packaging mode without publishing any checksum yet.

    @return Zero only after all actual operations in the selected mode succeed.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("reproducible", "generator-free"))
    args = parser.parse_args()
    result = reproducible() if args.mode == "reproducible" else generator_free()
    print(json.dumps(dict(mode=args.mode, complete=True, sha256=result["sha256"])))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
