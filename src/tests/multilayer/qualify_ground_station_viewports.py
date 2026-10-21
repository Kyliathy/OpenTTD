"""Qualify actual corrected default station/terminal source and vehicle foreground viewports.

@file qualify_ground_station_viewports.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHCC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native command construction, packet/position/loading/clock fixtures establish
presentation only. Transported lifecycle and personal acceptance remain separate.
"""

import json

from ground_pack_check_support import ROOT, handoff
from ground_station_pixel_checks import expected_matrix
from ground_station_runtime_checks import BASELINE, preserve_completed
from tools.ground_payload_binding import checked_reference, reference

RUN = ROOT / "logs/graphics/selected-ground-runtime/20261021-station-corrected-01"
DIRECTORY = ROOT / "logs/proofs/mlm/native-art/ground/ground-consumers-v1"


def qualify():
    """Join complete current native source, depth, glass and owner-purity evidence in both configurations.

    @return Retained exact executed-image/source proof, without closing later gameplay or human gates.
    """
    publication_path = ROOT / "logs/proofs/mlm/native-art/ground/ground-station-offsets-v1/publication.json"
    publication = handoff.read_json(publication_path.relative_to(ROOT).as_posix())
    if publication.get("native_loading_verified") is not True:
        raise ValueError("station viewports require the corrected source-loaded publication")
    for key in ("payload", "public", "packed_handoff", "retained_payload"):
        checked_reference(publication[key])
    refusal_path = RUN / "pixel-verifier-cases/verification.json"
    refusal = handoff.read_json(refusal_path.relative_to(ROOT).as_posix())
    # Freshly rehashed bad pixels, wrong glass/depth, detached/omitted parts,
    # phase, truncation and failed/partial processes must all refuse semantically.
    if refusal.get("passed") is not True or refusal["cases"] != 10:
        raise ValueError("station source verifier lacks its semantic corruption boundary")
    for row in refusal["inputs"] + [refusal["terminal"], refusal["native_execution"]]:
        checked_reference(row)
    native, first_rasters = [], None
    for configuration in ("Debug", "RelWithDebInfo"):
        suffix = "-02" if configuration == "Debug" else ""
        execution_path = RUN / ("modules-" + configuration.lower() + suffix) / "modules.json"
        pixel_path = RUN / ("pixels-" + configuration.lower() + ".json")
        execution = handoff.read_json(execution_path.relative_to(ROOT).as_posix())
        observed = execution["observation"]
        pixels = handoff.read_json(pixel_path.relative_to(ROOT).as_posix())
        # The complete source/appearance matrix and positive real overlap
        # controls must identify this exact successful current native process.
        if (execution.get("passed") is not True or observed["exit_code"] != 0 or observed["assertions"] != 2580
                or observed["native_scenes"] != 210 or observed["runtime_payload"]["sha256"] != publication["payload"]["sha256"]
                or pixels.get("passed") is not True or pixels["execution"] != reference(execution_path)
                or pixels["native_scenes"] != 210 or pixels["matched_source_composition_pixels"] != 197064
                or pixels["glass_vehicle_pixels"] != 1240 or pixels["foreground_vehicle_pixels"] != 4272
                or pixels["reversed_depth_different_pixels"] != 11759):
            raise ValueError("station source/native foreground qualification is incomplete")
        if {(row["asset_id"], row["graphics"], row["state"], row["frame"], row["appearance"]) for row in pixels["frames"]} != expected_matrix():
            raise ValueError("station source composition lacks exact native scene coverage")
        for row in execution["inputs"] + observed["artifacts"] + [observed[key] for key in ("application", "source", "terminal", "runtime_payload")]:
            checked_reference(row)
        for row in pixels["frames"]:
            checked_reference(row["native_indices"])
            checked_reference(row["source_atlas"])
            if row["ambiguous_depth_pixels"] != 0:
                raise ValueError("station source proof guessed an ambiguous physical foreground order")
        rasters = {row["path"].rsplit("/", 1)[-1]: row["sha256"] for row in observed["artifacts"] if row["path"].endswith((".idx", ".ppm"))}
        if first_rasters is None:
            first_rasters = rasters
        elif rasters != first_rasters:
            raise ValueError("actual station RGB/index pixels differ between configurations")
        application = checked_reference(observed["application"])
        retained = DIRECTORY / "station-viewports-bin" / configuration / application.name
        retained.parent.mkdir(parents=True, exist_ok=True)
        if retained.exists() and retained.read_bytes() != application.read_bytes():
            raise ValueError("retained station viewport image has another executed identity")
        if not retained.exists():
            retained.write_bytes(application.read_bytes())
        native.append({"configuration": configuration, "execution": reference(execution_path), "pixels": reference(pixel_path),
                       "executed_application": observed["application"], "retained_application": reference(retained),
                       "native_assertions": 2580, "native_viewports": 210, "matched_source_composition_pixels": 197064,
                       "glass_vehicle_pixels": 1240, "foreground_vehicle_pixels": 4272,
                       "wrong_depth_different_pixels": 11759})
    transitions, count = preserve_completed()
    public = handoff.read_json(handoff.PUBLIC.relative_to(ROOT).as_posix())
    sources = []
    # The four actual source families own station/terminal masks and the two
    # command-created carriers. No offline substitute scene or sprite is accepted.
    for asset in ("facility.rail-station", "facility.road-terminal", "vehicle.model-05", "vehicle.model-07"):
        family = next(row for row in public["assets"] if row["asset_id"] == asset)
        sources.append({"asset_id": asset, "atlas": reference(handoff.bound(family["authoring"]["native_atlas"])),
                        "manifest": reference(handoff.bound(family["authoring"]["atlas_manifest"]))})
    inputs = ("src/tests/ground_station_runtime.cpp", "src/tests/multilayer/ground_station_runtime_checks.py",
              "src/tests/multilayer/ground_station_pixel_checks.py", "src/tests/multilayer/ground_station_pixel_checks-1.py",
              "src/tests/multilayer/qualify_ground_station_viewports.py")
    result = {"_documentation": __doc__, "passed": True, "native": native, "sources": sources,
              "corrected_publication": reference(publication_path), "verifier_refusals": reference(refusal_path),
              "inputs": [reference(ROOT / path) for path in inputs], "baseline": reference(handoff.bound(BASELINE)),
              "preserved_file_count": count, "transitions": transitions, "all_native_roles_entrances_and_axes": True,
              "source_pixels_and_anchors_exact": True, "independent_native_parent_child_vehicle_depth": True,
              "native_glass_destination_recolour_and_building_appearance": True, "absent_parts_preserved": True,
              "native_rgb_and_indices_identical_between_configurations": True, "drawing_owners_unchanged": True,
              "fixture_floor_native_neighbour_removal_packets_positions_loading_and_clock": True,
              "human_acceptance": "pending", "transported_bootstrap_claimed": False}
    (DIRECTORY / "station-viewports.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


if __name__ == "__main__":
    result = qualify()
    print(json.dumps({key: result[key] for key in ("passed", "source_pixels_and_anchors_exact", "independent_native_parent_child_vehicle_depth")}))
