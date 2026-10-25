"""Execute current Ground source consumers with explicit current decoder/build inputs.

@file run_ground_consumer_checks.py Original isolated native owner; no historical EXE cache.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group20.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
"""
import json
from pathlib import Path
import struct
import sys

from ground_pack_check_support import ROOT, handoff
from tools.ground_payload_binding import reference

sys.path.insert(0, str(ROOT / "src/tests"))

# Historical callers are migrated/guarded separately; the current entry point
# never chooses a before snapshot or reads this old diagnostic location.
BASELINE = "logs/graphics/selected-ground-consumers/20261021-types-01/before.json"
POSE_SHA256 = "1a07fe173fa21124c603586c3695df2405151c6d66263c90e4cab7e5c594ff89"
FAMILIES_SHA256 = "0ffed19f65958151caed5798ef561ec5dd36a4bc03d815a3c1bd66704472a302"


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
    """Close original tuple coverage and independently verify the implemented native ownership boundary.

    @param request Registered Ground selector request with exact retained fixture/dispatch roles.
    @param root Explicit checkout owning immutable source expectations and current native processes.
    @return Exact original source and selected consumer scope, without a viewport/gameplay claim.
    """
    from artifact_rebuild import identity
    from artifact_paths import contained
    from merge_renderer_binding_checks import verify, verify_imported
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
    family_path = contained(root / inputs["family-ownership"]["path"], root / "src/tests/fixtures/graphics")
    # Group8 froze source identities before native integration. Neither changed
    # dispatch metadata nor its truthful new checksum may invent another family.
    if identity(family_path, root) != inputs["family-ownership"] or inputs["family-ownership"]["sha256"] != FAMILIES_SHA256:
        raise ValueError("Ground consumer requires independently frozen family ownership")
    originals = {row["asset_id"]: row for row in json.loads(family_path.read_text(encoding="utf-8"))["families"]}
    ground = {key for key, row in originals.items() if row["destination_packer"] == "ground"}
    conventional = {key for key in ground if originals[key]["destination_draw_contract"].get("binding_header", "").startswith("src/ground_")}
    existing = {"vehicle.model-04", "purchase.model-04", "facility.vent-collector"}
    current = {row["asset_id"]: row for row in dispatch["families"]}
    # Keep the original21/3/26 partition as historical fact. Group51's actual
    # callbacks, rather than a rewritten baseline, qualify the26new native owners.
    if (contract["dispatch_counts"] != dict(current_bound=21, current_existing=3, later_owner=26)
            or counts != dict(current_bound=47, current_existing=3, later_owner=0)
            or len(dispatch["families"]) != 50 or len(current) != 50
            or set(current) != ground | existing or len(ground) != 47 or len(conventional) != 21):
        raise ValueError("Ground consumer current/later ownership partition changed")
    bindings = []
    # Each original family must retain its own offset, pose extent and native
    # namespace. A count-preserving swap between Ground and M1 is still invalid.
    for asset, row in current.items():
        original = originals[asset]
        expected_dispatch = "current_existing" if asset in existing else "current_bound"
        expected_packer = "m1" if asset in existing else "ground"
        if (row["dispatch"] != expected_dispatch or original["destination_packer"] != expected_packer
                or row["native_offset"] != original["A_binding"]["native_offset"]
                or row["sprite_count"] != original["A_binding"]["sprite_count"]):
            raise ValueError("Ground consumer family ownership or original pose extent changed")
        if asset in conventional:
            if any(row.get(key) != value for key, value in original["destination_draw_contract"].items()):
                raise ValueError("Ground conventional drawing contract changed")
            bindings.append(verify(row, root))
        elif asset in ground:
            if not row.get("native_chain"):
                raise ValueError("Ground imported owner has no connected native callback chain")
            bindings.append(verify_imported(row, original, root))
    return dict(source_families=47, source_poses=14254, native_start=7791, native_end_exclusive=22045,
                dispatch_families=50, dispatch_counts=counts, historical_dispatch_counts=contract["dispatch_counts"],
                live_native_bindings=bindings, imported_native_families=len(ground - conventional), viewport_claimed=False,
                later_operating_claimed=False, whole_underground_complete=False)


def main():
    """Dispatch only the original current Ground selector cases with explicit actual prerequisites."""
    from space_source_review import main as selected_main
    selected_main("ground")


if __name__ == "__main__":
    main()
