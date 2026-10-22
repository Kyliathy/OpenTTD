"""Execute current Ground source consumers with explicit current decoder/build inputs.

@file run_ground_consumer_checks.py Original isolated native owner; no historical EXE cache.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
"""
import json
from pathlib import Path
import struct

from ground_pack_check_support import ROOT, handoff
from tools.ground_payload_binding import reference

# Historical callers are migrated/guarded separately; the current entry point
# never chooses a before snapshot or reads this old diagnostic location.
BASELINE = "logs/graphics/selected-ground-consumers/20261021-types-01/before.json"
POSE_SHA256 = "1a07fe173fa21124c603586c3695df2405151c6d66263c90e4cab7e5c594ff89"


def preserved(relative):
    """Require exact current source/payload/application identities outside named owner changes.

    @param relative Explicit repository-relative before snapshot for the active bounded group.
    @return Actual narrowly named existing-file transitions and preservation count.
    """
    before = handoff.read_json(relative)
    changed = []
    # Every old app, retained packing image and source artifact stays exact.
    # Only explicit executing-owner paths may differ; wildcard exemptions are
    # never inferred from a common directory or an old historical baseline.
    for row in before["files"]:
        actual = reference(handoff.bound(row["path"]))
        if actual != row:
            if row["path"] not in before["planned_existing_changes"]:
                raise ValueError("ground consumer changed an unrelated protected file: " + row["path"])
            changed.append({"before": row, "after": actual})
    return changed, len(before["files"])


def verify_consumer_inputs(request, root):
    """Close the independently retained tuple domain and unchanged current/later dispatch boundary.

    @param request Registered Ground selector request with exact retained fixture/dispatch roles.
    @param root Explicit checkout owning immutable source expectations and current native processes.
    @return Exact original source and selected consumer scope, without a viewport/gameplay claim.
    """
    from artifact_rebuild import identity
    from artifact_paths import contained
    inputs = request["inputs"]
    contract = json.loads((root / inputs["consumer-expectations"]["path"]).read_text(encoding="utf-8"))
    expected = {key: contract["fixture"][key] for key in ("path", "sha256")}
    fixture = contained(root / inputs["pose-fixture"]["path"], root / "src/tests/fixtures/graphics")
    # Current native resolver output cannot supply its own expectations. These
    # exact original source-owned bytes predate the cleanup reader modification.
    if (identity(fixture, root) != expected or inputs["pose-fixture"] != expected
            or expected["sha256"] != POSE_SHA256 or fixture.stat().st_size != 128301):
        raise ValueError("Ground consumer requires its independently retained original tuple bytes")
    data = fixture.read_bytes()
    if data[:8] != b"GRDPOS1\0" or struct.unpack_from("<HHHB", data, 8) != (14254, 7791, 22045, 47):
        raise ValueError("Ground consumer independent source domain changed")
    rows = list(struct.iter_unpack("<BBBBHBH", data[15:]))
    # Retain every logical source axis and native ID, including aliases. A
    # duplicate tuple or offset cannot hide an omitted source pose/family.
    if (len(rows) != 14254 or len({row[:-1] for row in rows}) != 14254
            or {row[-1] for row in rows} != set(range(7791, 22045))):
        raise ValueError("Ground consumer independent tuple coverage is incomplete")
    dispatch_path = root / inputs["dispatch"]["path"]
    if identity(dispatch_path, root) != inputs["dispatch"]:
        raise ValueError("Ground consumer dispatch contract changed")
    dispatch = json.loads(dispatch_path.read_text(encoding="utf-8"))
    counts = {name: sum(row["dispatch"] == name for row in dispatch["families"]) for name in contract["dispatch_counts"]}
    # Source completeness cannot promote twenty-six explicitly deferred owners
    # into current drawing, operations, useful-route or human acceptance.
    if counts != contract["dispatch_counts"] or len(dispatch["families"]) != 50:
        raise ValueError("Ground consumer current/later ownership partition changed")
    for row in dispatch["families"]:
        if row["dispatch"] == "later_owner":
            if not all(row.get(key) for key in ("group", "master", "reason")):
                raise ValueError("Ground deferred owner lost its explicit unfinished dispatch")
    return dict(source_families=47, source_poses=14254, native_start=7791, native_end_exclusive=22045,
                dispatch_families=50, dispatch_counts=counts, viewport_claimed=False,
                later_operating_claimed=False, whole_underground_complete=False)


def main():
    """Dispatch only the original current Ground selector cases with explicit actual prerequisites."""
    from space_source_review import main as selected_main
    selected_main("ground")


if __name__ == "__main__":
    main()
