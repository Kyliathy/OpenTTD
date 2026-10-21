"""Build and independently decode two complete selected-underground native payloads.

@file ground_payload_build_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79E.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Use the existing CMake/GRFCodec producer; current native applications and packed
publication remain separate gates. Logs stay in logs, transient decodes in test-cache.
"""

import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

from ground_pack_check_support import ROOT, handoff
from collector_payload_checks import decoded_native_rows, verify_pixels

PROOF = ROOT / "logs/proofs/mlm/native-art/ground/ground-packing-v1/build-proof.json"
TRANSITIONS = {"CMakeLists.txt", "media/baseset/openttd/CMakeLists.txt", "media/baseset/openttd/openttd.nfo",
               "src/table/sprites.h", "media/baseset/openttd.grf", "media/baseset/openttd.grf.hash",
               "media/multilayer/contracts/multilayer_sprite_manifest.json"}


def identity(path):
    """Bind an existing repository artifact to its exact current bytes.

    @param path Absolute repository-owned input/output.
    @return Repository-relative path and SHA256 identity.
    """
    return {"path": path.relative_to(ROOT).as_posix(), "sha256": handoff.digest(path)}


def preserved(before):
    """Check every immutable predecessor file and retain exact identities for bounded transitions.

    @param before Original2145-file/source/native/application snapshot from79D.1.
    @return Only changed files at the seven explicitly owned integration/publication paths.
    """
    changed = []
    # The named integration paths are the complete transition boundary. Other
    # old headers, originals, Space/launch files and applications remain exact.
    for row in before["files"]:
        actual = identity(handoff.bound(row["path"]))
        if actual != row:
            if row["path"] not in TRANSITIONS:
                raise ValueError("ground payload changed an unrelated predecessor file: " + row["path"])
            changed.append({"before": row, "after": actual})
    predecessor = before["predecessor_payload"]
    if identity(handoff.bound(predecessor["path"])) != predecessor:
        raise ValueError("ground retained predecessor payload changed")
    return changed


def execute(command, directory, log):
    """Run one encoder/decoder command with exact arguments and actual exit status.

    @param command Explicit process argument vector, without shell interpolation.
    @param directory Existing repository-owned working directory.
    @param log Repository log path for complete normal/error diagnostics.
    @return Actual command/result/log identity.
    @throws ValueError When the producer/decoder fails.
    """
    started = time.perf_counter()
    with log.open("wb") as stream:
        result = subprocess.run(command, cwd=directory, stdout=stream, stderr=subprocess.STDOUT, check=False)
    if result.returncode != 0:
        raise ValueError("ground payload command failed; inspect " + str(log))
    return {"command": [str(arg) for arg in command], "exit_code": result.returncode,
            "seconds": round(time.perf_counter() - started, 3), "log": identity(log)}


def decode(payload, destination, codec, log):
    """Extract actual Action5 geometry and indexed pixels through the existing external codec.

    @param payload Exact existing GRF, either retained predecessor or fresh complete build.
    @param destination New repository test-cache directory for decoded NFO/PNGs.
    @param codec Actual installed GRFCodec executable.
    @param log Repository log path for this exact decoding process.
    @return Actual native-offset declarations and executed decoder evidence.
    """
    destination.mkdir(parents=True)
    # GRFCodec may otherwise spell the new count/offset byte sequence as a
    # quoted string. Explicit hex-only metadata keeps the established Action5
    # parser authoritative without broadening it to ambiguous text escapes.
    result = execute([codec, "-d", "-xx", "-t", "-o", "png", "-p", "1", str(payload), str(destination)], ROOT, log)
    rows = decoded_native_rows(destination / (payload.stem + ".nfo"))
    if not rows:
        raise ValueError("ground actual codec decode contains no native sprites")
    return rows, result


