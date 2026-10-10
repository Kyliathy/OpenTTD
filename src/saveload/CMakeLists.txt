# Metadata/geology/job transport: docs/zz-reach2/upgrades/2026-10/r2mlvs14-savegame-determinism-and-lifecycle.md and r2mlvs2-geology-and-excavation.md.
# Native electrical transport: docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group 7B.
add_subdirectory(compat)

add_files(
    afterload.cpp
    ai_sl.cpp
    airport_sl.cpp
    animated_tile_sl.cpp
    autoreplace_sl.cpp
    cargomonitor_sl.cpp
    cargopacket_sl.cpp
    cheat_sl.cpp
    company_sl.cpp
    depot_sl.cpp
    economy_sl.cpp
    engine_sl.cpp
    game_sl.cpp
    geology_sl.cpp
    excavation_sl.cpp
    layer_portal_work_sl.cpp
    tbm_sl.cpp
    spoil_receiver_sl.cpp
    layer_mine_sl.cpp
    layer_processor_sl.cpp
    tbm_exchange_sl.cpp
    utility_power_sl.cpp
    # Physical paid tube invoices: r2mlvs16-underground-transport-tubes.md Group5.
    transport_tube_power_sl.cpp
    transport_tube_vehicle_sl.cpp
    # Cold planning transport: r2mlvs17-geothermal-energy-and-industry.md Group1.
    geothermal_site_sl.cpp
    # Actual native construction transport: r2mlvs17-geothermal-energy-and-industry.md Group1B.
    geothermal_collector_sl.cpp
    # Original paid portal-bound machinery: r2mlvs17-geothermal-energy-and-industry.md Group5B.
    geothermal_riser_sl.cpp
    geothermal_heat_route_sl.cpp
    gamelog_sl.cpp
    goal_sl.cpp
    group_sl.cpp
    industry_sl.cpp
    labelmaps_sl.cpp
    league_sl.cpp
    linkgraph_sl.cpp
    map_sl.cpp
    misc_sl.cpp
    multilayer_sl.cpp
    newgrf_sl.cpp
    newgrf_sl.h
    object_sl.cpp
    oldloader.cpp
    oldloader.h
    oldloader_sl.cpp
    order_sl.cpp
    randomizer_sl.cpp
    saveload.cpp
    saveload.h
    saveload_filter.h
    saveload_func.h
    saveload_internal.h
    saveload_type.h
    settings_sl.cpp
    signs_sl.cpp
    station_sl.cpp
    storage_sl.cpp
    strings_sl.cpp
    story_sl.cpp
    subsidy_sl.cpp
    town_sl.cpp
    vehicle_sl.cpp
    waypoint_sl.cpp
    water_regions_sl.cpp
)
