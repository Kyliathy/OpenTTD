"""Independently encode/decode the corrected facility epoch without publishing it.

@file ground_station_offset_payload_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHCCB.3.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
All unaffected decoded sprites remain byte/geometry-identical. Corrected source
expectations use a new fixture, preserving every historical report and payload.
"""

import argparse
import json
import re
import shutil
import struct
import sys
import time

from PIL import Image

from ground_pack_check_support import ROOT, handoff
from ground_payload_build_checks import decode, execute
from collector_payload_checks import verify_pixels
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import checked_reference, reference

sys.path.insert(0, str(ROOT / "src/tests"))
from multilayer_packed_art_fixture import pixel_hash

DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-station-offsets-v1"
BASELINE = "logs/graphics/selected-ground-runtime/20261021-station-offset-01/before.json"


def produce_fixture(expected):
    """Write complete independently qualified source expectations to a distinct native fixture.

    @param expected Original decoded unaffected tuples plus all accepted corrected source tuples.
    @return Exact fixture reference; an existing different fixture is never overwritten.
    """
    images, chunks = {}, [b"GRDART1\0", struct.pack("<HHHH", 22018, 22045, 7791, 14254)]
    # Exact offset order and full palette buffers preserve transparent holes,
    # company plaques, absent parts and every unchanged historical native range.
    for offset, row in sorted(expected.items()):
        if row["path"] not in images:
            images[row["path"]] = Image.open(row["path"])
        x, y, width, height = row["box"]
        pixels = images[row["path"]].crop((x, y, x + width, y + height)).tobytes()
        chunks.extend((struct.pack("<HHHhhI", offset, width, height, *row["anchor"], pixel_hash(pixels)), pixels))
    path = ROOT / "logs/proofs/mlm/native-art/expected/ground-station-offset-sprites.bin"
    data = b"".join(chunks)
    if path.exists() and path.read_bytes() != data:
        raise ValueError("corrected native source fixture already has another identity")
    if not path.exists():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    return reference(path)


