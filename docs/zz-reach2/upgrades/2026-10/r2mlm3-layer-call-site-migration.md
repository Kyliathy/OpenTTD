# R2MLM3 - Layer Call-Site Migration

## Global Mandatory Reading

> **Execution protocol**
>
> - Read this Global Mandatory Reading section before the first task and again
>   after every context compaction.
> - Before starting or resuming a task group, read the nearest preceding Area
>   Mandatory Reading block whose `Applies to` label includes that group. Area
>   blocks appear immediately before the first group they govern.
> - A referenced heading or exported symbol is authoritative. Global navigation
>   hints and Area line ranges are for navigation only; if they drift, relocate
>   the named section or symbol rather than expanding to the whole file.
> - Mandatory reading is non-transitive. Do not preload a linked file's own
>   Mandatory Reading, Consult on Demand, or Planning Provenance entries unless
>   this plan names them directly for the current area.
> - Do not read Consult on Demand or Planning Provenance merely because those
>   sources are listed. Use a Consult on Demand source only when its trigger
>   occurs.
> - No created source file, test file or JSON file should start with same prefix as a planning MD file (such as r2). Only MD files can have that prefix. No MD file should be created anywhere in source directories unless specifically asked.
> - After EVERY CONTEXT COMPACTION, you must add a note above the task group
>   which is currently in execution. The note will say: "Context has compacted
>   X times while executing this group.". If the compaction occurs before any
>   meaningful work was done (this can happen when there is a lot of baggage
>   from the previous compaction), then and ONLY then you can skip this step.
> - When exceeding 3 context compactions, YOU MUST ask yourself if you're not 
>   chewing more than we can handle :). Heck, plans change! :). In such a case, 
>   the way out is to split the group on the spot,
>   in the plan, into more groups, 
>   by appending B, C, D as a suffixes to those groups (splitting sub-groups 
>   themselves is also allowed). When you do this, create Local Mandatory
>   Reading areas so you can optimize your context only for the
>   ACTUAL task at hand.
>   Before starting or resuming a task group, read the nearest preceding Area
>   Mandatory Reading block whose `Applies to` label includes that group. Area
>   blocks appear immediately before the first group they govern.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 65-88 | `### AD-2` to `### AD-5`: stacked `TileY`, the `Map` / `MultiLayerMap` split, independent surface-only content, plane distances and surface projection for towns. Every classification decision in this plan applies one of these rules. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 110-127 | `### AD-11 Behaviour-neutral staging` and `## Cross-plan Invariants`: the game still runs with one layer, so every migration must be a no-op at one layer (I-6) while being correct at three. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 141-170 | `## Engineering Conventions` and `## Build and Test Commands`. In particular: port existing comments when rewriting a loop, and add a business comment saying which layer rule applies. |
| [r2mlm3-layer-call-site-migration.md](r2mlm3-layer-call-site-migration.md) | 56-66 | `## Classification Rules` C1 to C9 of this plan: the single vocabulary used in the ledger and in code comments. |
| [src/map_layer.h](../../../../src/map_layer.h) | Live definitions; no fixed range | Produced by r2mlm2 Group 4. `MultiLayerMap::IterateLayer`, `TileInLayer`, `ProjectToSurface`, `RandomTileInLayer`, `GetLayerBase`, `IsTileInLayer` are the only allowed replacements; this plan adds no new layer API. |
| [src/map_func.h](../../../../src/map_func.h) | Live definitions; no fixed range | Extended by r2mlm2 Group 1. `TilePlaneY`, `GetTileLayer`, `TileXYL`, `Map::PlaneSize`, `Map::WrapToLayer` are authoritative; `Map::WrapToMap` is deprecated and removed by task 2.8. |

## Outcome and Boundaries

Every use of a map-geometry API outside `src/map_func.h`, `src/map.cpp`, `src/tile_map.h` and `src/map_layer.*` is classified in `## Call-site Ledger` and, where needed, migrated. At one layer the game behaves exactly as before; at three layers no code silently falls into the wrong layer.

Scope: `Map::Size`, `Map::Iterate`, `Map::SizeY`, `Map::MaxY`, `Map::LogY`, `RandomTile` / `RandomTileSeed`, `Map::WrapToMap`, `TileY` compared with plane bounds, `ClosestTownFromTile` and the town and station k-d trees.

Out of scope, recorded only: viewport, smallmap, screenshot and terraform GUI code (class C7, migrated by r2mlm5) and the map chunk handlers (class C8, migrated by r2mlm4).

## Classification Rules

- **C1 All-layer storage walk.** Conversions, repairs, recounts and ownership changes over every tile keep `Map::Iterate()`, which now covers every layer.
- **C2 Surface-only content.** Terrain generation, land counting, towns, houses, industries, trees, objects, water, disasters and heightmaps use `MultiLayerMap::IterateLayer(MapLayer::Surface)`, `RandomTile()` and `TileXYL(x, y, MapLayer::Surface)`.
- **C3 Plane geometry.** A `TileY` compared or clamped against `Map::SizeY()` / `Map::MaxY()`, or a neighbourhood loop bounded by them, uses `TilePlaneY` and rebuilds tiles with `TileXYL(x, y, GetTileLayer(origin))`.
- **C4 Plane quantity.** `Map::Size()` used as an amount of world (budgets, scaling, node limits) becomes `Map::PlaneSize()`; `Map::Size()` used as a validity bound stays.
- **C5 Town association.** Nearest town, town radius and local authority project the tile with `MultiLayerMap::ProjectToSurface` before querying.
- **C6 Same-layer spatial query.** Station k-d tree searches, catchment, `TileArea` expansion and clamping, spiral searches and station spread clamp within the origin tile's layer.
- **C7 View code.** Viewport, smallmap, screenshots and terraform GUI: recorded only, migrated by r2mlm5.
- **C8 Map chunks.** `src/saveload/map_sl.cpp`: recorded only, migrated by r2mlm4.
- **C9 External interface.** Script API, NewGRF variables, network and social-integration reports keep plane semantics (`Map::SizeY()` per plane is what they already expect); NewGRF nearby-tile offsets wrap inside the origin layer.

## Call-site Ledger

Filled by Group 1 and updated by every later group. One row per call site. `Status` is `todo`, `migrated`, `kept` (correct as is) or `deferred` (C7, C8).

