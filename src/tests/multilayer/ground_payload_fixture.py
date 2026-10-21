"""Bind qualified predecessor/source pixels to full native underground loading expectations.

@file ground_payload_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79E.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This consumes the actual complete-byte79E.1 proof; it never regenerates old source
or replays stale historical producers. Output is reproducible in logs/proofs.
"""

import json
from pathlib import Path
import struct
import sys

from PIL import Image

from ground_pack_check_support import ROOT, handoff
from ground_payload_build_checks import PROOF, decoded_native_rows, identity, preserved

sys.path.insert(0, str(ROOT / "src/tests"))
from multilayer_packed_art_fixture import pixel_hash

FIXTURE = ROOT / "logs/proofs/mlm/native-art/expected/ground-packed-sprites.bin"
SUMMARY = PROOF.parent / "full-pixel-expectations.json"


def checked_build():
    """Require current exact input/output bytes for both clean complete encodings.

    @return Actual qualified build proof; no current payload/public file is changed.
    """
    proof = handoff.read_json(PROOF.relative_to(ROOT).as_posix())
    if (proof.get("passed") is not True or proof.get("protected_sprites") != 7764
            or proof.get("new_sprites") != 14254 or proof.get("total_sprites") != 22018
            or proof.get("full_pixel_bytes_exact") is not True or proof.get("anchors_exact") is not True
            or len(proof.get("builds", [])) != 2):
        raise ValueError("ground complete actual build/decode qualification is absent")
    # Native expectations may use only the proven exact inputs and outputs,
    # including independent decoder images; a stale success flag grants nothing.
    for reference in proof["inputs"] + [proof["predecessor"]]:
        if identity(handoff.bound(reference["path"])) != reference:
            raise ValueError("ground qualified complete payload input changed")
    for build in proof["builds"]:
        for reference in [build["payload"]] + build["decoded_images"]:
            if identity(handoff.bound(reference["path"])) != reference:
                raise ValueError("ground qualified encoding/decoded pixels changed")
    if proof["builds"][0]["payload"]["sha256"] != proof["builds"][1]["payload"]["sha256"]:
        raise ValueError("ground qualified complete builds disagree")
    return proof


def produce():
    """Write full source expectations and stage only the exactly qualified shared container.

    @return Source/native expectation summary, with actual application loading still pending.
    """
    proof = checked_build()
    scope, before, _public = handoff.read_scope()
    preserved(before)
    build = proof["builds"][0]
    decoded_directory = handoff.bound(build["decode"]["command"][-1])
    decoded_nfo = decoded_directory / (Path(build["payload"]["path"]).stem + ".nfo")
    rows = decoded_native_rows(decoded_nfo)
    report = handoff.read_json(scope["report"])
    # Prior expectations come from the complete-byte predecessor comparison,
    # while new expectations use the independent qualified source sheet.
    # Neither path reads pixels through the native cache being tested later.
    expected = {offset: row for offset, row in rows.items() if offset < 7791}
    for row in report["sprites"]:
        expected[row["native_offset"]] = {"path": handoff.bound(scope["png"]), "box": row["box"], "anchor": row["anchor"]}
    if len(expected) != 22018 or sum(offset < 7791 for offset in expected) != 7764:
        raise ValueError("ground native expectation domain changed")
    images, chunks = {}, [b"GRDART1\0", struct.pack("<HHHH", 22018, 22045, 7791, 14254)]
    # Preserve every source index and anchor in offset order, including all
    # transparent parts and static aliases that still own independent slots.
    for offset, row in sorted(expected.items()):
        path = row["path"]
        if path not in images:
            images[path] = Image.open(path)
        x, y, width, height = row["box"]
        pixels = images[path].crop((x, y, x + width, y + height)).tobytes()
        chunks.extend((struct.pack("<HHHhhI", offset, width, height, *row["anchor"], pixel_hash(pixels)), pixels))
    FIXTURE.parent.mkdir(parents=True, exist_ok=True)
    FIXTURE.write_bytes(b"".join(chunks))
    payload = ROOT / "media/baseset/openttd.grf"
    accepted = {before["predecessor_payload"]["sha256"], build["payload"]["sha256"]}
    if handoff.digest(payload) not in accepted:
        raise ValueError("ground shipped container changed outside the qualified transition")
    payload.write_bytes(handoff.bound(build["payload"]["path"]).read_bytes())
    Path(str(payload) + ".hash").write_bytes(Path(str(handoff.bound(build["payload"]["path"])) + ".hash").read_bytes())
    summary = {"_documentation": __doc__, "fixture": identity(FIXTURE), "producer": identity(Path(__file__)),
               "build_proof": identity(PROOF), "source_outputs": report["outputs"], "native_bound": 22045,
               "protected_sprites": 7764, "new_sprites": 14254, "total_sprites": 22018,
               "payload": identity(payload), "native_loading_verified": False, "runtime_activated": False}
    SUMMARY.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    preserved(before)
    return summary


if __name__ == "__main__":
    print(json.dumps({name: value for name, value in produce().items() if name.endswith("sprites") or name == "native_bound"}))
