"""Publish source geometry for actual conventional ground freight capabilities.

@file ground_fleet_contract.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group20C
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group20B
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group21
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group22
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
This read-only art contract consumes the native catalogue. It grants no cargo,
vehicle, blocked reason, calendar exception or independent stock authority.
"""

from pathlib import Path
import json
import re


ROOT = Path(__file__).resolve().parents[3]
DIRECTIONS = ("N", "NE", "E", "SE", "S", "SW", "W", "NW")


def native_cargo_labels(model_id=5):
    """Read the actual selected conventional ground model's cargo capability.

    @param model_id Actual ordinary05/sealed06 wagon or road07/08; default05.
    @return Exact admitted labels from the native model's capability owner.
    @throws ValueError If the expected native family/export is unavailable.
    """
    source = (ROOT / "src/multilayer_vehicle_catalogue.cpp").read_text(encoding="utf-8")
    definitions = {5: ("_mineral_cargo", "Mineral and heavy freight wagon"),
                   6: ("_sealed_cargo", "Sealed mineral wagon"), 7: ("_bulk_cargo", "Mine bulk truck"),
                   8: ("_sealed_cargo", "Sealed mineral carrier")}
    if model_id not in definitions:
        raise ValueError("unsupported conventional ground fleet source model")
    symbol, name = definitions[model_id]
    declaration = re.search(r"static constexpr CargoLabel " + symbol + r"\[\]\s*=\s*\{([^;]+)\};", source)
    model = re.search(r'\{' + str(model_id) + r', "' + re.escape(name) + r'"[^\n]+' + symbol, source)
    # An absent or rebound catalogue cannot authorize plausible replacement
    # labels. The source matrix must still describe the actual published model.
    if declaration is None or model is None:
        raise ValueError("native ordinary wagon cargo capability is unavailable")
    labels = re.findall(r'CargoLabel\{"([A-Z0-9]{4})"\}', declaration.group(1))
    # Duplicate/absent meanings would route distinct cargo to the wrong visual
    # variant, so a plausible partial table must fail before authoring proceeds.
    if not labels or len(labels) != len(set(labels)):
        raise ValueError("native ordinary wagon cargo labels are empty or ambiguous")
    return labels


def native_cargo_forms(model_id=5):
    """Resolve actual physical cargo forms for the selected conventional vehicle.

    @param model_id Actual ordinary05/sealed06 wagon or road07/08; default05.
    @return Native admitted label-to-form mapping from existing content owners.
    @throws ValueError On a missing baseline trait or unrecognized native label.
    """
    catalogue = json.loads((ROOT / "media/multilayer/cargo_catalogue.json").read_text(encoding="utf-8"))
    forms = {row["label"]: row["form"] for row in catalogue["cargoes"]}
    source = (ROOT / "src/multilayer_content.cpp").read_text(encoding="utf-8")
    baseline = {"COAL": ("COAL", "DryBulk", "dry_bulk"), "IORE": ("IRON_ORE", "DryBulk", "dry_bulk"),
                "STEL": ("STEEL", "Ingots", "ingots"), "GOOD": ("GOODS", "Packaged", "packaged")}
    # Original cargos keep their native baseline traits. The art fixture may
    # verify those declarations but cannot replace them with local cargo IDs or
    # silently reinterpret a covered payload as inexpensive loose granules.
    for label, (constant, native_form, public_form) in baseline.items():
        pattern = r"\{\s*CT_" + constant + r"\s*,\s*MultiLayerCargoForm::" + native_form + r"\s*,"
        if re.search(pattern, source) is None:
            raise ValueError("native baseline cargo form changed: " + label)
        forms[label] = public_form
    labels = native_cargo_labels(model_id)
    if any(label not in forms for label in labels):
        raise ValueError("ordinary wagon native cargo form is unavailable")
    return {label: forms[label] for label in labels}