| File:line | API | Class | Status | Note |
| --- | --- | --- | --- | --- |
| src/aircraft_cmd.cpp:549 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/aircraft_cmd.cpp:553 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/aircraft_cmd.cpp:714 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/aircraft_cmd.cpp:1199 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/cheat_gui.cpp:151 | Map::Iterate() | C1 | kept | A height anywhere in storage blocks the limit change. |
| src/clear_cmd.cpp:490 | RandomTileSeed | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/clear_cmd.cpp:498 | RandomTileSeed | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/command_func.h:172 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/command_func.h:329 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/company_cmd.cpp:385 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/company_cmd.cpp:485 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/console_cmds.cpp:386 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/console_cmds.cpp:400 | Map::SizeY() | C9 | kept | Scripts, NewGRF and network reports stay on one plane. |
| src/console_cmds.cpp:1395 | Map::LogY() | C9 | kept | Scripts, NewGRF and network reports stay on one plane. |
| src/console_cmds.cpp:1418 | Map::LogY() | C9 | kept | Scripts, NewGRF and network reports stay on one plane. |
| src/disaster_vehicle.cpp:207 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/disaster_vehicle.cpp:211 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/disaster_vehicle.cpp:265 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/disaster_vehicle.cpp:344 | RandomTile | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/disaster_vehicle.cpp:427 | IterateLayer | C2 | migrated | Surface content, written explicitly. |
| src/disaster_vehicle.cpp:580 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/disaster_vehicle.cpp:601 | RandomTile | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/disaster_vehicle.cpp:760 | RandomTile | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/disaster_vehicle.cpp:784 | RandomTile | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/disaster_vehicle.cpp:786 | TileXYL, Map::SizeY() | C2 | migrated | Surface content, written explicitly. |
| src/disaster_vehicle.cpp:853 | RandomTile | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/disaster_vehicle.cpp:857 | TileXYL, Map::SizeY() | C2 | migrated | Surface content, written explicitly. |
| src/disaster_vehicle.cpp:876 | RandomTileSeed | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/disaster_vehicle.cpp:879 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/disaster_vehicle.cpp:1023 | RandomTile | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/economy.cpp:480 | Map::Iterate() | C1 | kept | Ownership, label or cache walk over every stored tile. |
| src/economy.cpp:489 | Map::Iterate() | C1 | kept | Ownership, label or cache walk over every stored tile. |
| src/economy.cpp:884 | Map::LogY() | C1 | kept | Ownership, label or cache walk over every stored tile. |
| src/effectvehicle.cpp:625 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/genworld.cpp:131 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/genworld.cpp:132 | Map::SizeY(), TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/genworld.cpp:349 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/genworld.cpp:377 | TileXYL, Map::MaxY() | C2 | migrated | Surface content, written explicitly. |
| src/genworld.cpp:382 | TileXYL, Map::MaxY() | C2 | migrated | Surface content, written explicitly. |
| src/heightmap.cpp:345 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/heightmap.cpp:348 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/heightmap.cpp:365 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/heightmap.cpp:366 | Map::SizeY(), TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/heightmap.cpp:374 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/heightmap.cpp:375 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/heightmap.cpp:432 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/heightmap.cpp:440 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/heightmap.cpp:443 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/heightmap.cpp:444 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/heightmap.cpp:449 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/heightmap.cpp:468 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/heightmap.cpp:472 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/heightmap.cpp:473 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/heightmap.cpp:478 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/heightmap.cpp:567 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/heightmap.cpp:569 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/industry_cmd.cpp:1038 | WrapToLayer | C2 | migrated | Surface content, written explicitly. |
| src/industry_cmd.cpp:1076 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/industry_cmd.cpp:1088 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/industry_cmd.cpp:1302 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/industry_cmd.cpp:1600 | TilePlaneY | C3 | migrated | The edge or neighbour is measured inside the origin layer. |
| src/industry_cmd.cpp:1615 | TilePlaneY | C3 | migrated | The edge or neighbour is measured inside the origin layer. |
| src/industry_cmd.cpp:1648 | TilePlaneY | C3 | migrated | The edge or neighbour is measured inside the origin layer. |
| src/industry_cmd.cpp:1657 | TilePlaneY, Map::MaxY() | C3 | migrated | The edge or neighbour is measured inside the origin layer. |
| src/industry_cmd.cpp:2119 | RandomTile | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/industry_cmd.cpp:2403 | RandomTile | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/industry_gui.cpp:622 | IterateLayer | C2 | migrated | Surface content, written explicitly. |
| src/landscape.cpp:136 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/landscape.cpp:137 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/landscape.cpp:337 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/landscape.cpp:339 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/landscape.cpp:677 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/landscape.cpp:679 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/landscape.cpp:684 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/landscape.cpp:686 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/landscape.cpp:763 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/landscape.cpp:831 | Map::LogY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/landscape.cpp:835 | Map::LogY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/landscape.cpp:891 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/landscape.cpp:909 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/landscape.cpp:911 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/landscape.cpp:917 | PlaneSize() | C2 | migrated | Surface content, written explicitly. |
| src/landscape.cpp:926 | PlaneSize() | C2 | migrated | Surface content, written explicitly. |
| src/landscape.cpp:937 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/landscape.cpp:940 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/landscape.cpp:1009 | PlaneSize() | C2 | migrated | Surface content, written explicitly. |
| src/landscape.cpp:1011 | IterateLayer | C2 | migrated | Surface content, written explicitly. |
| src/landscape.cpp:1031 | IterateLayer | C2 | migrated | Surface content, written explicitly. |
| src/landscape.cpp:1435 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/landscape.cpp:1503 | RandomTile | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/landscape.cpp:1518 | RandomTile | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/landscape.cpp:1563 | IterateLayer | C2 | migrated | Surface content, written explicitly. |
| src/landscape.cpp:1579 | PlaneSize() | C2 | migrated | Surface content, written explicitly. |
| src/landscape.cpp:1663 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/landscape.cpp:1664 | Map::SizeY(), TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/linkgraph/demands.cpp:271 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/misc_gui.cpp:133 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/misc_gui.cpp:184 | TilePlaneY | C2 | migrated | Surface content, written explicitly. |
| src/network/core/network_game_info.cpp:152 | Map::SizeY() | C9 | kept | Scripts, NewGRF and network reports stay on one plane. |
| src/network/network_admin.cpp:191 | Map::SizeY() | C9 | kept | Scripts, NewGRF and network reports stay on one plane. |
| src/network/network_client.cpp:906 | Map::SizeY() | C9 | kept | Scripts, NewGRF and network reports stay on one plane. |
| src/newgrf/newgrf_actd.cpp:100 | Map::LogY() | C9 | kept | Scripts, NewGRF and network reports stay on one plane. |
| src/newgrf_airport.cpp:112 | Map::SizeY() | C9 | kept | Scripts, NewGRF and network reports stay on one plane. |
| src/newgrf_airport.cpp:236 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_airporttiles.cpp:171 | GetTownRadiusGroup, ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_commons.cpp:419 | WrapToLayer | C9 | migrated | Scripts, NewGRF and network reports stay on one plane. |
| src/newgrf_house.cpp:352 | GetTownRadiusGroup | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_house.cpp:399 | WrapToLayer | C9 | migrated | Scripts, NewGRF and network reports stay on one plane. |
| src/newgrf_industries.cpp:189 | GetTownRadiusGroup | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_industries.cpp:297 | GetTownRadiusGroup | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_industries.cpp:499 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_industries.cpp:554 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_industrytiles.cpp:71 | GetTownRadiusGroup, ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_object.cpp:283 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_object.cpp:336 | GetTownRadiusGroup | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_object.cpp:407 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_railtype.cpp:58 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_railtype.cpp:60 | GetTownRadiusGroup | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_railtype.cpp:213 | Map::Iterate() | C1 | kept | Ownership, label or cache walk over every stored tile. |
| src/newgrf_roadstop.cpp:115 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_roadstop.cpp:116 | GetTownRadiusGroup | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_roadstop.cpp:122 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_roadstop.cpp:264 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_roadtype.cpp:99 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_roadtype.cpp:101 | GetTownRadiusGroup | C5 | kept | Town association. The projection lives in the town query functions. |
| src/newgrf_roadtype.cpp:245 | Map::Iterate() | C1 | kept | Ownership, label or cache walk over every stored tile. |
| src/newgrf_station.cpp:277 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/object_cmd.cpp:96 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/object_cmd.cpp:262 | TilePlaneY, Map::MaxY() | C3 | migrated | The edge or neighbour is measured inside the origin layer. |
| src/object_cmd.cpp:407 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/object_cmd.cpp:831 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/object_cmd.cpp:845 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/object_cmd.cpp:846 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/object_cmd.cpp:847 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/object_cmd.cpp:848 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/object_cmd.cpp:877 | RandomTile | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/object_cmd.cpp:901 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/object_cmd.cpp:902 | TileXYL, Map::MaxY() | C2 | migrated | Surface content, written explicitly. |
| src/object_cmd.cpp:905 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/object_cmd.cpp:906 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/object_cmd.cpp:907 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/object_cmd.cpp:924 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/object_cmd.cpp:947 | RandomTile | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/openttd.cpp:997 | Map::SizeY() | C9 | kept | Scripts, NewGRF and network reports stay on one plane. |
| src/openttd.cpp:999 | Map::SizeY() | C9 | kept | Scripts, NewGRF and network reports stay on one plane. |
| src/openttd.cpp:1004 | Map::SizeY() | C9 | kept | Scripts, NewGRF and network reports stay on one plane. |
| src/pathfinder/water_regions.cpp:38 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/pathfinder/yapf/yapf_ship_regions.cpp:190 | PlaneSize() | C2 | migrated | Surface content, written explicitly. |
| src/rail_cmd.cpp:894 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/rail_cmd.cpp:1291 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/rail_cmd.cpp:1576 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/road_cmd.cpp:279 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/road_cmd.cpp:463 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/road_cmd.cpp:620 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/road_cmd.cpp:977 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/road_cmd.cpp:1076 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/road_cmd.cpp:1816 | Map::Size() | C4 | kept | Reviewed as a quantity or a storage bound. |
| src/road_cmd.cpp:1968 | Map::Iterate() | C1 | kept | Ownership, label or cache walk over every stored tile. |
| src/road_cmd.cpp:1972 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/road_cmd.cpp:2066 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/road_cmd.cpp:2071 | GetTownRadiusGroup | C5 | kept | Town association. The projection lives in the town query functions. |
| src/road_cmd.cpp:2507 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/road_cmd.cpp:2556 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/saveload/afterload.cpp:106 | TilePlaneY, Map::MaxY() | C3 | migrated | The edge or neighbour is measured inside the origin layer. |
| src/saveload/afterload.cpp:163 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:209 | TileXYL, Map::MaxY() | C2 | migrated | Surface content, written explicitly. |
| src/saveload/afterload.cpp:210 | Map::SizeY(), TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/saveload/afterload.cpp:620 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:637 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:664 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:861 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:878 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:934 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:941 | Map::Iterate() | C1 | kept | Runs for every save. Stations exist on every layer, so it walks every plane (FD-9). |
| src/saveload/afterload.cpp:1011 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1015 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/saveload/afterload.cpp:1022 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/saveload/afterload.cpp:1067 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1097 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1137 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1164 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/saveload/afterload.cpp:1202 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1233 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1248 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/saveload/afterload.cpp:1315 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1359 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1413 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1497 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1529 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1563 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1577 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1658 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1660 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/saveload/afterload.cpp:1718 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1746 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1763 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1806 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1868 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1902 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1924 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1933 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:1936 | TilePlaneY, Map::MaxY() | C3 | migrated | The edge or neighbour is measured inside the origin layer. |
| src/saveload/afterload.cpp:1980 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:2001 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:2025 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:2108 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:2120 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:2136 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:2165 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/saveload/afterload.cpp:2291 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:2303 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:2435 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:2480 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:2519 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:2566 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:2605 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:2914 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:2983 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:3018 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:3043 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:3236 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:3243 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:3338 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:3346 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/afterload.cpp:3417 | Map::Iterate() | C1 | migrated | FD-9: a version-gated conversion reads a one-plane file, so it walks the surface only (r2rvm task 14.5). |
| src/saveload/company_sl.cpp:141 | Map::Iterate() | C1 | kept | Ownership, label or cache walk over every stored tile. |
| src/saveload/map_sl.cpp:42 | PlaneSize() | C8 | deferred | Map chunk. Migrated by r2mlm4 Group 5. |
| src/saveload/map_sl.cpp:53 | Map::SizeY() | C8 | deferred | Map chunk. Migrated by r2mlm4 Group 5. |
| src/saveload/map_sl.cpp:110 | Map::Size() | C8 | deferred | Map chunk. Migrated by r2mlm4 Group 5. |
| src/saveload/map_sl.cpp:137 | Map::Size() | C8 | deferred | Map chunk. Migrated by r2mlm4 Group 5. |
| src/saveload/map_sl.cpp:164 | Map::Size() | C8 | deferred | Map chunk. Migrated by r2mlm4 Group 5. |
| src/saveload/map_sl.cpp:194 | Map::Size() | C8 | deferred | Map chunk. Migrated by r2mlm4 Group 5. |
| src/saveload/map_sl.cpp:221 | Map::Size() | C8 | deferred | Map chunk. Migrated by r2mlm4 Group 5. |
| src/saveload/map_sl.cpp:248 | Map::Size() | C8 | deferred | Map chunk. Migrated by r2mlm4 Group 5. |
| src/saveload/map_sl.cpp:275 | Map::Size() | C8 | deferred | Map chunk. Migrated by r2mlm4 Group 5. |
| src/saveload/map_sl.cpp:316 | Map::Size() | C8 | deferred | Map chunk. Migrated by r2mlm4 Group 5. |
| src/saveload/map_sl.cpp:343 | Map::Size() | C8 | deferred | Map chunk. Migrated by r2mlm4 Group 5. |
| src/saveload/map_sl.cpp:370 | Map::Size() | C8 | deferred | Map chunk. Migrated by r2mlm4 Group 5. |
| src/saveload/oldloader_sl.cpp:58 | IterateLayer | C2 | migrated | Surface content, written explicitly. |
| src/saveload/oldloader_sl.cpp:218 | IterateLayer | C2 | migrated | Surface content, written explicitly. |
| src/saveload/oldloader_sl.cpp:510 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/saveload/oldloader_sl.cpp:555 | PlaneSize() | C2 | migrated | Surface content, written explicitly. |
| src/saveload/oldloader_sl.cpp:556 | PlaneSize() | C2 | migrated | Surface content, written explicitly. |
| src/saveload/oldloader_sl.cpp:562 | PlaneSize() | C2 | migrated | Surface content, written explicitly. |
| src/saveload/oldloader_sl.cpp:568 | TileXYL, Map::MaxY(), PlaneSize() | C2 | migrated | Surface content, written explicitly. |
| src/saveload/oldloader_sl.cpp:1541 | IterateLayer | C2 | migrated | Surface content, written explicitly. |
| src/saveload/oldloader_sl.cpp:1544 | IterateLayer | C2 | migrated | Surface content, written explicitly. |
| src/saveload/oldloader_sl.cpp:1550 | IterateLayer | C2 | migrated | Surface content, written explicitly. |
| src/saveload/oldloader_sl.cpp:1554 | IterateLayer | C2 | migrated | Surface content, written explicitly. |
| src/saveload/oldloader_sl.cpp:1566 | IterateLayer | C2 | migrated | Surface content, written explicitly. |
| src/saveload/oldloader_sl.cpp:1569 | IterateLayer | C2 | migrated | Surface content, written explicitly. |
| src/saveload/station_sl.cpp:138 | Map::Iterate() | C1 | kept | Ownership, label or cache walk over every stored tile. |
| src/saveload/town_sl.cpp:38 | Map::Iterate() | C1 | kept | Ownership, label or cache walk over every stored tile. |
| src/saveload/town_sl.cpp:66 | Map::Iterate() | C1 | kept | Ownership, label or cache walk over every stored tile. |
| src/saveload/town_sl.cpp:79 | Map::Iterate() | C1 | kept | Ownership, label or cache walk over every stored tile. |
| src/saveload/waypoint_sl.cpp:106 | Map::Iterate() | C1 | kept | Ownership, label or cache walk over every stored tile. |
| src/saveload/waypoint_sl.cpp:213 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/screenshot.cpp:234 | TileXYL | C7 | migrated | View code. Migrated by r2mlm5 Group 7. |
| src/screenshot.cpp:235 | TileXYL, Map::MaxY() | C7 | migrated | View code. Migrated by r2mlm5 Group 7. |
| src/screenshot.cpp:348 | IterateLayer | C7 | migrated | View code. Migrated by r2mlm5 Group 7. |
| src/screenshot.cpp:353 | Map::SizeY() | C7 | migrated | View code. Migrated by r2mlm5 Group 7. |
| src/screenshot.cpp:379 | Map::SizeY() | C7 | migrated | View code. Migrated by r2mlm5 Group 7. |
| src/screenshot.cpp:502 | TileXYL | C7 | migrated | View code. Migrated by r2mlm5 Group 7. |
| src/screenshot.cpp:524 | Map::SizeY() | C7 | migrated | View code. Migrated by r2mlm5 Group 7. |
| src/script/api/script_map.cpp:20 | IsTileInLayer | C9 | migrated | Scripts, NewGRF and network reports stay on one plane. |
| src/script/api/script_map.cpp:26 | PlaneSize() | C9 | migrated | Scripts, NewGRF and network reports stay on one plane. |
| src/script/api/script_map.cpp:36 | Map::SizeY() | C9 | kept | Scripts, NewGRF and network reports stay on one plane. |
| src/script/api/script_map.cpp:48 | TilePlaneY | C9 | migrated | Scripts, NewGRF and network reports stay on one plane. |
| src/script/api/script_object.hpp:429 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/script/api/script_tile.cpp:317 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/script/api/script_tile.cpp:327 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/script/api/script_tilelist.cpp:116 | TilePlaneY | C9 | migrated | Scripts, NewGRF and network reports stay on one plane. |
| src/script/api/script_tilelist.cpp:119 | Map::MaxY() | C9 | kept | Scripts, NewGRF and network reports stay on one plane. |
| src/script/api/script_tilelist.cpp:122 | TileXYL | C9 | migrated | Scripts, NewGRF and network reports stay on one plane. |
| src/settings_table.cpp:499 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/settings_table.cpp:505 | TileXYL, Map::MaxY() | C2 | migrated | Surface content, written explicitly. |
| src/settings_table.cpp:510 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/settings_table.cpp:511 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/settings_table.cpp:516 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/settings_table.cpp:517 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/settings_table.cpp:532 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/settings_table.cpp:533 | Map::SizeY(), TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/settings_table.cpp:537 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/settings_table.cpp:538 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/settings_table.cpp:540 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/settings_table.cpp:541 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/settings_table.cpp:542 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/settings_table.cpp:572 | Map::Iterate() | C1 | kept | A height anywhere in storage blocks the limit change. |
| src/smallmap_gui.cpp:872 | Map::MaxY() | C7 | migrated | View code. Migrated by r2mlm5 Group 6. |
| src/smallmap_gui.cpp:873 | Map::MaxY() | C7 | migrated | View code. Migrated by r2mlm5 Group 6. |
| src/smallmap_gui.cpp:895 | Map::SizeY() | C7 | migrated | View code. Migrated by r2mlm5 Group 6. |
| src/smallmap_gui.cpp:933 | Map::MaxY() | C7 | migrated | View code. Migrated by r2mlm5 Group 6. |
| src/smallmap_gui.cpp:944 | TileXYL | C7 | migrated | View code. Migrated by r2mlm5 Group 6. |
| src/smallmap_gui.cpp:946 | TileXYL | C7 | migrated | View code. Migrated by r2mlm5 Group 6. |
| src/smallmap_gui.cpp:975 | Map::SizeY() | C7 | migrated | View code. Migrated by r2mlm5 Group 6. |
| src/smallmap_gui.cpp:1512 | Map::SizeY() | C7 | migrated | View code. Migrated by r2mlm5 Group 6. |
| src/smallmap_gui.cpp:1532 | TilePlaneY | C7 | migrated | View code. Migrated by r2mlm5 Group 6. |
| src/smallmap_gui.cpp:1738 | Map::SizeY() | C7 | migrated | View code. Migrated by r2mlm5 Group 6. |
| src/sound.cpp:231 | Map::MaxY(), TilePlaneY | C3 | migrated | The edge or neighbour is measured inside the origin layer. |
| src/station.cpp:40 | _station_kdtree | C6 | kept | The search stays in the origin tile's layer. |
| src/station.cpp:48 | _station_kdtree | C6 | kept | The search stays in the origin tile's layer. |
| src/station.cpp:161 | _station_kdtree | C6 | kept | The search stays in the origin tile's layer. |
| src/station_cmd.cpp:477 | _station_kdtree | C6 | kept | The search stays in the origin tile's layer. |
| src/station_cmd.cpp:481 | _station_kdtree | C6 | kept | The search stays in the origin tile's layer. |
| src/station_cmd.cpp:730 | _station_kdtree | C6 | kept | The search stays in the origin tile's layer. |
| src/station_cmd.cpp:732 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/station_cmd.cpp:1857 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/station_cmd.cpp:1890 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/station_cmd.cpp:2459 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/station_cmd.cpp:2507 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/station_cmd.cpp:2575 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/station_cmd.cpp:2677 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/station_cmd.cpp:3781 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/station_cmd.cpp:3783 | GetTownRadiusGroup | C5 | kept | Town association. The projection lives in the town query functions. |
| src/station_cmd.cpp:4699 | TilePlaneY | C6 | migrated | The search stays in the origin tile's layer. |
| src/station_cmd.cpp:4707 | Map::SizeY() | C6 | kept | The search stays in the origin tile's layer. |
| src/station_cmd.cpp:4710 | TileXYL | C6 | migrated | The search stays in the origin tile's layer. |
| src/station_cmd.cpp:4724 | _station_kdtree | C6 | kept | The search stays in the origin tile's layer. |
| src/station_cmd.cpp:4725 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/station_gui.cpp:86 | Map::Size() | C6 | kept | The search stays in the origin tile's layer. |
| src/station_gui.cpp:136 | TilePlaneY | C6 | migrated | The search stays in the origin tile's layer. |
| src/station_gui.cpp:141 | TileXYL, Map::MaxY() | C6 | migrated | The search stays in the origin tile's layer. |
| src/station_kdtree.h:26 | _station_kdtree | C6 | kept | The search stays in the origin tile's layer. |
| src/station_kdtree.h:38 | TilePlaneY | C6 | migrated | The search stays in the origin tile's layer. |
| src/station_kdtree.h:40 | Map::SizeY() | C6 | kept | The search stays in the origin tile's layer. |
| src/station_kdtree.h:46 | TileXYL | C6 | migrated | The search stays in the origin tile's layer. |
| src/station_kdtree.h:47 | Map::SizeY() | C6 | kept | The search stays in the origin tile's layer. |
| src/station_kdtree.h:52 | Map::Size() | C6 | kept | The search stays in the origin tile's layer. |
| src/station_kdtree.h:54 | TileXYL | C6 | migrated | The search stays in the origin tile's layer. |
| src/station_kdtree.h:57 | _station_kdtree | C6 | kept | The search stays in the origin tile's layer. |
| src/terraform_cmd.cpp:105 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/terraform_cmd.cpp:121 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/terraform_cmd.cpp:183 | Map::Size() | C4 | kept | Reviewed as a quantity or a storage bound. |
| src/terraform_cmd.cpp:190 | Map::Size() | C4 | kept | Reviewed as a quantity or a storage bound. |
| src/terraform_cmd.cpp:197 | Map::Size() | C4 | kept | Reviewed as a quantity or a storage bound. |
| src/terraform_cmd.cpp:216 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/terraform_cmd.cpp:332 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/terraform_gui.cpp:222 | TilePlaneY, Map::MaxY() | C7 | migrated | View code. Migrated by r2mlm5 Group 3. |
| src/terraform_gui.cpp:388 | TilePlaneY, Map::MaxY() | C7 | migrated | View code. Migrated by r2mlm5 Group 3. |
| src/tgp.cpp:167 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/tgp.cpp:256 | Map::LogY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/tgp.cpp:343 | Map::SizeY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/tgp.cpp:380 | Map::LogY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/tgp.cpp:1073 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/tgp.cpp:1074 | Map::SizeY(), TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/tgp.cpp:1082 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:59 | TilePlaneY | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:62 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/tile_map.cpp:65 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:66 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:67 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:100 | TilePlaneY | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:103 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/tile_map.cpp:106 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:107 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:108 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:123 | TilePlaneY | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:126 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/tile_map.cpp:130 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:131 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:132 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:145 | TilePlaneY | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:148 | Map::MaxY() | C3 | kept | SizeY, MaxY and LogY are already one plane. |
| src/tile_map.cpp:152 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:153 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/tile_map.cpp:154 | TileXYL | C2 | migrated | Surface content, written explicitly. |
| src/tilearea_diagonal.cpp:23 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/tilearea_diagonal.cpp:24 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/tilearea_diagonal.cpp:127 | Map::SizeY(), TileXYL | C6 | migrated | The search stays in the origin tile's layer. |
| src/tilearea_diagonal.cpp:128 | Map::Size() | C6 | kept | The search stays in the origin tile's layer. |
| src/tilearea_diagonal.h:67 | TilePlaneY | C6 | migrated | The search stays in the origin tile's layer. |
| src/tilearea_orthogonal.cpp:23 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/tilearea_orthogonal.cpp:24 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/tilearea_orthogonal.cpp:122 | TilePlaneY | C6 | migrated | The search stays in the origin tile's layer. |
| src/tilearea_orthogonal.cpp:128 | Map::SizeY() | C6 | kept | The search stays in the origin tile's layer. |
| src/tilearea_orthogonal.cpp:130 | TileXYL | C6 | migrated | The search stays in the origin tile's layer. |
| src/tilearea_orthogonal.cpp:141 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/tilearea_orthogonal.cpp:144 | Map::SizeY(), TilePlaneY | C6 | migrated | The search stays in the origin tile's layer. |
| src/tilearea_spiral.cpp:29 | TilePlaneY | C6 | migrated | The search stays in the origin tile's layer. |
| src/tilearea_spiral.cpp:37 | TilePlaneY | C6 | migrated | The search stays in the origin tile's layer. |
| src/tilearea_spiral.cpp:48 | TilePlaneY | C6 | migrated | The search stays in the origin tile's layer. |
| src/tilearea_spiral.cpp:61 | Map::SizeY() | C6 | kept | The search stays in the origin tile's layer. |
| src/tilearea_spiral.h:57 | TileXYL | C6 | migrated | The search stays in the origin tile's layer. |
| src/town.h:292 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town.h:316 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town.h:318 | GetTownRadiusGroup | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town.h:340 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:81 | _town_kdtree | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:89 | _town_kdtree | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:141 | Map::Iterate() | C1 | kept | Walks every stored tile. |
| src/town_cmd.cpp:187 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:376 | _town_kdtree | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:377 | _town_kdtree | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:1094 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/town_cmd.cpp:1104 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/town_cmd.cpp:1514 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/town_cmd.cpp:1776 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/town_cmd.cpp:2027 | _town_kdtree | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:2370 | RandomTile | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/town_cmd.cpp:2521 | GetTownRadiusGroup | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:2524 | ProjectToSurface | C5 | migrated | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:2841 | GetTownRadiusGroup | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:3012 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:3033 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/town_cmd.cpp:3399 | Map::Iterate() | C1 | kept | Walks every stored tile. |
| src/town_cmd.cpp:3407 | Map::Iterate() | C1 | kept | Walks every stored tile. |
| src/town_cmd.cpp:3451 | _town_kdtree | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:3998 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:4014 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:4019 | ProjectToSurface | C5 | migrated | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:4020 | _town_kdtree | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:4034 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:4038 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:4045 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:4054 | ProjectToSurface | C5 | migrated | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:4064 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/town_cmd.cpp:4242 | IterateLayer | C2 | migrated | Surface content, written explicitly. |
| src/town_kdtree.h:24 | _town_kdtree | C5 | kept | Town association. The projection lives in the town query functions. |
| src/train_cmd.cpp:1855 | Map::Size() | C4 | kept | Reviewed as a quantity or a storage bound. |
| src/train_cmd.cpp:1864 | Map::Size() | C4 | kept | Reviewed as a quantity or a storage bound. |
| src/train_cmd.cpp:1879 | Map::Size() | C4 | kept | Reviewed as a quantity or a storage bound. |
| src/train_cmd.cpp:1895 | Map::Size() | C4 | kept | Reviewed as a quantity or a storage bound. |
| src/train_cmd.cpp:1901 | Map::Size() | C4 | kept | Reviewed as a quantity or a storage bound. |
| src/train_cmd.cpp:1906 | Map::Size() | C4 | kept | Reviewed as a quantity or a storage bound. |
| src/tree_cmd.cpp:314 | RandomTile | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/tree_cmd.cpp:383 | RandomTileSeed | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/tree_cmd.cpp:414 | RandomTileSeed | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/tree_cmd.cpp:524 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/tree_cmd.cpp:615 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/tree_cmd.cpp:742 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/tree_cmd.cpp:1001 | RandomTileSeed | C2 | kept | RandomTileSeed already wraps onto the surface. |
| src/tunnel_map.cpp:71 | TilePlaneY, Map::MaxY() | C3 | migrated | The edge or neighbour is measured inside the origin layer. |
| src/tunnelbridge_cmd.cpp:345 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/tunnelbridge_cmd.cpp:437 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/tunnelbridge_cmd.cpp:669 | CalcClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/tunnelbridge_cmd.cpp:885 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/tunnelbridge_cmd.cpp:966 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/viewport.cpp:1120 | ClosestTownFromTile | C7 | migrated | View code. Migrated by r2mlm5 Groups 2 and 3. |
| src/viewport.cpp:1271 | Map::SizeY() | C7 | migrated | View code. Migrated by r2mlm5 Groups 2 and 3. |
| src/viewport.cpp:1272 | Map::SizeY() | C7 | migrated | View code. Migrated by r2mlm5 Groups 2 and 3. |
| src/viewport.cpp:1319 | Map::SizeY() | C7 | migrated | View code. Migrated by r2mlm5 Groups 2 and 3. |
| src/viewport.cpp:2188 | Map::SizeY() | C7 | migrated | View code. Migrated by r2mlm5 Groups 2 and 3. |
| src/viewport.cpp:2214 | Map::SizeY() | C7 | migrated | View code. Migrated by r2mlm5 Groups 2 and 3. |
| src/viewport.cpp:2296 | Map::MaxY() | C7 | migrated | View code. Migrated by r2mlm5 Groups 2 and 3. |
| src/viewport.cpp:2611 | Map::SizeY() | C7 | migrated | View code. Migrated by r2mlm5 Groups 2 and 3. |
| src/viewport.cpp:3208 | Map::MaxY() | C7 | migrated | View code. Migrated by r2mlm5 Groups 2 and 3. |
| src/viewport.cpp:3209 | Map::MaxY() | C7 | migrated | View code. Migrated by r2mlm5 Groups 2 and 3. |
| src/viewport.cpp:3243 | Map::MaxY() | C7 | migrated | View code. Migrated by r2mlm5 Groups 2 and 3. |
| src/viewport.cpp:3244 | Map::MaxY() | C7 | migrated | View code. Migrated by r2mlm5 Groups 2 and 3. |
| src/viewport.cpp:3661 | Map::SizeY() | C7 | migrated | View code. Migrated by r2mlm5 Groups 2 and 3. |
| src/viewport.cpp:3662 | Map::SizeY() | C7 | migrated | View code. Migrated by r2mlm5 Groups 2 and 3. |
| src/water_cmd.cpp:484 | TilePlaneY, Map::MaxY() | C3 | migrated | The edge or neighbour is measured inside the origin layer. |
| src/water_cmd.cpp:517 | Map::Size() | C4 | kept | Validity bound. Map::Size() covers every layer. |
| src/water_cmd.cpp:609 | TilePlaneY, Map::MaxY() | C3 | migrated | The edge or neighbour is measured inside the origin layer. |
| src/water_cmd.cpp:632 | ClosestTownFromTile | C5 | kept | Town association. The projection lives in the town query functions. |
| src/water_cmd.cpp:1405 | IterateLayer | C2 | migrated | Surface content, written explicitly. |