def main():
    """Build two fresh complete encodings and compare all prior/new decoded native cells.

    @return None; write exact source/build/decoder evidence without packed publication.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", required=True, help="Repository logs run directory")
    args = parser.parse_args()
    output = handoff.bound(args.run)
    if not output.is_relative_to(ROOT / "logs"):
        raise ValueError("ground build diagnostics belong in repository logs")
    output.mkdir(parents=True, exist_ok=True)
    scope, before, _public = handoff.read_scope()
    changes = preserved(before)
    qualification = handoff.read_json(scope["qualification"])
    # A historical passed flag cannot qualify altered converter, checks, scope
    # or fixture code. Every qualified input remains bound to actual bytes.
    if qualification.get("passed") is not True or qualification.get("focused_cases") != 14:
        raise ValueError("ground focused packing qualification is absent")
    for row in qualification["inputs"] + qualification["outputs"]:
        if identity(handoff.bound(row["path"])) != row:
            raise ValueError("ground qualified input/output changed: " + row["path"])
    report = handoff.read_json(scope["report"])
    cache = dict(re.findall(r"^([^#/:=]+):[^=]+=(.*)$", (ROOT / "build/CMakeCache.txt").read_text(encoding="utf-8"), re.MULTILINE))
    tools = {name: cache[name] for name in ("CMAKE_COMMAND", "GRFCODEC_EXECUTABLE", "NFORENUM_EXECUTABLE", "GRFID_EXECUTABLE")}
    source = ROOT / "media/baseset/openttd"
    cmake = (source / "CMakeLists.txt").read_text(encoding="utf-8")
    pngs = re.findall(r"\$\{CMAKE_CURRENT_SOURCE_DIR\}/([^\s)]+\.png)", cmake.split("PNG_SOURCE_FILES", 1)[1])
    if len(pngs) != len(set(pngs)) or "ground_final.png" not in pngs:
        raise ValueError("ground native source PNG dependencies are malformed")
    run = ROOT / "test-cache/ground-payload" / ("run-" + str(time.time_ns()))
    run.mkdir(parents=True)
    old, old_command = decode(handoff.bound(before["predecessor_payload"]["path"]), run / "predecessor-decoded",
                              tools["GRFCODEC_EXECUTABLE"], output / "predecessor-decode.log")
    required_old = set(range(66)) | set(range(3588, 3599))
    for row in before["native_prefix"]:
        required_old.update(range(row["packed"]["native_offset"], row["packed"]["native_offset"] + row["packed"]["sprite_count"]))
    if set(old) != required_old or len(old) != 7764:
        raise ValueError("ground predecessor decode differs from the complete published prefix")
    expected = dict(old)
    for row in report["sprites"]:
        expected[row["native_offset"]] = {"path": handoff.bound(scope["png"]), "box": row["box"], "anchor": row["anchor"]}
    if len(expected) != 22018 or set(expected) != required_old | set(range(7791, 22045)):
        raise ValueError("ground complete expected native domain changed")
    builds = []
    # Both fresh directories receive all current native sheets, then the exact
    # existing CMake producer performs include expansion, linting and encoding.
    # Decode both complete outputs; equal hashes alone cannot prove source pixels.
    for attempt in (1, 2):
        directory = run / str(attempt)
        sprites = directory / "sprites"
        sprites.mkdir(parents=True)
        for name in pngs:
            shutil.copyfile(source / name, sprites / name)
        target = directory / "verified-openttd.grf"
        command = [tools["CMAKE_COMMAND"], "-DGRF_SOURCE_FOLDER=" + str(source), "-DGRF_BINARY_FILE=" + str(target),
                   "-DNFORENUM_EXECUTABLE=" + tools["NFORENUM_EXECUTABLE"], "-DGRFCODEC_EXECUTABLE=" + tools["GRFCODEC_EXECUTABLE"],
                   "-DGRFID_EXECUTABLE=" + tools["GRFID_EXECUTABLE"], "-P", str(ROOT / "cmake/scripts/CreateGRF.cmake")]
        encoded = execute(command, directory, output / ("clean-build-" + str(attempt) + ".log"))
        decoded, command = decode(target, directory / "decoded", tools["GRFCODEC_EXECUTABLE"], output / ("decode-" + str(attempt) + ".log"))
        count, images = verify_pixels(expected, decoded)
        checksum = Path(str(target) + ".hash").read_text(encoding="utf-8").strip()
        if not re.fullmatch(r"[0-9a-f]{32}", checksum):
            raise ValueError("ground GRFID output is not the format-specific checksum")
        builds.append({"payload": identity(target), "grfid_md5": checksum, "build": encoded, "decode": command,
                       "decoded_sprites": count, "decoded_images": [identity(path) for path in images]})
    if builds[0]["payload"]["sha256"] != builds[1]["payload"]["sha256"] or builds[0]["grfid_md5"] != builds[1]["grfid_md5"]:
        raise ValueError("ground two complete clean encodings differ")
    preserved(before)
    inputs = [source / name for name in pngs]
    inputs += list(source.glob("*.nfo"))
    inputs += [Path(__file__), ROOT / "src/tests/multilayer/collector_payload_checks.py", ROOT / "cmake/scripts/CreateGRF.cmake",
               source / "CMakeLists.txt", ROOT / "src/table/sprites.h", handoff.SCOPE,
               handoff.bound(scope["report"]), handoff.bound(scope["qualification"])]
    proof = {"_documentation": __doc__, "schema_version": 1, "passed": True, "builds": builds,
             "inputs": [identity(path) for path in inputs], "predecessor": before["predecessor_payload"],
             "predecessor_decode": old_command, "protected_sprites": len(old), "new_sprites": report["pose_count"],
             "total_sprites": len(expected), "native_bound": 22045, "full_pixel_bytes_exact": True, "anchors_exact": True,
             "transitions": changes, "native_loading_verified": False, "runtime_activated": False}
    PROOF.parent.mkdir(parents=True, exist_ok=True)
    PROOF.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: proof[key] for key in ("passed", "protected_sprites", "new_sprites", "total_sprites", "full_pixel_bytes_exact")}))


if __name__ == "__main__":
    main()
