# Focused content/geology/portal/TBM/mine/toolbar fixtures: docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md, r2mlvs2-geology-and-excavation.md, r2mlvs3-sealed-portals-and-deep-rail.md, r2mlvs4-tunnel-boring-machines.md, r2mlvs5-spoil-wagons-and-haulage.md, r2mlvs6-mines-and-resource-chains.md and r2mlvs7-underground-construction-and-fleet.md.
# Pure Space terminal/attachment geometry: docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups9-10.
# Passive generator/site source boundaries: docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8B.
# Actual collector rendering: docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79C.
# Orbital Core activation and sole packet stock: docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1.
# Contained Core service and shared electrical settlement: docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1B.
# Native processor work, independent copied records and ordinary files: docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C.
# Read-only orbital source selection: docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K.
# Paid processor construction/resource routes: docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group11.
# Explicit paid-hauler packet/refit boundaries: docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12.
# Actual processor product returns and linked Surface collection: docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12B.
# Conventional station source geometry: docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group24B.
# Native Surface processor geometry/state: docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group39B.
add_test_files(
    # Detached original-version/capability refusals: archi-savegame.md; m2o23-1-come-space.md Groups5/9B.
    save_lineage_policy.cpp
    # Shared artifact ownership: docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Group 3.
    test_artifact_paths.h
    test_artifact_paths.cpp
    artifact_paths.cpp
    space_preview.cpp
    # Final read-only native Space art: r2mlvs10-asteroids-and-mining-probes.md Group14BG.
    space_sprite.cpp
    # Original rocket source/flight selection: r2mlvs14 Group62J.
    launch_fleet_sprite.cpp
    # Original station zones, work admission and foreground scopes: r2mlvs14 Group62JB.
    launch_facility_sprite.cpp
    launch_facility_render.cpp
    launch_fleet_paid_review.cpp
    launch_craft_window.cpp
    launch_pod_review.cpp
    # Original craft work/load/service owner isolation: r2mlvs11-orbital-stations-and-manufacturing.md Group13K.
    orbital_sprite_craft.cpp
    orbital_sprite_module.cpp
    # Paid ordinary construction milestones: r2mlvs11-orbital-stations-and-manufacturing.md Group13L.
    orbital_art_native.cpp
    # Exact native population/body/probe cost boundaries: r2mlvs10-asteroids-and-mining-probes.md Group21.
    asteroid_work_costs-1.cpp
    asteroid_work_costs-2.cpp
    asteroid_work_fixture.h
    probe_mining_fixture.h
    # Immutable class/size definitions and inclusive sampling: r2mlvs10-asteroids-and-mining-probes.md Group1.
    asteroid_catalogue.cpp
    # Pure company disclosure and signed motion presentation: r2mlvs10-asteroids-and-mining-probes.md Group14.
    asteroid_inspection.cpp
    asteroid_inspection_native.cpp
    # Bounded real-site starter selection and frozen inputs: r2mlvs10-asteroids-and-mining-probes.md Group2.
    asteroid_generation.cpp
    asteroid_generation_native.cpp
    # Integer trajectory, envelope and numerical budget fixtures: r2mlvs10-asteroids-and-mining-probes.md Group3.
    asteroid_motion.cpp
    space_straight_motion.cpp
    space_attachment_safety.cpp
    space_departure_plan.cpp
    space_body_approach_safety.cpp
    asteroid_renewal.cpp
    asteroid_identity.cpp
    space_spatial_membership.cpp
    asteroid_live_validation.cpp
    asteroid_preview_identity.cpp
    space_identity_consumers.cpp
    asteroid_cargo_origin.cpp
    asteroid_cargo_wire.cpp
    asteroid_field_wire.cpp
    asteroid_cargo_native.cpp
    asteroid_cargo_validation.cpp
    asteroid_cargo_custody.cpp
    space_mining_policy.cpp
    space_mining_wire.cpp
    space_mining_target.cpp
    space_mining_continuation.cpp
    space_mining_departure.cpp
    space_survey_departure.cpp
    asteroid_renewal_proposal.cpp
    asteroid_retirement.cpp
    asteroid_creation.cpp
    asteroid_lifecycle_tick.cpp
    asteroid_lifecycle_native.cpp
    allocation_failure_fixture.cpp
    space_mining_intercept_native.cpp
    space_return_target.cpp
    space_return_surface.cpp
    space_return_service.cpp
    asteroid_motion_native.cpp
    # Persistent depletion/invalid-target boundaries: r2mlvs10-asteroids-and-mining-probes.md Group5.
    asteroid_depletion.cpp
    asteroid_depletion_native.cpp
    # Shared body-strip/static-footprint separation: r2mlvs10-asteroids-and-mining-probes.md Group6.
    asteroid_band.cpp
    space_mining.cpp
    # Model15 catalogue binding and real paid-hull mining: r2mlvs10-asteroids-and-mining-probes.md Group8.
    probe_shell.cpp
    probe_shell_native.cpp
    space_content.cpp
    space_orders.cpp
    # Actual typed freight/service completion: r2mlvs10-asteroids-and-mining-probes.md Group8B.
    probe_order_actions.cpp
    probe_order_actions_native.cpp
    # Current paid scan/company/migration and original network-queue dispatch: r2mlvs10-asteroids-and-mining-probes.md task8B.4.
    probe_mission_native.cpp
    # Per-miner legacy/operating contract: r2mlvs11-orbital-stations-and-manufacturing.md Group9.
    heavy_mining_tuning.cpp
    # Exact legacy2/current3 and original paid heavy factory: r2mlvs11-orbital-stations-and-manufacturing.md Group9B.
    heavy_commissioning.cpp
    heavy_commissioning_native.cpp
    # Actual per-model native extraction and sustainable explicit orbital return: r2mlvs11 Group9C.
    heavy_mining_runtime.cpp
    heavy_mining_runtime_native.cpp
    # Original paid rocket/shell custody at a genuine orbital factory hull: r2mlvs11 Group9CB.
    heavy_supply_custody.cpp
    heavy_supply_custody_native.cpp
    legacy_descent_native.cpp
    heavy_descent.cpp
    heavy_descent_native.cpp
    heavy_onboard_service.cpp
    heavy_onboard_service_native.cpp
    # Genuine pre-heavy one-part Core service transport: r2mlvs11 Group9CCB.
    legacy_contained_service_native.cpp
    # Immutable original contained service bills and shared Core/Dock work: r2mlvs11 Group9CCC.
    heavy_contained_service.cpp
    heavy_contained_service_native.cpp
    # Actual paid Dock construction and original heavy/legacy service: r2mlvs11-orbital-stations-and-manufacturing.md Group 9CCCC.
    heavy_dock_service_native.cpp
    heavy_dock_work_native.cpp
    # Ordinary native/Space copied packet exclusivity: r2mlvs11-orbital-stations-and-manufacturing.md Group 9CD.
    heavy_joined_validation.cpp
    heavy_joined_native.cpp
    # Actual original15/21 lease, finite work and native upkeep: r2mlvs11-orbital-stations-and-manufacturing.md Group9D.
    heavy_role_native.cpp
    heavy_cycle_native.cpp
    heavy_network_native.cpp
    # Shared deterministic approach and queued-save contracts: r2mlvs8 Group11 / r2mlvs11 Group4B.
    space_approach.cpp
    space_survey.cpp
    space_resupply.cpp
    # Direct native module flight and exact same-rocket recovery: r2mlvs9 Group11.
    orbital_freight.cpp
    orbital_freight_native.cpp
    # Whole-label/fragment and physical custody refusal: r2mlvs9 Group11C.
    orbital_freight_bounds.cpp
    orbital_freight_failure.cpp
    space_construction.cpp
    orbital_core.cpp
    orbital_service.cpp
    orbital_module_reservation.cpp
    orbital_module_assembly.cpp
    # Bounded activation, native paid Storage freight and original Power panels: Plan11 Group4.
    orbital_module_activation.cpp
    orbital_recipe_inventory.cpp
    orbital_recipe_files.cpp
    orbital_processor_native.cpp
    processor_freight.cpp
    processor_freight_fixture.h
    processor_freight_native.cpp
    processor_returns_native.cpp
    # Original paid product command dispatch: r2mlvs10-asteroids-and-mining-probes.md task8C.4.
    probe_product_replay_native.cpp
    probe_replay_native.cpp
    # r2sff1-space-fixes-features-1.md Group32: actual late join and complete private-renewal replay.
    space_lifecycle_peer_native.cpp
    # r2sff1-space-fixes-features-1.md Group33: live storage and native event work at exact population fixtures.
    asteroid_lifecycle_work.cpp
    # Moving-target pursuit and station-free paid deployment: r2mlvs10-asteroids-and-mining-probes.md Group9.
    probe_pursuit.cpp
    probe_pursuit_native.cpp
    # Finite extraction, original provenance and allocation refusal: r2mlvs10-asteroids-and-mining-probes.md Group10.
    probe_extraction.cpp
    probe_extraction_native.cpp
    # Native retained-cargo departure/circle entry: r2mlvs10-asteroids-and-mining-probes.md Group11.
    probe_return_native.cpp
    # Actual descent, paid resupply and repeat: r2mlvs10-asteroids-and-mining-probes.md Group12.
    probe_repeat_native.cpp
    # Recall/recovery shared command and saved owners: r2mlvs10-asteroids-and-mining-probes.md Group13.
    probe_exit_native.cpp
    # Same-tick interception and saved edge decisions: r2mlvs10-asteroids-and-mining-probes.md Group15.
    probe_interception_native.cpp
    # Actual exit/retained recovery boundaries: r2mlvs10-asteroids-and-mining-probes.md Group16.
    probe_boundary_native.cpp
    # Conserved original paid phase saves/next decisions: r2mlvs10-asteroids-and-mining-probes.md Group17.
    probe_conservation_native.cpp
    # Three actual paid resource missions and declared drop boundary: r2mlvs10-asteroids-and-mining-probes.md Group18.
    probe_class_mission_native.cpp
    # Actual linked Surface carrier delivery: r2mlvs10-asteroids-and-mining-probes.md Group19.
    surface_return_delivery_native.cpp
    # Paid Assembly activation/source binding before manufacturing: r2mlvs11 Group6B.
    assembly_facility.cpp
    assembly_native_fixture.h
    assembly_facility_native.cpp
    manufacturing_reservation.cpp
    manufacturing_reservation_native.cpp
    # Actual shared-grant progress, final material consumption and native orbital factory provenance: r2mlvs11 Group8.
    manufacturing_materialization.cpp
    manufacturing_materialization_native.cpp
    # Ordinary purchase commands consume the paid native producer: r2mlvs11-orbital-stations-and-manufacturing.md Group8B.
    manufacturing_purchase.cpp
    # Independent paid native clone intent: r2mlvs11-orbital-stations-and-manufacturing.md Group8BB.
    manufacturing_clone.cpp
    manufacturing_clone_native.cpp
    # Actual native paid replacement settlement: r2mlvs11-orbital-stations-and-manufacturing.md Group8BC.
    manufacturing_replacement.cpp
    manufacturing_replacement-1.cpp
    manufacturing_replacement_native.cpp
    manufacturing_purchase_native.cpp
    # Explicit source-connected two-berth Dock owner: r2mlvs11 Group4C.
    orbital_dock.cpp
    orbital_dock_native.cpp
    orbital_utility_retirement.cpp
    # Safe empty removal and retained spent receipts: r2mlvs11 Group5.
    orbital_removal.cpp
    orbital_removal_native.cpp
    # All-label packet evacuation and cancellation: r2mlvs11 Group5B.
    orbital_evacuation.cpp
    orbital_evacuation_native.cpp
    # Real takeover/reuse and paid replacement-Core dependency removal: r2mlvs11 Group5C.
    orbital_module_lifecycle.cpp
    # Public launch owner/signature handoff: r2mlvs9-launch-centers-and-rockets.md Group7.
    launch_contract.cpp
    # Revised reusable-hull and first-Core numerical fixtures: Plan9 Group9.
    launch_economics.cpp
    launch_production.cpp
    space_logistics.cpp
    launch_flight.cpp
    space_descent.cpp
    # Actual current-position release/foreign overlap and native reservation quotes: r2mlvs9 Group12.
    orbital_release_admission.cpp
    descent_custody.cpp
    descent_custody_native.cpp
    cargo_return_native.cpp
    space_company_custody_native.cpp
    launch_failure_phase_native.cpp
    launch_retirement_native.cpp
    paid_probe_phase_native.cpp
    surface_space_loop_native.cpp
    # Exact circles/clipped targets and actual moving release commands: r2mlvs9-launch-centers-and-rockets.md Group18.
    orbital_release_geometry.cpp
    # Actual multi-cargo recovery and separate boundary collection: r2mlvs9-launch-centers-and-rockets.md Group19.
    surface_collection_native.cpp
    surface_collection_boundaries.cpp
    # Native packet/provenance/fee conservation through repeated hub circulation: r2mlvs9-launch-centers-and-rockets.md Group20.
    hub_circulation_native.cpp
    launch_center.cpp
    # Actual command's company-independent terrain predicate: r2mlvs10-asteroids-and-mining-probes.md Group2.
    launch_site.cpp
    alternating_iterator.cpp
    bitmath_func.cpp
    console.cpp
    enum_over_optimisation.cpp
    excavation_storage.cpp
    excavation_contract.cpp
    excavation_guard.cpp
    excavation_clearing.cpp
    excavation_jobs.cpp
    excavation_cut.cpp
    excavation_outline.cpp
    excavation_route.cpp
    excavation_pass.cpp
    excavation_starter.cpp
    excavation_budget.cpp
    excavation_fixture.h
    solid_starter_fixture.h
    tbm_model.cpp
    tbm_engine.cpp
    tbm_fixture.h
    tbm_acquisition.cpp
    tbm_project.cpp
    tbm_area.cpp
    tbm_replanning.cpp
    tbm_planning_commands.cpp
    tbm_planning_seed.cpp
    tbm_planning_refusals.cpp
    # Native map and construction presentation: archi-map-layers.md; m2o23-2 Group24.
    layer_construction_controls.cpp
    # Read-only presentation and synchronized planning/window actions: archi-gui.md; m2o23-2 Group24B.
    tbm_status_presentation.cpp
    tbm_work_change_window.cpp
    tbm_face_status.cpp
    # Actual pending-settings/accepted-policy boundary: archi-map-layers.md; m2o23-2 Group24C.
    newgame_layer_policy.cpp
    window_hotkey_toggle.cpp
    tbm_commission.cpp
    tbm_commission_fixture.h
    tbm_economics.cpp
    tbm_logistics_save.cpp
    tbm_logistics_runtime.cpp
    # Native GUI input and movement baseline: r2tbmf-tbm-fixes.md Group1.
    tbm_feedback_runtime.cpp
    tbm_motion_reload.cpp
    tbm_pacing.cpp
    tbm_advice.cpp
    tbm_parking.cpp
    tbm_release.cpp
    tbm_hangar.cpp
    tbm_hangar_service.cpp
    tbm_exit.cpp
    tbm_feedback_haul_fixture.h
    native_art_loop_fixture.h
    native_sound_runtime_fixture.h
    # Actual M1 base-set loading: r2mlvs13-graphics-and-asset-pipeline.md Groups79B/80B.
    multilayer_packed_load.cpp
    multilayer_sprite_selection.cpp
    multilayer_terrain_portal.cpp
    multilayer_terrain_runtime.cpp
    multilayer_train_art.cpp
    multilayer_facility_selection.cpp
    multilayer_collector_selection.cpp
    multilayer_collector_runtime.cpp
    multilayer_control_selection.cpp
    multilayer_control_runtime.cpp
    multilayer_facility_runtime.cpp
    multilayer_sound_selection.cpp
    tbm_logistics_stalls.cpp
    tbm_logistics_queue.cpp
    tbm_logistics_crash.cpp
    logistics_crash_fixture.h
    tbm_depot_workflow.cpp
    tbm_depot_trace.cpp
    tbm_depot_save.cpp
    tbm_depot_stalls.cpp
    tbm_depot_queue.cpp
    tbm_depot_crash.cpp
    depot_file_fixture.h
    underground_depot_fixture.h
    tbm_conservation.cpp
    tbm_phase.cpp
    tbm_consumer.cpp
    portal_tbm_access.cpp
    tbm_work.cpp
    tbm_support.cpp
    tbm_route.cpp
    tbm_coverage.cpp
    tbm_cost.cpp
    tbm_corridor.cpp
    tbm_reuse.cpp
    tbm_exchange_fixture.h
    tbm_merge_observation.cpp
    tbm_face_fixture.h
    tbm_face_stand.cpp
    tbm_face_meeting.cpp
    tbm_face_deep.cpp
    tbm_face_preflight.cpp
    tbm_face_transfer.cpp
    tbm_face_turn.cpp
    tbm_face_steering.cpp
    tbm_face_access.cpp
    tbm_face_cycle.cpp
    # Native installed phase activation: archi-map-layers.md; m2o23-2-come-space.md Group23.
    tbm_face_controller.cpp
    tbm_face_controller_loop.cpp
    tbm_remainder.cpp
    tbm_change_work.cpp
    excavation_approach.cpp
    tbm_approach.cpp
    tbm_obstruction.cpp
    tbm_face_support.cpp
    tbm_face_lifecycle.cpp
    tbm_face_regression.cpp
    tbm_reserved_loop.cpp
    excavation_claim_fixture.h
    tbm_face_loop_watch.h
    tbm_source_loop_fixture.h
    portal_signal_boundary.cpp
    tbm_exchange_bay.cpp
    tbm_exchange_proposal.cpp
    tbm_exchange_commit.cpp
    tbm_exchange_origin.cpp
    tbm_exchange_consumer.cpp
    tbm_hauler_cycle.cpp
    tbm_wagon_sets.cpp
    spoil_receiver_fixture.h
    spoil_receiver_definition.cpp
    spoil_receiver_intake.cpp
    spoil_receiver_leg.cpp
    spoil_receiver_load.cpp
    spoil_receiver_delivery.cpp
    spoil_receiver_stall.cpp
    spoil_receiver_accounting.cpp
    tbm_exchange_load.cpp
    tbm_exchange_lifecycle.cpp
    tbm_exchange_acceptance.cpp
    tbm_recovery.cpp
    movement_fixture.h
    tbm_movement.cpp
    portal_movement.cpp
    portal_recovery.cpp
    geology_deposits.cpp
    geology_generation.cpp
    geology_hardness.cpp
    tbm_geology.cpp
    geology_state.cpp
    geothermal_geology.cpp
    flatset_type.cpp
    history_func.cpp
    landscape_partial_pixel_z.cpp
    map_layer.cpp
    multilayer_content.cpp
    multilayer_content_identity.cpp
    multilayer_recipe.cpp
    layer_industry_admission.cpp
    layer_industry_contract.cpp
    layer_mine_production.cpp
    layer_mine_definition.cpp
    processor_fixture.h
    layer_processor.cpp
    layer_processor_definition.cpp
    layer_processor_presentation.cpp
    layer_processor_delivery.cpp
    processor_spoil_delivery.cpp
    layer_baseline_receiver.cpp
    layer_freight_accounting.cpp
    layer_surface_manufacturing.cpp
    layer_mine_presentation.cpp
    layer_mine_load.cpp
    layer_save_fixture.h
    layer_processor_load.cpp
    layer_industry_lifecycle.cpp
    layer_window_fixture.h
    layer_industry_window.cpp
    layer_mine_dispatch.cpp
    layer_mine_dispatch_pool.cpp
    layer_recipe_batch.cpp
    layer_recipe_work.cpp
    mine_fixture.h
    mine_production_fixture.h
    multilayer_bootstrap.cpp
    # Pure paid Surface order/actual station boundary: r2mlvs6-mines-and-resource-chains.md Group13C.1.
    surface_supply.cpp
    # Native non-paying purpose and original packet custody: r2mlvs6-mines-and-resource-chains.md Group13CB.
    surface_supply_custody.cpp
    # Detached bounded order/receipt validation: r2mlvs6-mines-and-resource-chains.md Group13CCB.
    surface_supply_order.cpp
    # Canonical native/legacy metadata: archi-map-layers.md; m2o23-1-come-space.md Group10.
    surface_supply_record_fixture.h
    surface_supply_receipts.cpp
    surface_supply_conversion.cpp
    surface_supply_packet_evidence.cpp
    surface_supply_departure.cpp
    surface_supply_refund_stock.cpp
    native_cargo_custody_fixture.h
    cargo_custody.cpp
    surface_supply_custody_policy.cpp
    surface_supply_legacy_fixture.h
    surface_supply_adoption.cpp
    surface_supply_space_custody.cpp
    surface_supply_window.cpp
    surface_supply_refund.cpp
    surface_supply_legacy_lifecycle.cpp
    cargo_partial_transfer.cpp
    surface_supply_cross_owner.cpp
    # Shared actual command-created component station: r2mlvs6-mines-and-resource-chains.md Group13CCC.
    surface_supply_fixture.h
    # Actual native restore/codec/copy references, no procurement: r2mlvs6-mines-and-resource-chains.md Group13CCC.
    surface_supply_order_owner.cpp
    surface_supply_order_wire.cpp
    surface_supply_merged_wire.cpp
    # Complete native file/preview boundaries over retained worlds: r2mlvs6-mines-and-resource-chains.md Group13CCC.
    surface_supply_order_native.cpp
    # Paid conventional carrier/collector/riser receipts: r2mlvs6 Group13CD.
    surface_supply_delivery.cpp
    # Real new-game company/factory/launch entry: r2mlvs11 Group15BB.
    surface_entry_native.cpp
    # Actual native borrowing, no fixture balance: r2mlvs11 Group15BC.
    surface_entry_credit.cpp
    # Original paid factory construction: r2mlvs11 Group15BC.
    surface_entry_factories.cpp
    # Genuine Surface freight infrastructure: r2mlvs11 Group15BD.
    surface_entry_roads.cpp
    # Actual conventional empty purchases/orders: r2mlvs11 Group15BD.
    surface_entry_fleet.cpp
    # Exact native game-tick replay of actual products: r2mlvs11 Group15BD.
    surface_entry_production.cpp
    # Pure genuine Salt/portal/spoil characterization: r2mlvs11 Group15BE.
    surface_entry_salt.cpp
    # Actual joint Surface spoil/rail/portal access: r2mlvs11 Group15BE.
    surface_entry_access.cpp
    # Genuine whole Salt mine site and retained cutter: r2mlvs11 Group15BF.
    surface_entry_mine.cpp
    # Genuine Salt mine capital and matching bank debt: r2mlvs11 Group15BG.
    surface_entry_salt_mine.cpp
    surface_entry_chemical.cpp
    surface_entry_fuel.cpp
    surface_entry_income.cpp
    # Genuine second Chemical works and physical Propellant: r2mlvs11 Group15BID.
    surface_entry_propellant.cpp
    # Actual paid launch-site/material/capital query: r2mlvs11 Group15BIE.
    surface_entry_launch.cpp
    # Original continuing freight replacement: r2mlvs11 Group15BIEE.
    surface_entry_renewal.cpp
    surface_entry_goods.cpp
    surface_entry_resupply.cpp
    surface_entry_fuel_refill.cpp
    surface_entry_gateway.cpp
    surface_entry_bus_renewal.cpp
    surface_entry_road_renewal.cpp
    # Actual regional catchments/paid roads/passenger profits: r2mlvs11 Group15BIEQA.
    surface_entry_regional.cpp
    multilayer_utility.cpp
    # Canonical tube registry boundary: docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md
    transport_tube_registry.cpp
    transport_tube_prototype.cpp
    # Physical source geometry: docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group10B.
    transport_tube_model.cpp
    transport_tube_definitions.cpp
    transport_tube_admission.cpp
    transport_tube_geothermal.cpp
    transport_tube_geothermal_binding.cpp
    transport_tube_commands.cpp
    # Native merged peer gate and exact supply codec: archi-commands.md; m2o23-3 Group47.
    merged_command_compatibility.cpp
    transport_tube_provider_switch.cpp
    utility_scheduler_publication.cpp
    utility_scheduler_contention.cpp
    utility_scheduler_lifecycle.cpp
    utility_scheduler_visibility.cpp
    utility_controls_window.cpp
    save_envelope_identity.cpp
    save_envelope_framing.cpp
    save_envelope_inspection.cpp
    save_envelope_originals.cpp
    save_envelope_adversarial.cpp
    # Descriptor-level CAPA columns and MLLN tuples for every merged-era writer: archi-savegame.md; m2o28-1 Group 11.
    save_envelope_cargo_columns.cpp
    # SSOR fixed/tagged/TABLE book shapes, overlaps and refusals: archi-savegame.md; m2o28-1 Group 12.
    save_envelope_supply_book.cpp
    save_lineage_preflight.cpp
    # Original native Space branch/receipt qualification: archi-savegame.md; m2o23-3 Group42.
    space_lineage_native.cpp
    machinery_lineage_native.cpp
    save_first_tick_fixture.cpp
    transport_tube_calendar.cpp
    transport_tube_fixture.h
    transport_tube_construction.cpp
    transport_tube_reservations.cpp
    # Native junction source geometry: docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group47D
    transport_tube_art_geometry.cpp
    # Pure imported native art selectors: archi-map-layers.md; m2o23-3 Group51.
    transport_tube_sprite.cpp
    transport_tube_vehicle_sprite.cpp
    transport_tube_facility_sprite.cpp
    transport_tube_conduit_sprite.cpp
    geothermal_generator_sprite.cpp
    geothermal_riser_sprite.cpp
    geothermal_conduit_sprite.cpp

    transport_tube_power_fixture.h
    transport_tube_power.cpp
    transport_tube_power_load.cpp
    transport_tube_meter.cpp
    transport_tube_meter_fixture.h
    transport_tube_recovery.cpp
    transport_tube_readiness.cpp
    # Native interchange source geometry: docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group48D.
    transport_tube_interchange_geometry.cpp
    # Bounded native packet/feeder transfer, not a second inventory: r2mlvs17 Group9.
    transport_tube_freight_fixture.h
    transport_tube_interchange.cpp
    transport_tube_interchange_delivery.cpp
    # Actual cold site admission: docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1.
    geothermal_site.cpp
    geothermal_site_load.cpp
    geothermal_definitions.cpp
    # Paid native generator construction/canonical delivery: archi-map-layers.md; m2o23-2 Group25.
    utility_heat_conversion.cpp
    utility_heat_conversion_draft.cpp
    utility_heat_conversion_fixture.h
    geothermal_generator_operating.cpp
    utility_power_local_feed.cpp
    geothermal_town_service.cpp
    geothermal_town_recovery.cpp
    geothermal_generator_operating_fixture.h
    geothermal_generator_lifecycle.cpp
    geothermal_generator.cpp
    geothermal_generator_magma.cpp
    geothermal_generator_delivery.cpp
    geothermal_generator_geometry.cpp
    geothermal_generator_fixture.h
    geothermal_collector_fixture.h
    geothermal_collector.cpp
    geothermal_heat.cpp
    geothermal_allocation.cpp
    geothermal_allocation_load.cpp
    geothermal_topology.cpp
    geothermal_heat_load.cpp
    geothermal_utility.cpp
    geothermal_electrical.cpp
    geothermal_power_export.cpp
    utility_power_lineage.cpp
    utility_power_transfer.cpp
    utility_power_path_load.cpp
    geothermal_electrical_fixture.h
    # Complete native portal-riser admission: r2mlvs18-geothermal-energy-and-industry.md Group5B.
    geothermal_riser.cpp
    geothermal_riser_definition.cpp
    geothermal_riser_fixture.h
    geothermal_riser_construction.cpp
    geothermal_riser_lifecycle.cpp
    geothermal_riser_delivery.cpp
    geothermal_riser_save.cpp
    geothermal_riser_link.cpp
    geothermal_riser_load.cpp
    geothermal_utility_load.cpp
    geothermal_collector_load.cpp
    # Declared connected ground fixture: r2mlvs16-integrated-acceptance-and-balance.md Group2.
    acceptance_ground_fixture.cpp
    # Shared thermal/electrical native owner: r2mlvs18-geothermal-energy-and-industry.md Group2B.
    utility_heat.cpp
    utility_heat_load.cpp
    utility_power.cpp
    utility_power_company.cpp
    utility_power_performance.cpp
    utility_power_load.cpp
    utility_power_mutation.cpp
    utility_power_transfer_load.cpp
    utility_power_effects.cpp
    utility_power_effect_load.cpp
    solar_power_contract.cpp
    multilayer_state.cpp
    portal_profile.cpp
    portal_build.cpp
    portal_date.cpp
    portal_fleet_date.cpp
    portal_occupied_save.cpp
    multilayer_art_geometry.cpp
    portal_lifecycle.cpp
    portal_work_state.cpp
    portal_window.cpp
    underground_window.cpp
    underground_job_presentation.cpp
    tbm_presentation.cpp
    underground_quote.cpp
    underground_area.cpp
    underground_workflow_fixture.h
    underground_view_fixture.h
    underground_workflow.cpp
    underground_filter.cpp
    underground_facility.cpp
    conventional_station_art.cpp
    # Actual bay/through command and drawing proof: r2mlvs14-remaining-assets-and-runtime-integration.md Group25B.
    conventional_terminal_art.cpp
    underground_engine.cpp
    underground_fleet.cpp
    underground_loading.cpp
    underground_project_label.cpp
    underground_admission.cpp
    underground_airport.cpp
    underground_command_boundary.cpp
    underground_save.cpp
    underground_facility_fixture.h
    underground_fleet_fixture.h
    portal_bootstrap.cpp
    math_func.cpp
    mock_environment.h
    mock_fontcache.h
    mock_spritecache.cpp
    mock_spritecache.h
    string_builder.cpp
    string_consumer.cpp
    string_inplace.cpp
    string_func.cpp
    test_main.cpp
    test_network_crypto.cpp
    test_script_admin.cpp
    test_window_desc.cpp
    tilearea.cpp
    utf8.cpp
)