Counts: C1 69, C2 107, C3 58, C4 42, C5 82, C6 33, C7 33, C8 12, C9 20. Total 456.

## Implementation Tasks

### Area Mandatory Reading - Ledger and core helpers

_Applies to Groups 1-2._

Entry gate. Before Group 1 starts, `src/map_layer.h` must declare `MultiLayerMap::IterateLayer`, `TileInLayer`, `ProjectToSurface` and `RandomTileInLayer`, and `src/map_func.h` must declare `TilePlaneY`, `GetTileLayer`, `TileXYL`, `Map::PlaneSize` and `Map::WrapToLayer`. Verify with a grep for each name. If any is missing, stop every task in this plan and finish r2mlm2 Groups 1 and 4 first. Group 1 inventory tasks 1.1 to 1.3 may still run. Do not compute layer bases with `Map::PlaneSize()` arithmetic or local helper functions as a substitute.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/tile_map.cpp](../../../../src/tile_map.cpp) | 50-148 | `GetTileSlopeZ`, `IsTileFlat`, `GetTileZ`, `GetTileMaxZ`: each clamps the neighbour row with `std::min(y1 + 1, Map::MaxY())`, which would pull a lower-layer neighbour into the surface. Class C3. `GetTilePixelSlopeOutsideMap` in the same range is view code (C7, r2mlm5). |
| [src/tilearea_orthogonal.cpp](../../../../src/tilearea_orthogonal.cpp) | 113-142 | `OrthogonalTileArea::Expand` and `ClampToMap`: clamp Y to `Map::SizeY()`. Class C6, must clamp inside the area's layer. |
| [src/tilearea_spiral.cpp](../../../../src/tilearea_spiral.cpp) | 19-63 | `SpiralTileIterator` constructors and `SkipOutsideMap`: the `y >= Map::SizeY()` test skips every tile of a lower layer. Class C6. |
| [src/tilearea_diagonal.cpp](../../../../src/tilearea_diagonal.cpp) | 84-132 | `DiagonalTileArea::iterator::operator++`: rebuilds tiles with `TileXY` and an `y >= Map::SizeY()` test. Class C6. |
| [src/station_kdtree.h](../../../../src/station_kdtree.h) | 27-45 | `ForAllStationsRadius`: clamps the search box to `Map::SizeY()`. Class C6. |
| [src/newgrf_commons.cpp](../../../../src/newgrf_commons.cpp) | 397-419 | `GetNearbyTile`: `Map::WrapToMap(tile + TileDiffXY(x, y))`. Class C9: wrap inside the origin layer with `Map::WrapToLayer`. |