def variant_contract(model_id=5):
    """Map actual cargo labels to stable source variants and existing state facts.

    @param model_id Actual ordinary05/sealed06 wagon or road07/08; default05.
    @return Public artwork fields with source-only native ownership policies.
    """
    labels = native_cargo_labels(model_id)
    result = {"variant_semantics": "native_vehicle_cargo_label",
            "native_cargo_labels": labels, "tile_variants": len(labels),
            "native_state_policy": {
                "cargo_owner": "Train::cargo.StoredCount()/Train::cargo_cap",
                "cargo_label_owner": f"CargoSpec::Get(train.cargo_type)->label; validate GetUndergroundEngineCapability(model{model_id:02})",
                "empty": "StoredCount()==0",
                "partial": "0<StoredCount()<cargo_cap",
                "full": "cargo_cap>0 and StoredCount()>=cargo_cap",
                "service_empty": "StoredCount()==0 and train.IsInDepot() and IsRailDepotTile(train.tile)",
                "blocked_empty": "StoredCount()==0 and head->flags.Test(VehicleRailFlag::Stuck), after service priority",
                "priority": ["packet-backed partial/full", "actual rail depot service when empty", "actual head reservation hold when empty", "empty"],
                "frame_owner": "empty/partial/full wheel phases only while head actually moves; service/blocked lamp phases keep gear static",
                "native_length": 8, "shorten_factor": 0}}
    if model_id in (6, 8):
        # Both actual covered mineral cargos share one genuinely sealed exterior.
        # Packet-backed external fill cues retain load visibility without opening
        # the cover, changing vehicle geometry or introducing another inventory.
        result.update(variant_semantics="sealed_native_cargo_exterior", tile_variants=1,
                      sealed_load_policy="URAN/RARE share the closed original body; external empty/partial/full indicator follows actual packets only, never visible loose cargo or an open hatch")
    if model_id in (7, 8):
        # Road traffic has its own existing controller/occupancy facts. Artwork
        # reads the stopped head's real traffic-block counter; it never calls a
        # mutating blocker probe or borrows the train reservation-Stuck flag.
        result["native_state_policy"] = {
            "cargo_owner": "RoadVehicle::cargo.StoredCount()/RoadVehicle::cargo_cap",
            "cargo_label_owner": f"CargoSpec::Get(road_vehicle.cargo_type)->label; validate GetUndergroundEngineCapability(model{model_id:02})",
            "empty": "StoredCount()==0", "partial": "0<StoredCount()<cargo_cap",
            "full": "cargo_cap>0 and StoredCount()>=cargo_cap",
            "service_empty": "StoredCount()==0 and road_vehicle.IsInDepot() and IsRoadDepotTile(road_vehicle.tile)",
            "blocked_empty": "StoredCount()==0 and head->blocked_ctr>0 and head->cur_speed==0 and !head->vehstatus.Test(VehState::Stopped) and !head->vehstatus.Test(VehState::Crashed), after service priority",
            "priority": ["packet-backed partial/full", "actual road depot service when empty", "actual stopped head traffic-block counter when empty", "empty"],
            "frame_owner": "empty/partial/full tyre phases only while road head actually moves and is neither Stopped nor Crashed; service/blocked lamp phases keep tyres static",
            "native_length": 8, "shorten_factor": 0}
    return result


