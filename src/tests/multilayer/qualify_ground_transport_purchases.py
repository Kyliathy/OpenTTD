"""Retain the actually tested native purchase epoch before extending its observer.

@file qualify_ground_transport_purchases.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHC.
@see docs/zz-reach2/architecture/vehicles/archi-vehicles.md.
This partial receipt closes no map, station or gameplay task.
"""

import json

from ground_pack_check_support import ROOT, handoff
from run_ground_consumer_checks import preserved
from tools.ground_payload_binding import checked_reference, reference

RUN = ROOT / "logs/graphics/selected-ground-runtime/20261021-transport-01"
DESTINATION = ROOT / "logs/proofs/mlm/native-art/ground/ground-consumers-v1"


def qualify():
    """Join both native configurations and accepted source pixels, retaining exact executed binaries.

    @return Bounded purchase qualification with subsequent body/station scopes explicitly pending.
    """
    native = []
    # Keep distinct executed images for this immutable partial epoch. Later
    # body/station observers may relink only their current working executable.
    for configuration in ("Debug", "RelWithDebInfo"):
        execution_path = RUN / ("purchases-" + configuration.lower()) / "purchases.json"
        pixels_path = RUN / ("purchases-pixels-" + configuration.lower() + ".json")
        execution = handoff.read_json(execution_path.relative_to(ROOT).as_posix())
        pixels = handoff.read_json(pixels_path.relative_to(ROOT).as_posix())
        observed = execution["observation"]
        # The complete native matrix and independently read source pixels must
        # belong to this exact process, rather than a refreshed report identity.
        if (execution.get("passed") is not True or observed["assertions"] != 180 or observed["exit_code"] != 0
                or pixels.get("passed") is not True or pixels["execution"] != reference(execution_path)
                or pixels["matched_source_pixels"] != 1155 or pixels["native_purchase_views"] != 10):
            raise ValueError("purchase qualification lacks complete native/source proof")
        for row in execution["inputs"] + observed["artifacts"] + [observed[key] for key in ("application", "source", "terminal", "runtime_payload")]:
            checked_reference(row)
        for frame in pixels["frames"]:
            for key in ("source_atlas", "source_manifest", "native_rgb"):
                checked_reference(frame[key])
        application = checked_reference(observed["application"])
        retained = DESTINATION / "transport-purchases-bin" / configuration / application.name
        retained.parent.mkdir(parents=True, exist_ok=True)
        if retained.exists() and retained.read_bytes() != application.read_bytes():
            raise ValueError("purchase observer retention conflicts with an existing executed image")
        if not retained.exists():
            retained.write_bytes(application.read_bytes())
        native.append({"configuration": configuration, "execution": reference(execution_path), "pixels": reference(pixels_path),
                       "executed_application": observed["application"], "retained_application": reference(retained),
                       "native_assertions": 180, "native_purchase_views": 10, "matched_source_pixels": 1155})
    transitions, count = preserved((RUN / "before.json").relative_to(ROOT).as_posix())
    result = {"_documentation": __doc__, "passed": True, "native": native, "preserved_file_count": count,
              "transitions": transitions, "source_pixels_and_anchors_exact": True,
              "map_body_coverage": False, "station_foreground_coverage": False, "human_acceptance": "pending",
              "inputs": [reference(ROOT / path) for path in ("src/tests/ground_transport_runtime.cpp",
                         "src/tests/multilayer/ground_transport_pixel_checks.py",
                         "src/tests/multilayer/qualify_ground_transport_purchases.py")]}
    (DESTINATION / "transport-purchases.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


if __name__ == "__main__":
    result = qualify()
    print(json.dumps({"passed": result["passed"], "configurations": 2, "purchase_views_each": 10, "matched_source_pixels_each": 1155}))