### Group 1 - Ledger inventory

{{SIMPLE}}
- [x] 1.1 Grep `src/` for `Map::Size()`, `Map::Iterate()`, `Map::SizeY()`, `Map::MaxY()`, `Map::LogY()`, `RandomTile`, `RandomTileSeed`, `Map::WrapToMap`, excluding `src/map_func.h`, `src/map.cpp`, `src/tile_map.h`, `src/map_layer.*` and `src/tests/`; paste one ledger row per hit with status `todo`.
- [x] 1.2 Grep for `TileY(` within 3 lines of `SizeY` or `MaxY`, and for `for (uint y` / `for (int y` loops bounded by `Map::SizeY()` or `Map::MaxY()`; add any hit not yet in the ledger.
- [x] 1.3 Grep for `ClosestTownFromTile`, `CalcClosestTownFromTile`, `_town_kdtree`, `_station_kdtree`, `GetTownRadiusGroup`; add their call sites with a provisional class.
- [x] 1.4 Assign a class C1 to C9 to every row, using `## Classification Rules`; rows that fit no class are escalated to the user before Group 2.
- [x] 1.5 Mark every C7 and C8 row `deferred` with a one-line note for r2mlm5 or r2mlm4.
- [x] 1.6 Sort the ledger by file and record the total count per class under the ledger table.

