"""Actual native geometry and role contracts for remaining orbital artwork.

@file orbital_art_contract.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups13B/13D
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups65-75, hauler refinement66B.1
These graphics mappings consume existing native definitions. They grant no
orientation, inventory, work, capability, sprite allocation or delivered source.
"""

MODULE_LAYOUTS = {
    "module.nucleus": (("Core",), (3, 3)),
    "module.dock": (("Dock",), (2, 2)),
    "module.storage": (("Storage",), (2, 2)),
    "module.workshop": (("Assembly",), (2, 2)),
    "module.processor": (("Steelworks", "Goodsworks", "Refinery", "IceProcessor"), (3, 3)),
    "module.power": (("Power",), (2, 2)),
    "module.fuel-store": (("FuelStore",), (2, 2)),
}


def module_contract(asset_id):
    """Describe one actual fixed native site without inventing a rotation field.

    @param asset_id Existing stable public module family.
    @return Fresh native layout/ownership fields for its planned source contract.
    """
    kinds, size = MODULE_LAYOUTS[asset_id]
    return dict(footprint=list(size), tile_variants=size[0] * size[1], directions=["none"],
                native_build_kinds=list(kinds), variant_semantics="native_fixed_site_world_tiles",
                native_orientation_policy="fixed_axis_aligned; SpaceStructureState has no saved orientation",
                native_layout_owner="GetSpaceConstructionDefinitions/SpaceStructureState",
                native_state_owner="SpaceStructureState::phase/OrbitalConstructionJob; actual native module utility and work owners",
                native_inventory_owner="OrbitalConstructionJob::inputs and OrbitalModule::stock; original CargoPacket owners",
                native_frame_policy="Actual original construction/service/processing or utility receipt selects an authored phase; paused/missing work stays canonical static. No art-local clock.")


def heavy_miner_contract():
    """Append the actual heavy mining role while retaining the construction robot16.

    @return Complete required geometry/state overrides for model21's planned family.
    """
    return dict(consumer=11, kind="vehicle", native_model_id=21, native_role="heavy_mining_robot",
                directions=["N", "NE", "E", "SE", "S", "SW", "W", "NW"],
                states=["free", "rendezvous", "attached", "mining", "full", "return", "service", "blocked"],
                frames=2, native_size=[64, 48], anchor=[-32, -32], bounds=[0, 0, 64, 48],
                footprint=[0, 0], tile_variants=1, parts=["body"], company_colors="none",
                native_role_owner="SPACE_HEAVY_MINER_MODEL/GetSpaceMiningTuning; never model16",
                native_state_policy=dict(attachment="Original reciprocal AsteroidBody::mining_slot lease and saved attachment point",
                                         mining="IsSpaceMiningWorkTick and actual mission work_ticks; no new work or resource clock",
                                         load="Original native cargo.TotalCount/cargo_cap; no art inventory",
                                         service="Original onboard manifest or generation-qualified Core/Dock service receipt",
                                         movement="Original unstopped admitted travel to a real outstanding destination",
                                         blocked="Original native order/mining/service admission reason"),
                source_frame_contract=dict(free=[0, 0], rendezvous=[0, 1], attached=[0, 0], mining=[0, 1],
                                           full=[0, 0], **{"return": [0, 1]}, service=[0, 1], blocked=[0, 0]))


def retained_robot_contract():
    """Preserve the distinct existing cargo-less robot without borrowing probe work.

    @return Role/source policy only; existing model16 states and identity stay intact.
    """
    return dict(native_model_id=16, native_role="retained_heavy_construction_robot", company_colors="none",
                native_role_owner="GetSpaceVehicleModel/SpaceBuildKind::HeavyRobot; distinct from heavy miner21 and builder20",
                native_work_policy="Current ReadyProbe admits model20 only. Model16 working source is an explicit idle alias and cannot claim another hull's work.",
                source_frame_contract=dict(idle=[0, 0], moving=[0, 1], working=[0, 0], service=[0, 1], blocked=[0, 0]))


def hauler_contract():
    """Preserve native freight in every reachable hauler presentation state.

    @return Ten-state source policy with original eight ordinals, appended loaded-blocked cues and an explicit unreachable-service alias; no new cargo, service or delivery authority.
    """
    return dict(native_model_id=17, native_role="orbital_cargo_hauler", company_colors="none",
                states=["empty", "partial", "full", "moving-empty", "moving-partial", "moving-full", "service", "blocked",
                        "blocked-partial", "blocked-full"], frames=2,
                native_role_owner="GetSpaceVehicleModel/SpaceBuildKind::Hauler",
                native_load_owner="Original native CargoPacket hold and GetSpaceFreightCapacity for its actual cargo; no painted extra capacity",
                native_motion_owner="Original generation-qualified typed order and outstanding actual movement; stopped/arrived craft use the actual static load pose",
                native_service_owner="GetSpaceServiceTuning excludes17 and Core/Dock ReadyCraft requires that tuning. Service is an unreachable empty-source alias, never a paid repair producer.",
                native_quantity_cue_policy="Original native CargoPacket count/capacity selects empty/partial/full in static, moving and blocked poses; blocking never erases actual freight.",
                source_state_aliases={"service": "empty"},
                source_frame_contract={"empty": [0, 0], "partial": [0, 0], "full": [0, 0], "moving-empty": [0, 1],
                                       "moving-partial": [0, 1], "moving-full": [0, 1], "service": [0, 0], "blocked": [0, 0],
                                       "blocked-partial": [0, 0], "blocked-full": [0, 0]})


def validate_orbital_contract(row):
    """Refuse draft art that changes native module geometry or confuses Space roles.

    @param row Selected public family, possibly already authored or packed later.
    @return None; raise ValueError before an invalid source contract can publish.
    """
    asset = row["asset_id"]
    if asset in MODULE_LAYOUTS:
        expected = module_contract(asset)
    elif asset == "vehicle.model-21":
        expected = heavy_miner_contract()
    elif asset == "vehicle.model-16":
        expected = retained_robot_contract()
    elif asset == "vehicle.model-17":
        expected = hauler_contract()
    else:
        return
    # Geometry and role metadata must agree with the same original native producer. An artist's draft cannot add rotation, shrink a paid footprint or turn robot16 into miner21.
    if any(row.get(key) != value for key, value in expected.items()):
        raise ValueError("native orbital geometry/role contract changed: " + asset)
