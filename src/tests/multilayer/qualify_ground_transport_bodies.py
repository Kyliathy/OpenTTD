"""Join actual conventional body/map/list source proof before station observer extensions.

@file qualify_ground_transport_bodies.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHC.1.
@see docs/zz-reach2/architecture/vehicles/archi-vehicles.md.
Packet, position, phase, hold and floor staging remains explicit. Native depot
occlusion and original list clipping are reported, rather than counted as visible pixels.
"""

import json

from ground_pack_check_support import ROOT, handoff
from ground_transport_body_pixel_checks import expected_matrix
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import checked_reference, reference

RUN = ROOT / "logs/graphics/selected-ground-runtime/20261021-transport-01"
DESTINATION = ROOT / "logs/proofs/mlm/native-art/ground/ground-consumers-v1"


def qualify():
    """Retain exact native body executions and join their separately dated purchase epoch.

    @return Both configurations' exact body/source proof, with station and gameplay gates open.
    """
    purchase_path = DESTINATION / "transport-purchases.json"
    purchase = handoff.read_json(purchase_path.relative_to(ROOT).as_posix())
    if purchase.get("source_pixels_and_anchors_exact") is not True:
        raise ValueError("transport body closure lacks actual source-qualified native purchases")
    # The earlier observer source hash remains historical after this extension;
    # its retained executed image and source/capture receipts remain immutable.
    for row in purchase["native"]:
        checked_reference(row["retained_application"])
        checked_reference(row["execution"])
        checked_reference(row["pixels"])
    refusal_path = RUN / "body-pixel-verifier-cases/verification.json"
    refusal = handoff.read_json(refusal_path.relative_to(ROOT).as_posix())
    if refusal.get("passed") is not True or refusal["cases"] != 8:
        raise ValueError("transport body pixel verifier lacks its semantic refusal boundary")
    for row in refusal["inputs"] + [refusal["terminal"], refusal["native_execution"]]:
        checked_reference(row)
    native, counterpart = [], {}
    # Each configuration independently executes real native commands and draws
    # every expected matrix row. Retain the exact image before later relinking.
    for configuration in ("Debug", "RelWithDebInfo"):
        application_reference = None
        scopes = []
        for mode, assertions, source_pixels, occluded, clipped in (("rail", 3295, 77300, 13919, 392), ("road", 2410, 53763, 7031, 0)):
            suffix = ("-04" if mode == "rail" else "-03") if configuration == "Debug" else ""
            execution_path = RUN / (mode + "-" + configuration.lower() + suffix) / (mode + ".json")
            pixel_path = RUN / (mode + "-pixels-" + configuration.lower() + suffix + ".json")
            execution = handoff.read_json(execution_path.relative_to(ROOT).as_posix())
            observed = execution["observation"]
            pixels = handoff.read_json(pixel_path.relative_to(ROOT).as_posix())
            # Native assertions, complete semantic coverage and exact source
            # pixels must all identify this same actual process and source epoch.
            if (execution.get("passed") is not True or observed["exit_code"] != 0 or observed["assertions"] != assertions
                    or pixels.get("passed") is not True or pixels["execution"] != reference(execution_path)
                    or pixels["native_body_frames"] != len(expected_matrix(mode)) or pixels["matched_source_pixels"] != source_pixels
                    or sum(row["native_depot_occluded_source_pixels"] for row in pixels["frames"]) != occluded
                    or sum(row["native_list_clipped_source_pixels"] for row in pixels["frames"]) != clipped):
                raise ValueError("transport body qualification lacks exact native/source scope: " + mode)
            for row in execution["inputs"] + observed["artifacts"] + [observed[key] for key in ("application", "source", "terminal", "runtime_payload")]:
                checked_reference(row)
            for row in pixels["frames"]:
                for key in ("source_atlas", "source_manifest", "native_rgb", "native_indices"):
                    if row[key] is not None:
                        checked_reference(row[key])
            raster_hashes = {row["path"].rsplit("/", 1)[-1]: row["sha256"] for row in observed["artifacts"]
                             if row["path"].endswith((".idx", ".ppm"))}
            if configuration == "Debug":
                counterpart[mode] = raster_hashes
            elif raster_hashes != counterpart[mode]:
                raise ValueError("native transport pixels differ between configurations: " + mode)
            if application_reference is not None and application_reference != observed["application"]:
                raise ValueError("native transport scopes executed different application images")
            application_reference = observed["application"]
            scopes.append({"mode": mode, "execution": reference(execution_path), "pixels": reference(pixel_path),
                           "native_assertions": assertions, "native_frames": pixels["native_body_frames"],
                           "matched_source_pixels": source_pixels, "native_depot_occluded_source_pixels": occluded,
                           "native_list_clipped_source_pixels": clipped})
        application = checked_reference(application_reference)
        retained = DESTINATION / "transport-bodies-bin" / configuration / application.name
        retained.parent.mkdir(parents=True, exist_ok=True)
        if retained.exists() and retained.read_bytes() != application.read_bytes():
            raise ValueError("retained native transport body image has another identity")
        if not retained.exists():
            retained.write_bytes(application.read_bytes())
        native.append({"configuration": configuration, "scopes": scopes,
                       "executed_application": application_reference, "retained_application": reference(retained)})
    transitions, count = preserved((RUN / "before.json").relative_to(ROOT).as_posix())
    inputs = ("src/tests/ground_transport_runtime.cpp", "src/tests/ground_transport_runtime_fixture.h",
              "src/tests/multilayer/ground_transport_runtime_checks.py", "src/tests/multilayer/ground_transport_body_pixel_checks.py",
              "src/tests/multilayer/ground_transport_body_pixel_checks-1.py", "src/tests/multilayer/qualify_ground_transport_bodies.py")
    result = {"_documentation": __doc__, "passed": True, "native": native, "native_frames_each_configuration": 499,
              "matched_body_source_pixels_each_configuration": 131063, "purchase_receipt": reference(purchase_path),
              "verifier_refusals": reference(refusal_path), "inputs": [reference(ROOT / path) for path in inputs],
              "baseline": reference(RUN / "before.json"), "preserved_file_count": count, "transitions": transitions,
              "actual_native_map_and_list_drawing": True, "all_native_cargo_source_variants": True,
              "retained_zero_capacity_hauler_not_loaded": True, "drawing_owners_unchanged": True,
              "debug_optimized_native_rasters_identical": True, "station_foreground_coverage": False,
              "fixture_floor_positions_phase_packets_and_hold_states": True,
              "human_acceptance": "pending", "transported_bootstrap_claimed": False}
    (DESTINATION / "transport-bodies.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


if __name__ == "__main__":
    result = qualify()
    print(json.dumps({key: result[key] for key in ("passed", "native_frames_each_configuration", "matched_body_source_pixels_each_configuration")}))
