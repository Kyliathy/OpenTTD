# Focused content/geology/portal/TBM/mine/toolbar fixtures: docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md, r2mlvs2-geology-and-excavation.md, r2mlvs3-sealed-portals-and-deep-rail.md, r2mlvs4-tunnel-boring-machines.md, r2mlvs5-spoil-wagons-and-haulage.md, r2mlvs6-mines-and-resource-chains.md and r2mlvs7-underground-construction-and-fleet.md.
# Passive generator/site source boundaries: docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8B.
# Actual collector rendering: docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79C.
# Orbital Core activation and sole packet stock: docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1.
# Contained Core service and shared electrical settlement: docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1B.
# Conventional station source geometry: docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group24B.
# Native Surface processor geometry/state: docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group39B.
add_test_files(
    # Shared artifact ownership: docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Group 3.
    test_artifact_paths.h
    test_artifact_paths.cpp
    artifact_paths.cpp
    space_preview.cpp
    # Immutable class/size definitions and inclusive sampling: r2mlvs10-asteroids-and-mining-probes.md Group1.
    asteroid_catalogue.cpp
    # Bounded real-site starter selection and frozen inputs: r2mlvs10-asteroids-and-mining-probes.md Group2.
    asteroid_generation.cpp
    asteroid_generation_native.cpp
    # Integer trajectory, envelope and numerical budget fixtures: r2mlvs10-asteroids-and-mining-probes.md Group3.
    asteroid_motion.cpp
    asteroid_motion_native.cpp
    # Persistent depletion/invalid-target boundaries: r2mlvs10-asteroids-and-mining-probes.md Group5.
    asteroid_depletion.cpp
    asteroid_depletion_native.cpp
    # Shared body-strip/static-footprint separation: r2mlvs10-asteroids-and-mining-probes.md Group6.
    asteroid_band.cpp
    space_mining.cpp
    space_content.cpp
    space_orders.cpp
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
    mine_fixture.h
    mine_production_fixture.h
    multilayer_bootstrap.cpp
    multilayer_utility.cpp
    # Canonical tube registry boundary: docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md
    transport_tube_registry.cpp
    transport_tube_prototype.cpp
    # Physical source geometry: docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group10B.
    transport_tube_model.cpp
    transport_tube_fixture.h
    transport_tube_construction.cpp
    transport_tube_reservations.cpp
    # Native junction source geometry: docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group47D
    transport_tube_art_geometry.cpp
    transport_tube_power_fixture.h
    transport_tube_power.cpp
    transport_tube_power_load.cpp
    transport_tube_meter.cpp
    transport_tube_meter_fixture.h
    transport_tube_recovery.cpp
    transport_tube_readiness.cpp
    # Native interchange source geometry: docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group48D.
    transport_tube_interchange_geometry.cpp
    # Actual cold site admission: docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1.
    geothermal_site.cpp
    geothermal_site_load.cpp
    geothermal_definitions.cpp
    geothermal_generator_geometry.cpp
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