def main():
    """Build two clean containers and prove exact complete source and unaffected-prefix decoding.

    @return None; retain candidate/native fixture evidence without replacing the published GRF or manifest.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", required=True)
    args = parser.parse_args()
    output = handoff.bound(args.run)
    if not output.is_relative_to(ROOT / "logs"):
        raise ValueError("corrected payload diagnostics belong beneath repository logs")
    output.mkdir(parents=True, exist_ok=True)
    preserved(BASELINE)
    staged_path = DIRECTORY / "staged.json"
    staged = handoff.read_json(staged_path.relative_to(ROOT).as_posix())
    if staged.get("passed") is not True or staged["changed_native_tuples"] != 432:
        raise ValueError("corrected payload lacks exact focused source staging")
    report_path = checked_reference(staged["report"])
    report = handoff.read_json(report_path.relative_to(ROOT).as_posix())
    for row in report["inputs"] + report["outputs"]:
        checked_reference(row)
    diagnostic = handoff.read_json(report["diagnostic"]["path"])
    previous = checked_reference(diagnostic["archive"]["payload"])
    cache = dict(re.findall(r"^([^#/:=]+):[^=]+=(.*)$", (ROOT / "build/CMakeCache.txt").read_text(encoding="utf-8"), re.MULTILINE))
    tools = {name: cache[name] for name in ("CMAKE_COMMAND", "GRFCODEC_EXECUTABLE", "NFORENUM_EXECUTABLE", "GRFID_EXECUTABLE")}
    work = ROOT / "test-cache/ground-station-offset-payload" / ("run-" + str(time.time_ns()))
    work.mkdir(parents=True)
    old, old_command = decode(previous, work / "previous", tools["GRFCODEC_EXECUTABLE"], output / "previous-decode.log")
    if len(old) != 22018:
        raise ValueError("retained previous publication has an unexpected native domain")
    expected = dict(old)
    for row in report["sprites"]:
        expected[row["native_offset"]] = {"path": ROOT / "media/baseset/openttd/ground_final.png", "box": row["box"], "anchor": row["anchor"]}
    if set(expected) != set(old):
        raise ValueError("corrected payload changed native identities or gaps")
    changed = set(report["changed_native_offsets"])
    unchanged = {offset: row for offset, row in old.items() if offset not in changed}
    source = ROOT / "media/baseset/openttd"
    cmake = (source / "CMakeLists.txt").read_text(encoding="utf-8")
    pngs = re.findall(r"\$\{CMAKE_CURRENT_SOURCE_DIR\}/([^\s)]+\.png)", cmake.split("PNG_SOURCE_FILES", 1)[1])
    builds = []
    # Two actual clean native encodings prove reproducibility. Independent
    # GRFCodec decoding compares every complete source buffer and all21,586
    # unaffected tuple dimensions/anchors/palette bytes, including Space assets.
    for attempt in (1, 2):
        directory = work / str(attempt)
        sprites = directory / "sprites"
        sprites.mkdir(parents=True)
        for name in pngs:
            shutil.copyfile(source / name, sprites / name)
        target = directory / "corrected-openttd.grf"
        command = [tools["CMAKE_COMMAND"], "-DGRF_SOURCE_FOLDER=" + str(source), "-DGRF_BINARY_FILE=" + str(target),
                   "-DNFORENUM_EXECUTABLE=" + tools["NFORENUM_EXECUTABLE"], "-DGRFCODEC_EXECUTABLE=" + tools["GRFCODEC_EXECUTABLE"],
                   "-DGRFID_EXECUTABLE=" + tools["GRFID_EXECUTABLE"], "-P", str(ROOT / "cmake/scripts/CreateGRF.cmake")]
        encoded = execute(command, directory, output / ("encode-" + str(attempt) + ".log"))
        decoded, decoded_command = decode(target, directory / "decoded", tools["GRFCODEC_EXECUTABLE"], output / ("decode-" + str(attempt) + ".log"))
        count, images = verify_pixels(expected, decoded)
        preserved_count, _ = verify_pixels(unchanged, {offset: decoded[offset] for offset in unchanged})
        if count != 22018 or preserved_count != 21586:
            raise ValueError("corrected native payload source/prefix proof is incomplete")
        builds.append({"payload": reference(target), "hash_file": reference(target.with_name(target.name + ".hash")),
                       "encode": encoded, "decode": decoded_command, "decoded_images": [reference(path) for path in images],
                       "exact_source_sprites": count, "unchanged_native_sprites": preserved_count})
    if builds[0]["payload"]["sha256"] != builds[1]["payload"]["sha256"]:
        raise ValueError("clean corrected encodings differ")
    fixture = produce_fixture(expected)
    transitions, count = preserved(BASELINE)
    # Candidate verification must not expose a partially qualified publication
    # to ordinary games or reassign their canonical public payload hash.
    if reference(ROOT / "media/baseset/openttd.grf")["sha256"] != diagnostic["archive"]["payload"]["sha256"]:
        raise ValueError("candidate build unexpectedly replaced the published payload")
    proof = {"_documentation": __doc__, "passed": True, "builds": builds, "fixture": fixture,
             "staging": reference(staged_path), "report": reference(report_path), "previous_decode": old_command,
             "previous_payload": reference(previous), "total_native_sprites": 22018, "changed_native_sprites": 432,
             "unchanged_native_sprites": 21586, "exact_source_pixels_and_offsets": True,
             "native_loading_verified": False, "published": False, "preserved_file_count": count, "transitions": transitions,
             "inputs": [reference(ROOT / path) for path in ("src/tests/multilayer/ground_station_offset_payload_checks.py",
                        "media/multilayer/pack_ground_art.py", "cmake/scripts/CreateGRF.cmake", "media/baseset/openttd/CMakeLists.txt")]}
    (DIRECTORY / "build-proof.json").write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: proof[key] for key in ("passed", "total_native_sprites", "changed_native_sprites", "unchanged_native_sprites")}))


if __name__ == "__main__":
    main()
