"""Stage a distinct facility-offset correction from unchanged accepted source poses.

@file ground_station_offset_repair.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHCCB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This producer preserves the historical source/publication reports. Staging
corrected PNG/NFO inputs grants no decoded/native-loading publication claim.
"""

import argparse
import copy
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer"))

import pack_ground_art as ground
import pack_m1_art as packing
from tools import ground_art_handoff as handoff
from tools.ground_payload_binding import checked_reference, reference
from run_ground_consumer_checks import preserved

DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-station-offsets-v1"
BASELINE = "logs/graphics/selected-ground-runtime/20261021-station-offset-01/before.json"
CHECKS = "logs/graphics/selected-ground-runtime/20261021-station-offset-01/offset-source-checks.json"
DIAGNOSTIC = "logs/proofs/mlm/native-art/ground/ground-consumers-v1/station-offset-diagnostic/diagnostic.json"
FACILITIES = {"facility.rail-station", "facility.road-terminal"}


def derive():
    """Prepare corrected packing from current exact accepted sources and the immutable old allocation.

    @return Corrected report and deterministic generated inputs, without writing or publishing a GRF.
    """
    diagnostic = handoff.read_json(DIAGNOSTIC)
    checks = handoff.read_json(CHECKS)
    # Actual native geometry/source/crop refusal checks must qualify this exact
    # canonical packer before its corrected inputs can be staged.
    if checks.get("passed") is not True or checks["cases"] != 8:
        raise ValueError("facility offset packing lacks its focused source qualification")
    for row in checks["inputs"]:
        checked_reference(row)
    original_path = checked_reference(diagnostic["archive"]["packed_manifest"])
    original = handoff.read_json(original_path.relative_to(ROOT).as_posix())
    old = {(row["asset_id"], row["state"], row["frame"], row["direction"], row["tile_variant"], row["part"]): row for row in original["sprites"]}
    scope, before, public = handoff.read_scope()
    families = {row["asset_id"]: row for row in public["assets"]}
    entries, sprites, documents, images = [], [], {}, {}
    preserved_count, changed = 0, []
    # Reuse exact immutable tuple identities and allocations. Repacking sheet
    # coordinates may change storage placement, never an unaffected decoded
    # sprite's pixels, dimensions, world offset or company-colour indices.
    for selected in scope["source_rows"]:
        family = families[selected["asset_id"]]
        poses, image = handoff.source_poses(family, scope, before, documents, images)
        for index, key in enumerate(packing.required_keys(family)):
            previous = old[(family["asset_id"], *key)]
            if previous["native_offset"] != selected["native_offset"] + index:
                raise ValueError("facility correction attempted native tuple reallocation")
            sprite = ground.native_pose(family, key, poses, image)
            pixels, width, height, x, y = sprite
            pixel_sha = hashlib.sha256(bytes(pixels)).hexdigest()
            equal = (previous["pixel_sha256"], previous["box"][2:], previous["anchor"]) == (pixel_sha, [width, height], [x, y])
            if family["asset_id"] not in FACILITIES:
                if not equal:
                    raise ValueError("facility correction changed an unrelated decoded tuple: " + family["asset_id"])
                preserved_count += 1
            elif not equal:
                changed.append(previous["native_offset"])
            entry = copy.deepcopy(previous)
            entry.update(anchor=[x, y], pixel_sha256=pixel_sha)
            entries.append(entry)
            sprites.append(sprite)
    # Missing/duplicated tuples cannot be hidden by a new sheet or a successful
    # encoder. The complete47-family/14,254-pose append remains unchanged.
    if len(entries) != 14254 or len(old) != len(entries) or preserved_count != 13454 or not changed:
        raise ValueError("facility correction changed the selected source domain")
    sheet, positions = packing.shelf_sheet(sprites, scope["sheet_width"], scope["sprite_gap"])
    for entry, sprite, position in zip(entries, sprites, positions):
        entry["box"] = [*position, sprite[1], sprite[2]]
    png = ground.encoded_png(sheet)
    nfo = packing.format_nfo(entries, sprites, positions).replace(b"map_layers_m1.png", b"ground_final.png")
    nfo = b"// @see r2mlvs14-remaining-assets-and-runtime-integration.md Groups79D/79FHCCB; archi-map-layers.md.\n" + nfo
    header = ground.format_header(original["families"], original["native_start"], original["native_sprite_bound"])
    if header != handoff.bound(scope["header"]).read_bytes():
        raise ValueError("facility correction changed typed native family/range declarations")
    files = {handoff.bound(scope["png"]): png, handoff.bound(scope["nfo"]): nfo}
    report = copy.deepcopy(original)
    report.update(_documentation=__doc__, sprites=entries, sheet_size=list(sheet.size),
                  corrects=reference(original_path), diagnostic=reference(handoff.bound(DIAGNOSTIC)),
                  qualification=reference(handoff.bound(CHECKS)), corrected_families=sorted(FACILITIES),
                  changed_native_offsets=changed, unchanged_other_selected_tuples=preserved_count,
                  native_loading_verified=False, packed_grf=None, runtime_activated=False)
    report["outputs"] = [{"path": path.relative_to(ROOT).as_posix(), "sha256": hashlib.sha256(data).hexdigest()} for path, data in files.items()] + [reference(handoff.bound(scope["header"]))]
    report["inputs"] = [reference(ROOT / path) for path in ("media/multilayer/tools/ground_station_offset_repair.py", "media/multilayer/pack_ground_art.py", handoff.SCOPE.relative_to(ROOT).as_posix())]
    return report, files


def main():
    """Stage reproducible corrected source inputs while retaining all unrelated predecessor files.

    @return None; native decode/loading and publication remain separate explicit gates.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", action="store_true", required=True)
    parser.parse_args()
    preserved(BASELINE)
    report, files = derive()
    for path, data in files.items():
        path.write_bytes(data)
    DIRECTORY.mkdir(parents=True, exist_ok=True)
    (DIRECTORY / "packed-ground-manifest.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    transitions, count = preserved(BASELINE)
    staged = {"_documentation": __doc__, "passed": True, "source_staging_only": True,
              "report": reference(DIRECTORY / "packed-ground-manifest.json"), "transitions": transitions,
              "preserved_file_count": count, "changed_native_tuples": len(report["changed_native_offsets"]),
              "unchanged_other_selected_tuples": report["unchanged_other_selected_tuples"],
              "native_loading_verified": False, "published": False}
    (DIRECTORY / "staged.json").write_text(json.dumps(staged, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: staged[key] for key in ("passed", "source_staging_only", "changed_native_tuples", "unchanged_other_selected_tuples")}))


if __name__ == "__main__":
    main()