def cargo_geometry():
    """Define load-only regions inside the reviewed eight-view gondola anatomy.

    @return Per-view bulk apertures, raised protected-load envelopes and front rims.
    """
    full = {"N": [[27, 19], [35, 19], [35, 27], [27, 27]],
            "NE": [[36, 24], [39, 26], [26, 32], [23, 30]],
            "E": [[20, 31], [43, 31], [43, 32], [20, 32]],
            "SE": [[25, 24], [40, 31], [37, 32], [23, 25]],
            "S": [[27, 19], [35, 19], [35, 27], [27, 27]],
            "SW": [[36, 24], [39, 26], [26, 32], [23, 30]],
            "W": [[20, 31], [43, 31], [43, 32], [20, 32]],
            "NW": [[25, 24], [40, 31], [37, 32], [23, 25]]}
    partial = {"N": [[27, 24], [35, 24], [35, 27], [27, 27]],
               "NE": [[30, 27], [33, 29], [26, 32], [23, 30]],
               "E": [[20, 31], [31, 31], [31, 32], [20, 32]],
               "SE": [[25, 24], [32, 27], [29, 29], [23, 25]],
               "S": [[27, 19], [35, 19], [35, 23], [27, 23]],
               "SW": [[36, 24], [39, 26], [33, 29], [30, 27]],
               "W": [[32, 31], [43, 31], [43, 32], [32, 32]],
               "NW": [[32, 27], [40, 31], [37, 32], [29, 29]]}
    raised = {"N": [[27, 14], [35, 14], [35, 27], [27, 27]],
              "NE": [[36, 18], [39, 20], [39, 26], [26, 32], [23, 30], [23, 24]],
              "E": [[20, 25], [43, 25], [43, 32], [20, 32]],
              "SE": [[25, 18], [40, 25], [40, 31], [37, 32], [23, 25], [23, 19]],
              "S": [[27, 14], [35, 14], [35, 27], [27, 27]],
              "SW": [[36, 18], [39, 20], [39, 26], [26, 32], [23, 30], [23, 24]],
              "W": [[20, 25], [43, 25], [43, 32], [20, 32]],
              "NW": [[25, 18], [40, 25], [40, 31], [37, 32], [23, 25], [23, 19]]}
    rims = {"N": [[[26, 28], [36, 28]]],
            "NE": [[[39, 27], [26, 33]], [[26, 33], [22, 31]]],
            "E": [[[19, 33], [44, 33]]],
            "SE": [[[23, 26], [37, 33]], [[37, 33], [40, 31]]],
            "S": [[[26, 28], [36, 28]]],
            "SW": [[[39, 27], [26, 33]], [[26, 33], [22, 31]]],
            "W": [[[19, 33], [44, 33]]],
            "NW": [[[23, 26], [37, 33]], [[37, 33], [40, 31]]]}
    return {"bulk_full_polygons": full, "bulk_partial_polygons": partial,
            "protected_load_envelopes": raised, "foreground_rim_lines": rims,
            "bulk_alpha_extension": False,
            "partial_must_fit_full_aperture": True,
            "protected_alpha_extension": "only inside the approved raised envelope, behind retained front rim",
            "bulk_material_policy": "authored isotropic granule interiors; no directional hardware, recoloring or synthetic body view",
            "partial_policy": "rear B-end occupied portion; visual category for any positive nonfull packet count, not a second volume ledger"}


def mine_truck_cargo_geometry():
    """Describe actual model07 open-bed floors without borrowing rail apertures.

    @return Native partial/full floor polygons, retained rims and load policy.
    """
    full = {"N": [[28, 26], [34, 26], [34, 30], [28, 30]],
            "NE": [[24, 31], [32, 27], [34, 27], [28, 31]],
            "E": [[18, 31], [38, 31], [38, 31], [18, 31]],
            "SE": [[25, 24], [27, 24], [34, 28], [32, 28]],
            "S": [[28, 20], [34, 20], [34, 24], [28, 24]],
            "SW": [[29, 28], [36, 24], [38, 24], [31, 28]],
            "W": [[26, 31], [43, 31], [43, 31], [26, 31]],
            "NW": [[28, 27], [30, 27], [38, 32], [36, 32]]}
    partial = {"N": [[28, 28], [34, 28], [34, 30], [28, 30]],
               "NE": [[24, 31], [28, 29], [32, 29], [28, 31]],
               "E": [[18, 31], [27, 31], [27, 31], [18, 31]],
               "SE": [[25, 24], [27, 24], [30, 26], [28, 26]],
               "S": [[28, 20], [34, 20], [34, 22], [28, 22]],
               "SW": [[33, 26], [36, 24], [38, 24], [35, 26]],
               "W": [[35, 31], [43, 31], [43, 31], [35, 31]],
               "NW": [[32, 30], [34, 30], [38, 32], [36, 32]]}
    rims = {"N": [[[27, 31], [35, 31]]], "NE": [[[29, 32], [36, 28]]],
            "E": [[[17, 32], [39, 32]]], "SE": [[[24, 25], [35, 30]]],
            "S": [[[27, 25], [35, 25]]], "SW": [[[32, 29], [40, 25]]],
            "W": [[[25, 32], [44, 32]]], "NW": [[[27, 29], [38, 34]]]}
    return {"bulk_full_polygons": full, "bulk_partial_polygons": partial,
            "foreground_rim_lines": rims, "bulk_alpha_extension": False,
            "partial_must_fit_full_aperture": True,
            "bulk_material_policy": "original opaque isotropic grain/stone interiors only; no source heap outline, rail body, recoloring or hardware",
            "partial_policy": "rear B-end occupied floor portion, opposite the yellow A/front cab; visual packet category, never a second volume ledger",
            "bed_policy": "original lowered open road bed, fixed cab/steel/tyres/alpha/contact; no tipping animation or changed road length"}