### Group 2 - Core spatial helpers

{{MEDIUM}}
- [x] 2.1 `src/tile_map.cpp`: in `GetTileSlopeZ`, `IsTileFlat`, `GetTileZ` and `GetTileMaxZ`, clamp neighbour rows with the plane Y and rebuild neighbours with `TileXYL` in the tile's layer (C3).
- [x] 2.2 `src/tilearea_orthogonal.cpp`: make `Expand` and `ClampToMap` clamp within the layer of `this->tile` (C6); keep the asserts on `Map::Size()`.
- [x] 2.3 `src/tilearea_spiral.cpp`: store the plane Y and the layer of the start tile, test `y` against `Map::SizeY()` in the plane, and yield tiles with `TileXYL` (C6).
- [x] 2.4 `src/tilearea_diagonal.cpp`: same treatment for the iterator (C6).
- [x] 2.5 `src/station_kdtree.h`: clamp `ForAllStationsRadius` to the band of the centre's layer and skip stations in other layers (C6).
- [x] 2.6 `src/tunnel_map.cpp` (`IsTunnelInWay`): compare `TilePlaneY(tile)` with `Map::MaxY() / 2` (C3).
- [x] 2.7 `src/newgrf_commons.cpp`, `src/newgrf_house.cpp` and `src/industry_cmd.cpp`: replace `Map::WrapToMap` with `Map::WrapToLayer(..., GetTileLayer(origin))` (C9 / C2).
- [x] 2.8 Remove the deprecated `Map::WrapToMap` definition from `src/map_func.h` (an edit, not a file deletion), build, and mark these ledger rows `migrated`. `WrapToMap` is gone and the rows are migrated. Both builds pass on 2026-10-04 (task 8.4).

