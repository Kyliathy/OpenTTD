"""Freeze the current source/packing epoch before a bounded native ground consumer group.

@file ground_consumer_baseline.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups79FB-H.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Carry predecessor inputs forward with actual current hashes and protect all
qualified packing/typed images. The current-consumer working binary may relink;
its completed source-bound test images live in the retained proof directories.
"""

import argparse
import json

from ground_pack_check_support import ROOT, handoff
from tools.ground_payload_binding import reference


def freeze(previous, destination, allowed):
    """Capture the actual current epoch with only explicit old-owner mutation paths.

    @param previous Existing earlier group's explicit before snapshot path.
    @param destination New repository logs snapshot path for the active group.
    @param allowed Concrete existing source/dispatch owner paths; no wildcard or directory exemption.
    @return Newly frozen source/payload/public/application file count.
    """
    target = handoff.bound(destination)
    if not target.is_relative_to(ROOT / "logs"):
        raise ValueError("ground consumer snapshots belong in repository logs")
    if target.exists():
        raise ValueError("ground consumer before snapshot already exists; use its exact executing epoch")
    before = handoff.read_json(previous)
    paths = {handoff.bound(row["path"]) for row in before["files"]}
    for relative in allowed:
        path = handoff.bound(relative)
        if not path.is_file():
            raise ValueError("ground allowed owner must be a concrete existing file: " + relative)
        paths.add(path)
    patterns = ["src/ground*.h", "src/ground*.cpp", "src/tests/ground*.cpp", "src/tests/multilayer/*ground*.py",
                "media/multilayer/contracts/ground*.json", "logs/proofs/mlm/native-art/ground/ground-consumers-v1/**/*.json",
                "logs/proofs/mlm/native-art/ground/ground-consumers-v1/**/*.exe",
                "logs/proofs/mlm/native-art/expected/ground-*.bin"]
    # Extend the existing complete source/payload/application set with each
    # completed consumer handoff and its tested image. Later working builds
    # cannot silently replace those historical executed-code identities.
    for pattern in patterns:
        paths.update(path for path in ROOT.glob(pattern) if path.is_file())
    record = {"_documentation": __doc__, "schema_version": 1, "predecessor": reference(handoff.bound(previous)),
              "files": [reference(path) for path in sorted(paths)], "planned_existing_changes": list(allowed),
              "public": handoff.read_json(handoff.PUBLIC.relative_to(ROOT).as_posix()), "native_sprite_bound": 22045}
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    return len(paths)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--previous", required=True, help="Earlier explicit repository logs before snapshot")
    parser.add_argument("--output", required=True, help="New explicit repository logs before snapshot")
    parser.add_argument("--allow", action="append", required=True, help="One concrete executing existing owner path")
    args = parser.parse_args()
    print(json.dumps({"frozen_files": freeze(args.previous, args.output, args.allow)}))
