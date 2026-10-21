"""Retain the observed native station offset defect before correcting its publication epoch.

@file ground_station_anchor_diagnostic.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHCCB.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This diagnostic is deliberately a failing source-placement result. Native
construction/assertions alone do not qualify the displaced original sprites.
"""

import json

from ground_pack_check_support import ROOT, handoff
from tools.ground_payload_binding import checked_reference, reference

DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-consumers-v1/station-offset-diagnostic"
EXECUTION = "logs/graphics/selected-ground-runtime/20261021-station-01/modules-debug/modules.json"


def retain(path, name):
    """Archive exact existing executed/source/publication bytes without replacing a prior identity.

    @param path Existing repository file whose historical bytes must survive repair.
    @param name Explicit archive-relative filename under this defect receipt.
    @return Exact retained repository reference.
    """
    target = DIRECTORY / name
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists() and target.read_bytes() != path.read_bytes():
        raise ValueError("station defect archive already contains another identity: " + name)
    if not target.exists():
        target.write_bytes(path.read_bytes())
    return reference(target)


def diagnose():
    """Bind actual displaced native captures to accepted native offsets and the old packed/source epoch.

    @return Explicit defect evidence with native source-placement acceptance false.
    """
    execution = handoff.read_json(EXECUTION)
    observed = execution["observation"]
    # The defect must belong to an actual complete loaded scene, not a guessed
    # source contact sheet or an unsuccessful construction/process run.
    if execution.get("passed") is not True or observed["assertions"] != 2580 or observed["native_scenes"] != 210:
        raise ValueError("station offset diagnostic lacks actual native capture scope")
    for row in execution["inputs"] + observed["artifacts"] + [observed[key] for key in ("application", "source", "terminal", "runtime_payload")]:
        checked_reference(row)
    scope, before, public = handoff.read_scope()
    packed = handoff.read_json("logs/proofs/mlm/native-art/ground/packed-ground-manifest.json")
    families = {row["asset_id"]: row for row in public["assets"]}
    entries = {(row["asset_id"], row["state"], row["frame"], row["direction"], row["tile_variant"], row["part"]): row for row in packed["sprites"]}
    mismatches, source_inputs = [], []
    # Diagnose only the two conventional facility producers. All raw sources,
    # typed tuple ranges and native parent geometry remain unchanged.
    for asset in ("facility.rail-station", "facility.road-terminal"):
        family = families[asset]
        poses, atlas = handoff.source_poses(family, scope, before, {}, {})
        source_inputs.extend(reference(handoff.bound(family["authoring"][key])) for key in ("native_atlas", "atlas_manifest"))
        for key, pose in poses.items():
            if pose["structurally_absent"]:
                continue
            left, top, width, height = pose["box"]
            crop = atlas.crop((left, top, left + width, top + height)).getbbox()
            expected = [pose["native_packed_anchor"][0] + crop[0], pose["native_packed_anchor"][1] + crop[1]]
            entry = entries[(asset, *key)]
            if entry["anchor"] == expected:
                continue
            mismatches.append({"asset_id": asset, "source_key": list(key), "native_offset": entry["native_offset"],
                               "source_canvas_anchor": pose["anchor"], "native_uncropped_anchor": pose["native_packed_anchor"],
                               "current_transparent_crop": list(crop), "published_anchor": entry["anchor"],
                               "required_individual_anchor_before_relative_parent_crop_handling": expected})
    if not mismatches:
        raise ValueError("the historical station offset defect is no longer reproducible; do not rewrite its epoch")
    archive = {
        "application": retain(checked_reference(observed["application"]), "Debug/openttd_ground_viewport_test.exe"),
        "payload": retain(ROOT / "media/baseset/openttd.grf", "publication/openttd.grf"),
        "public": retain(handoff.PUBLIC, "publication/multilayer_sprite_manifest.json"),
        "packed_manifest": retain(ROOT / "logs/proofs/mlm/native-art/ground/packed-ground-manifest.json", "publication/packed-ground-manifest.json"),
        "payload_receipt": retain(ROOT / "logs/proofs/mlm/native-art/ground/packed-ground-payload.json", "publication/packed-ground-payload.json"),
        "sheet": retain(ROOT / "media/baseset/openttd/ground_final.png", "publication/ground_final.png"),
        "nfo": retain(ROOT / "media/baseset/openttd/ground_final.nfo", "publication/ground_final.nfo"),
        "observer_source": retain(ROOT / "src/tests/ground_station_runtime.cpp", "source/ground_station_runtime.cpp"),
        "runner_source": retain(ROOT / "src/tests/multilayer/ground_station_runtime_checks.py", "source/ground_station_runtime_checks.py"),
    }
    result = {"_documentation": __doc__, "defect_confirmed": True, "native_source_placement_passed": False,
              "execution": reference(handoff.bound(EXECUTION)), "native_assertions": 2580, "actual_native_views": 210,
              "mismatched_individual_packed_offsets": len(mismatches), "mismatches": mismatches,
              "relative_glass_parent_crop_reference_also_requires_preservation": True,
              "source_inputs": source_inputs, "archive": archive,
              "inputs": [reference(ROOT / path) for path in ("src/tests/multilayer/ground_station_anchor_diagnostic.py",
                         "media/multilayer/pack_ground_art.py", "media/multilayer/tools/ground_station_contract.py",
                         "src/viewport.cpp", "src/sprite.cpp")]}
    (DIRECTORY / "diagnostic.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


if __name__ == "__main__":
    result = diagnose()
    print(json.dumps({key: result[key] for key in ("defect_confirmed", "native_source_placement_passed", "mismatched_individual_packed_offsets")}))