### Area Mandatory Reading - World generation

_Applies to Group 3._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/genworld.cpp](../../../../src/genworld.cpp) | 94-232 | `_GenerateWorld`: the generation order (landscape, clear tiles, `CountLandTiles`, towns, industries, objects, trees, tile-loop warm-up). r2mlm4 later inserts non-surface initialisation here; this group only makes each step surface-explicit. |
| [src/landscape.cpp](../../../../src/landscape.cpp) | 845-858 | `InitializeLandscape`: fills the inner rectangle and voids the south edges using `Map::MaxX` / `Map::MaxY` loops. Class C2 for now (surface plane); r2mlm4 generalises it per layer. |
| [src/landscape.cpp](../../../../src/landscape.cpp) | 1618-1719 | `GenerateLandscape`: the generator dispatch (original, TGP, heightmap) and the freeform north-edge voiding. Class C2. |
| [docs/zz-reach2/architecture/landscape/archi-landscape.md](../../architecture/landscape/archi-landscape.md) | 135-159 | `## Map size, void tiles, and terraforming`: the four places that void the north edges (`landscape.cpp`, `tgp.cpp`, `heightmap.cpp`, `genworld.cpp`); all four must stay surface-only. |

### Group 3 - Surface-only world generation

{{MEDIUM}}
- [x] 3.1 `src/landscape.cpp`: make `InitializeLandscape`, `GenerateTerrain`, the river and desert or rainforest passes, `GenerateLandscape` and the freeform voiding iterate `MultiLayerMap::IterateLayer(MapLayer::Surface)` or build tiles with `TileXYL(..., MapLayer::Surface)` (C2).
- [x] 3.2 `src/tgp.cpp` and `src/heightmap.cpp`: make every `TileXY` write and every `Map::SizeY()` / `MaxY()` loop explicitly surface (C2).
- [x] 3.3 `src/genworld.cpp`: make the freeform voiding loops surface-explicit and add a comment marking where r2mlm4 inserts non-surface initialisation.
- [x] 3.4 `src/clear_cmd.cpp` (`GenerateClearTile`) and `src/tree_cmd.cpp`: confirm `RandomTile()` is surface and convert `Map::Size()` budgets to `Map::PlaneSize()` (C2, C4).
- [x] 3.5 `src/industry_cmd.cpp` and `src/object_cmd.cpp`: convert placement loops, `Map::MaxY()` bounds and budgets to surface plane semantics (C2, C3, C4).
- [x] 3.6 `src/water_cmd.cpp` (`ConvertGroundTilesIntoWaterTiles` and the flood checks): surface-only (C2).
- [x] 3.7 `src/disaster_vehicle.cpp`: target tiles, start positions and `Map::MaxY()` bounds use the surface plane (C2, C3).
- [x] 3.8 Build, regenerate the four baseline saves, and diff digests against `baseline-digests.txt`; any difference blocks Group 4. All ten Surface prefixes match for all four final worlds; R2RVM2 task 18.1 and [tools/final-layer-save-digests.txt](tools/final-layer-save-digests.txt).

