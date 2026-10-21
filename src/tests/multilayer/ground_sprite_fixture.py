"""Emit independent canonical source tuples for the actual typed ground sprite API.

@file ground_sprite_fixture.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Read the already qualified source report; never recompute native offsets with
the resolver formula under test. Rebuild output in logs/proofs/mlm/native-art/expected.
"""

import json
import struct

from ground_pack_check_support import ROOT, handoff
from tools.ground_payload_binding import checked_reference, reference

FIXTURE = ROOT / "logs/proofs/mlm/native-art/expected/ground-pose-tuples.bin"
SUMMARY = ROOT / "logs/proofs/mlm/native-art/ground/ground-consumers-v1/pose-expectations.json"


def produce():
    """Serialize all14,254 source-owned canonical tuples and exact qualified native IDs.

    @return Actual fixture/source/API identities; native signature and behavior still require compilation.
    """
    scope, _before, _public = handoff.read_scope()
    packed = handoff.read_json("logs/proofs/mlm/native-art/ground/packed-ground-payload.json")
    checked_reference(packed["source_manifest"])
    source = handoff.read_json(scope["report"])
    families = {row["asset_id"]: (index, row) for index, row in enumerate(source["families"])}
    chunks = [b"GRDPOS1\0", struct.pack("<HHHB", 14254, 7791, 22045, 47)]
    tuples, offsets = set(), set()
    # The accepted converter's explicit source tuple and native_offset supply
    # independent expected answers. Reusing resolver arithmetic here would
    # allow the same axis/order bug to pass in both producer and native test.
    for row in source["sprites"]:
        index, family = families[row["asset_id"]]
        pose = (index, family["states"].index(row["state"]), row["frame"], family["directions"].index(row["direction"]),
                row["tile_variant"], family["parts"].index(row["part"]))
        if pose in tuples or row["native_offset"] in offsets:
            raise ValueError("ground independent tuple fixture repeats a source identity")
        tuples.add(pose)
        offsets.add(row["native_offset"])
        chunks.append(struct.pack("<BBBBHBH", *pose, row["native_offset"]))
    if len(tuples) != 14254 or offsets != set(range(7791, 22045)):
        raise ValueError("ground independent source tuple coverage changed")
    FIXTURE.parent.mkdir(parents=True, exist_ok=True)
    FIXTURE.write_bytes(b"".join(chunks))
    report = {"_documentation": __doc__, "fixture": reference(FIXTURE), "producer": reference(ROOT / "src/tests/multilayer/ground_sprite_fixture.py"),
              "source_manifest": packed["source_manifest"], "api_header": reference(ROOT / "src/ground_sprite.h"),
              "families": 47, "source_tuples": 14254, "native_start": 7791, "native_bound": 22045,
              "native_signature_verified": False, "runtime_activated": False}
    SUMMARY.parent.mkdir(parents=True, exist_ok=True)
    SUMMARY.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


if __name__ == "__main__":
    print(json.dumps({key: value for key, value in produce().items() if key in ("families", "source_tuples", "native_start", "native_bound")}))