### Area Mandatory Reading - Savegame conversions

_Applies to Group 4._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | 351-369 | `## AfterLoadGame`: version-gated conversions over the loaded image. Explains why most loops are C1: they repair whatever tiles exist, and old files only contain surface tiles. |
| [src/saveload/afterload.cpp](../../../../src/saveload/afterload.cpp) | 558-564 | Start of `AfterLoadGame`, including the `_cur_tileloop_tile` zero fix that stays valid per plane (AD-6). |
| [src/saveload/oldloader_sl.cpp](../../../../src/saveload/oldloader_sl.cpp) | 544-565 | TTDPatch header tiles at `Map::Size() - 3`, `- 2`, `- 1` and the `ClearOldMap3` loop from `TileXY(0, Map::MaxY())`: these mean "end of the surface plane" and become `Map::PlaneSize()` (C4, C3). |
| [src/saveload/oldloader_sl.cpp](../../../../src/saveload/oldloader_sl.cpp) | 1527-1566 | `LoadOldMapPart1` and `LoadOldMapPart2`: allocation for TTO files and the loops that read one byte per tile straight from the file. The file holds one 256x256 plane, so these loops must iterate the surface plane only (C2), or a three-layer allocation would read past the data. |

### Group 4 - Savegame conversion loops

Closure, 2026-10-04: task 4.6's complete operator play matrix is skipped under the goal's operator allowance. Its example `trt00.sv0` is actually TTD and passes 31 days of Debug play plus complete four-plane round trips (R2RVM2 Group 18B). Actual TTO `trt16.sv1` fails at old custom-name conversion in both the pre-epic and final builds; that existing compatibility gap is recorded in the master's Deferred Points. Modern baseline upgrades/paused round trips pass task 18.1, but a full month of Debug play for the modern row is not inferred.

{{HARD}}
- [x] 4.1 Walk the 52 `Map::Iterate()` loops in `src/saveload/afterload.cpp`; for each, record in the ledger whether it is C1 (keep) or uses `TileY` against plane bounds (C3, migrate).
- [x] 4.2 Migrate every C3 loop in `afterload.cpp`, porting the existing comments and adding the class tag in a `//` comment.
- [x] 4.3 `src/saveload/oldloader_sl.cpp`: convert the TTDPatch header tile indices and the `ClearOldMap3` loop to plane semantics (C4, C3).
- [x] 4.4 `src/saveload/oldloader_sl.cpp`: convert the read loops in `LoadOldMapPart1` / `LoadOldMapPart2` and the other whole-map loops to `MultiLayerMap::IterateLayer(MapLayer::Surface)` (C2), so a later three-layer allocation still reads exactly one plane from the file.
- [x] 4.5 `src/saveload/company_sl.cpp`, `station_sl.cpp`, `town_sl.cpp`, `waypoint_sl.cpp`: classify their loops (expected C1) and record them.
- [s] 4.6 Load a TTO save copied from `!kit/TT/` (for example `trt00.sv0`) and a modern baseline save in a Debug build; confirm both load and play for one in-game month.

### Area Mandatory Reading - Towns, stations, commands and interfaces

_Applies to Groups 5-7._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/town_cmd.cpp](../../../../src/town_cmd.cpp) | 3990-4047 | `CalcClosestTownFromTile` and `ClosestTownFromTile`: the k-d tree nearest query and the ownership shortcuts for roads and depots. Class C5: project before the query. |
| [src/town_kdtree.h](../../../../src/town_kdtree.h) | 16-23 | `Kdtree_TownXYFunc`: keys towns by `TileX` / `TileY`; towns are surface-only, so queries must be projected rather than the tree changed. |
| [src/script/api/script_map.cpp](../../../../src/script/api/script_map.cpp) | 16-46 | `ScriptMap::IsValidTile`, `GetMapSize`, `GetMapSizeX`, `GetMapSizeY`, `GetTileX`, `GetTileY`: the script-visible map. Class C9: scripts see the surface plane only in version 1 (AD-12). |
| [src/pathfinder/water_regions.cpp](../../../../src/pathfinder/water_regions.cpp) | 31-41 | Water-region coordinate helpers using `TileY`: water regions exist for the surface plane only, so non-surface tiles must be ignored before indexing. |
| [src/command_func.h](../../../../src/command_func.h) | 167-193 | `Command<Tcmd>::Do` tile bound check `tile >= Map::Size()`: a validity bound (C4 keep) that now admits every layer, which is intended (AD-2). |

### Group 5 - Town and station association

{{MEDIUM}}
- [x] 5.1 `CalcClosestTownFromTile` and `ClosestTownFromTile`: project with `MultiLayerMap::ProjectToSurface` before the k-d tree query and before the distance threshold test (C5).
- [x] 5.2 `GetTownRadiusGroup`, local-authority lookups and town rating checks in `src/town_cmd.cpp`: project the tile (C5).
- [x] 5.3 The remaining `Map::Iterate()` and `Map::Size()` uses in `src/town_cmd.cpp` (town growth, house counts, `ScaleByLandProportion` budget): classify; growth and house loops are C2.
- [x] 5.4 `src/station_cmd.cpp`: station spread, `GetStationAround`, rectangle checks and the three `Map::Size()` uses: classify; spatial searches stay in the origin layer (C6).
- [x] 5.5 `src/station_gui.cpp`: the join-station search and `Map::MaxY()` bound use the origin layer (C6).
- [x] 5.6 Record every change in the ledger and build. The ledger records the changes; both builds pass on 2026-10-04 (task 8.4).

### Group 6 - Commands, economy and vehicles

{{MEDIUM}}
- [x] 6.1 `src/rail_cmd.cpp`, `src/road_cmd.cpp`, `src/train_cmd.cpp`, `src/terraform_cmd.cpp`: classify the `Map::Size()` uses (expected C4 keep as bounds) and the single `Map::Iterate()` in `road_cmd.cpp`.
- [x] 6.2 `src/economy.cpp`: the two company-removal loops are C1 (every layer must be cleaned on bankruptcy); confirm and record. Classify the `Map::MaxY()` use.
- [x] 6.3 `src/cheat_gui.cpp`, `src/industry_gui.cpp`: classify (the land-count refresh is C2).
- [x] 6.4 `src/settings_table.cpp` and `src/console_cmds.cpp`: map-size settings and console reports are plane (C9 / C3).
- [x] 6.5 `src/aircraft_cmd.cpp`, `src/effectvehicle.cpp`, `src/sound.cpp`: clamps against `Map::MaxY()` use the plane of the vehicle or tile (C3); aircraft stay on the surface (AD-12).
- [x] 6.6 `src/openttd.cpp`, `src/network/network_client.cpp`: social-integration map size reports are plane (C9); record.

### Group 7 - External interfaces and surface-only subsystems

{{MEDIUM}}
- [x] 7.1 `src/linkgraph/demands.cpp`: the `base_distance` scale from `TileXY(0, 0)` to `TileXY(Map::MaxX(), Map::MaxY())` is a plane quantity (C4); confirm it is unchanged by plane distances.
- [x] 7.2 `src/newgrf_railtype.cpp`, `src/newgrf_roadtype.cpp`: label-map loops are C1.
- [x] 7.3 `src/newgrf/newgrf_actd.cpp`, `src/newgrf_airport.cpp`: map-size variables and airport bounds are plane (C9, C3).
- [x] 7.4 `src/network/core/network_game_info.cpp`, `src/network/network_admin.cpp`: reported map height is the plane height (C9).
- [x] 7.5 `src/script/api/script_map.cpp`, `script_tilelist.cpp`, `script_object.hpp`: `GetMapSize` returns `Map::PlaneSize()`, `ScriptMap::IsValidTile` also requires `MultiLayerMap::IsTileInLayer(t, MapLayer::Surface)`, `GetMapSizeY` stays per plane, and the tile-list bound uses the plane (C9, C4). Scripts therefore see exactly the old single map.
- [x] 7.6 `src/pathfinder/water_regions.cpp`: return early from `InvalidateWaterRegion` and the other public entry points for non-surface tiles, with a comment that ships are surface-only (AD-12).
- [x] 7.7 `src/pathfinder/yapf/yapf_ship_regions.cpp`: the node limit uses `Map::PlaneSize()` (C4).

### Area Mandatory Reading - Deferred records and verification

_Applies to Group 8._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 1195-1320 | `GetViewportY` and `ViewportAddLandscape`: the main C7 site, recorded only; its migration is r2mlm5 Group 2. |
| [src/saveload/map_sl.cpp](../../../../src/saveload/map_sl.cpp) | 22-95 | `_map_desc`, `MAPSChunkHandler`, `MAPTChunkHandler`: the C8 pattern repeated by every plane chunk; recorded only, migrated by r2mlm4 Group 5. |
| [docs/zz-reach2/upgrades/2026-10/tools/baseline-digests.txt](tools/baseline-digests.txt) | Live definitions; no fixed range | Produced by r2rvm task 2.5 (master plan `## Baseline Data`). The neutrality reference for task 8.6. |

### Group 8 - Deferred records and verification

Closure, 2026-10-04: task 8.7's year-long player-route operator run is skipped under the goal's operator allowance. The supplied-save Debug smoke and corrected station/portal round trips are recorded in R2MLM13 Group 9C; those shorter checks do not count as a year-long pass. Surface neutrality is now verified below.

{{SIMPLE}}
- [x] 8.1 Confirm every hit in `src/viewport.cpp`, `src/smallmap_gui.cpp`, `src/screenshot.cpp` and `src/terraform_gui.cpp` has a `deferred` C7 row with a note naming the r2mlm5 group that will migrate it.
- [x] 8.2 Confirm every hit in `src/saveload/map_sl.cpp` has a `deferred` C8 row naming r2mlm4 Group 5.
- [x] 8.3 Re-run the Group 1 greps; every hit must have a ledger row whose status is not `todo`.
- [x] 8.4 Build both configurations without new warnings; run `ctest`.

  Verified 2026-10-04 against the integrated tree: both targets/configurations build with exit 0 and zero compiler warnings; both unit runs pass 110/110. Evidence: `build/build-portal-rail-conversion-{Debug,RelWithDebInfo}.log`, `build/ctest-portal-rail-conversion-{Debug,RelWithDebInfo}.log`. Surface digest and runtime checks remain open; this does not recreate the earlier one-layer staging build.
- [x] 8.5 Add to `src/tests/map_layer.cpp` tests for the migrated helpers on a three-layer map: spiral search, `OrthogonalTileArea::Expand`, `GetTileSlopeZ` neighbours and `ProjectToSurface` all stay in the expected layer.
- [x] 8.6 Regenerate the four baseline saves and compare digests with `baseline-digests.txt`; they must match (I-6). All four final worlds match every Surface chunk, R2RVM2 task 18.1; full digest lines are in [tools/final-layer-save-digests.txt](tools/final-layer-save-digests.txt).
- [s] 8.7 Play a new game for one in-game year at fast forward in a Debug build with towns, industries and a few player routes; confirm no assert fires.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| A ledger row fits none of C1 to C9 | [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | `## Architecture Decisions` (lines 59-116) | The decision still depends on gameplay intent | Find the AD that governs the case; if none does, ask the user and record the answer as a new class. |
| An `afterload.cpp` loop's purpose is unclear in task 4.1 | [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | `## Changing the format` (lines 371-389) | The loop's version gate does not explain the repair | Understand whether the loop repairs bytes or rebuilds caches; the code remains the authority. |
| Digests differ in task 3.8 or 9.4 | [docs/zz-reach2/architecture/landscape/archi-landscape.md](../../architecture/landscape/archi-landscape.md) | `## Dispatch` (lines 183-207) | The difference is in tile types rather than heights | Locate which generator step changed tile content. |
| A town or station behaves oddly at the map's south edge after Group 5 | [src/town_cmd.cpp](../../../../src/town_cmd.cpp) | `GetTownRadiusGroup` | The issue involves `TileY` clamps | Check for a missed C3 site. |
| A NewGRF industry or house misplaces sprites after task 2.7 | [docs/zz-reach2/architecture/newgrf/archi-newgrf.md](../../architecture/newgrf/archi-newgrf.md) | `## Callback resolution (industry production)` (lines 155-184) | The variable involved is a nearby-tile query | Confirm how nearby-tile offsets are consumed. |

## Planning Provenance

This table records what shaped the plan. It is not an execution reading queue, it must not be preloaded after context compaction, it is non-transitive, and its line ranges record planning use rather than runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| Repository grep of `Map::Iterate`, `Map::Size`, `Map::SizeY`, `Map::MaxY`, `RandomTile` (per-file counts) | `src/` | Sized the work (86 iterate uses in 19 files, 96 `Size` uses, 117 `SizeY` / `MaxY` uses, 23 random-tile uses) and grouped files into Groups 2 to 8. |
| [src/tile_map.cpp](../../../../src/tile_map.cpp) | 50-148 | Found the `std::min(y1 + 1, Map::MaxY())` clamp that silently pulls a lower layer into the surface. |
| [src/tilearea_orthogonal.cpp](../../../../src/tilearea_orthogonal.cpp), [src/tilearea_spiral.cpp](../../../../src/tilearea_spiral.cpp) | 113-142, 19-63 | Showed spatial helpers that would skip every lower-layer tile, motivating class C6. |
| [src/saveload/oldloader_sl.cpp](../../../../src/saveload/oldloader_sl.cpp) | 544-565, 1527-1566 | `Map::Size() - 3` header tiles and per-tile file reads that assume one plane. |
| [src/town_cmd.cpp](../../../../src/town_cmd.cpp) | 3990-4047 | Nearest-town logic behind class C5. |
| [src/pathfinder/water_regions.cpp](../../../../src/pathfinder/water_regions.cpp) | 31-41 | Water-region indexing by `TileY`, which needs a surface guard. |

## Completion Conditions

Closed 2026-10-04 under the goal's operator allowance, with skips at Groups 4 and 8. Code/ledger and Surface digest checks are complete; the original one-layer live staging has been superseded. Final builds pass 111/111 unit cases in both configurations and 115/115 full release CTest cases (`build/ctest-console-input-*.log`, `build/ctest-console-final-RelWithDebInfo.log`). Skipped play scenarios remain future acceptance work, not passing evidence.

- `## Call-site Ledger` lists every grep hit from Group 1 with a class and a non-`todo` status.
- No migrated code computes a layer base by hand; all replacements use the r2mlm2 API.
- `Map::WrapToMap` no longer exists.
- Surface digests match `baseline-digests.txt`, and the new helper tests pass on a three-layer allocation.
