# R2MLM4 - Layer World Lifecycle, Rules and Savegame

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
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 83-85 | `### AD-4 Independent layers`: each layer has its own rules, initialiser and void fence; Underground and Space initialise without `Random()`; the surface stays bit-identical. This plan implements the rules and initialisers. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 91-93 | `### AD-6 Every layer is simulated`: the shared LFSR cursor is applied to every plane; climate and flooding only where the rules allow. Governs Group 3. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 103-105 | `### AD-9 Savegame`: `SaveLoadVersion::MapLayers`, the `layers` field, all planes in the RIFF chunks, `AfterLoadGame` upgrade of older files. Governs Group 5. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 111-128 | `### AD-11`, `### AD-12` and `## Cross-plan Invariants`: this plan is where live games switch to three layers; I-6 (surface identical) and I-7 (round-trip and upgrade) are its acceptance tests. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 158-177 | `## Engineering Conventions` and `## Build and Test Commands`. |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | 431-442 | `## Invariants a change must not break`: big-endian chunk stream, version enum order, older versions keep loading, `MAPS` before the RIFF planes. Constrains every savegame task and the decision to add a field rather than a new chunk. |
| [src/map_layer.h](../../../../src/map_layer.h) | Live definitions; no fixed range | Produced by r2mlm2 Group 4: `MultiLayerMap` core. This plan adds `GetRules` and `InitialiseLayer` to the same struct; it must not duplicate any existing member. |

## Outcome and Boundaries

Exported by the end of this plan:

- `src/map_layer.h`: `enum class MapLayerGround { Natural, Bedrock, Vacuum }`, `enum class MapLayerConstruction` with `using MapLayerConstructions = EnumBitSet<...>`, `struct MapLayerRules`, `MultiLayerMap::GetRules(MapLayer)`, `MultiLayerMap::GetRules(TileIndex)`, `MultiLayerMap::InitialiseLayer(MapLayer)`, and the free function `CommandCost CheckLayerAllowsConstruction(TileIndex tile, MapLayerConstruction kind)`.
- `RunTileLoop` visiting every plane; climate, flooding and growth limited by the rules.
- `SaveLoadVersion::MapLayers`, the `MAPS` `layers` field, RIFF planes for all layers, and the `AfterLoadGame` upgrade.
- New games, scenario-editor worlds and loaded games allocate `MAX_MAP_LAYERS`.

Version-1 rules (AD-12): Surface allows everything. Underground allows rail, road (not tram), rail stations, road stops, depots, signals, signs and layer portals; it is flat bedrock at height 0 with no climate, water, trees, towns or industries. Space allows signs only; it is flat vacuum at height 0.

Not in this plan: drawing (r2mlm5 draws Bedrock and Vacuum and culls bands), portals (r2mlm7). Until r2mlm5 lands, switching the view still shows the surface.

## Implementation Tasks

### Area Mandatory Reading - Layer rules and initialisers

_Applies to Groups 1-2._

Entry gate. `src/map_layer.h` must export `MultiLayerMap::GetLayerBase`, `IterateLayer`, `TileInLayer` and `IsTileInLayer` (r2mlm2 Group 4), and the r2mlm3 ledger must have no `todo` row (r2mlm3 task 8.3). Verify with a grep for the four names and a search of the ledger for `todo`. If either check fails, stop every task from 2.1 onwards; Group 1 (rules table, error string, `CheckLayerAllowsConstruction`) may continue. Do not switch live games to three layers (Group 6) on an unmigrated tree, and do not compute plane bases with `Map::PlaneSize()` arithmetic as a substitute for the missing API.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/landscape.cpp](../../../../src/landscape.cpp) | 845-858 | `InitializeLandscape`: fills inner tiles with clear grass at height 0 and voids the south edges. Made surface-explicit by r2mlm3; this plan turns it into a loop over `InitialiseLayer`. |
| [src/landscape.cpp](../../../../src/landscape.cpp) | 545-557 | `DoClearSquare`: every demolition ends here with `MakeClear(tile, ClearGround::Grass, ...)`. Off the surface it must restore the layer's own ground instead of grass. |
| [src/clear_map.h](../../../../src/clear_map.h) | 247-265 | `MakeClear`: the constructor for clear tiles used by the initialiser (`ClearGround::Rocks`, density 3 for Bedrock and Vacuum). |
| [src/void_map.h](../../../../src/void_map.h) | 15-31 | `MakeVoid`: the fence constructor used for each plane's edges. |
| [src/genworld.cpp](../../../../src/genworld.cpp) | 94-157 | `_GenerateWorld` up to object and tree generation: the r2mlm3 marker comment shows where non-surface layers are confirmed initialised; no generator touches them. |
| [docs/zz-reach2/architecture/landscape/clear-tiles.md](../../architecture/landscape/clear-tiles.md) | 17-26 | ``## `m5`: ground, density, counter``: the clear-tile bits the initialiser writes. |

### Group 1 - Layer rules

{{MEDIUM}}
- [x] 1.1 Add `enum class MapLayerGround : uint8_t { Natural, Bedrock, Vacuum }` to `src/map_layer.h` with `///<` comments explaining how each is drawn (r2mlm5).
- [x] 1.2 Add `enum class MapLayerConstruction : uint8_t` with Rail, Road, Tram, RailStation, RoadStop, Depot, Signal, Bridge, Tunnel, Airport, Dock, Water, Terraform, Trees, Object, Town, Industry, House, Sign, LayerPortal, and `using MapLayerConstructions = EnumBitSet<MapLayerConstruction, uint32_t>`.
- [x] 1.3 Add `struct MapLayerRules` with `allowed`, `ground`, `base_height`, `has_climate`, `has_water_flow` and `has_ambient_sound`, each documented.
- [x] 1.4 Define the static rules table in `src/map_layer.cpp` for the three layers exactly as `## Outcome and Boundaries` lists, with a comment per row.
- [x] 1.5 Add `MultiLayerMap::GetRules(MapLayer)` and `MultiLayerMap::GetRules(TileIndex)`.
- [x] 1.6 Add strings `STR_ERROR_NOT_ALLOWED_IN_MAP_LAYER :{WHITE}... not possible in the {STRING} layer` and use the layer name from `GetMapLayerName`.
- [x] 1.7 Implement `CheckLayerAllowsConstruction(TileIndex tile, MapLayerConstruction kind)` returning an empty `CommandCost` or the error above; Javadoc both parameters.

### Group 2 - Plane initialisers

{{MEDIUM}}
- [x] 2.1 Implement `MultiLayerMap::InitialiseLayer(MapLayer layer)`: for Surface, run the existing body of `InitializeLandscape` on the surface plane; for other layers, fill inner tiles with `MakeClear(t, ClearGround::Rocks, 3)` at `base_height`, `TropicZone::Normal`, no bridge, then void the plane's south edges and, with `freeform_edges`, its north edges.
- [x] 2.2 Make `InitializeLandscape` call `InitialiseLayer` for every allocated layer and keep its Javadoc story (now per layer).
- [x] 2.3 Make `DoClearSquare` restore the layer default: `ClearGround::Grass` on the surface, `ClearGround::Rocks` density 3 elsewhere, with a comment.
- [x] 2.4 In Debug builds, assert in `InitialiseLayer` for non-surface layers that `_random` state is unchanged across the call (AD-4).
- [x] 2.5 Confirm in `_GenerateWorld`, at the r2mlm3 marker, that no generator writes outside the surface; replace the marker with a comment stating that non-surface planes come from `InitializeLandscape`.
- [x] 2.6 Add unit tests: after `Map::Allocate(64, 64, 3)` and `InitialiseLayer(MapLayer::Underground)`, every inner tile is clear rocks at the base height and each edge is void, with both `freeform_edges` values.

### Area Mandatory Reading - Per-layer simulation

_Applies to Group 3._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/landscape.cpp](../../../../src/landscape.cpp) | 800-843 | `RunTileLoop`: the Galois LFSR over one plane, tile 0 handled separately, `_cur_tileloop_tile` saved in the misc chunk. The same sequence is applied to each plane by adding the layer base (AD-6). |
| [src/clear_cmd.cpp](../../../../src/clear_cmd.cpp) | 292-349 | `TileLoop_Clear`: ambient sound, climate passes (`TileLoopClearDesert`, `TileLoopClearAlps`), grass growth and fields. All are skipped off the surface. |
| [src/rail_cmd.cpp](../../../../src/rail_cmd.cpp) | 2656-2779 | `TileLoop_Rail`: picks the rail ground type from climate and neighbours. Off the surface the ground is always `RailGroundType::Barren`. |
| [src/road_cmd.cpp](../../../../src/road_cmd.cpp) | 2020-2122 | `TileLoop_Road`: roadside changes, town paving, road works. Off the surface the roadside is barren and towns never act. |
| [src/void_cmd.cpp](../../../../src/void_cmd.cpp) | 49-54 | `TileLoop_Void`: floods the adjacent edge tile. Must not run off the surface (`has_water_flow`). |
| [src/water_cmd.cpp](../../../../src/water_cmd.cpp) | 1209-1275 | `DoFloodTile`: the flooding entry point that must never target a non-surface tile. |
| [src/station_cmd.cpp](../../../../src/station_cmd.cpp) | 3719-3786 | `TileLoop_Station`: snow and desert ground for stations, skipped off the surface. |

### Group 3 - Per-layer simulation

{{HARD}}
- [x] 3.1 Rewrite `RunTileLoop` to apply the same LFSR steps to every allocated plane by adding `MultiLayerMap::GetLayerBase(layer)`, including the separate tile-0 update per plane; keep one `_cur_tileloop_tile` (plane-local) and port every existing comment.
- [x] 3.2 `TileLoop_Clear`: skip ambient sound, climate passes and growth when the layer has no climate.
- [x] 3.3 `TileLoop_Rail` and `TileLoop_Road`: force barren ground off the surface and skip town and climate effects, with comments.
- [x] 3.4 `TileLoop_Void`, `TileLoop_Water` and `DoFloodTile`: return early off the surface (`has_water_flow`).
- [x] 3.5 `TileLoop_Station`: skip climate ground changes off the surface.
- [x] 3.6 Add Debug asserts that `TileLoop_Trees`, the house tile loop and `TileLoop_Industry` only ever see surface tiles.
- [ ] 3.7 Measure `PerformanceElement::GameLoopLandscape` with the framerate window on a 1024x1024 three-layer game before and after; record both numbers here.

### Area Mandatory Reading - Construction gating

_Applies to Group 4._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/commands/archi-commands.md](../../architecture/commands/archi-commands.md) | 122-150 | `## Test, then execute`: the proc runs in test mode first, so the layer check must be at the top of each proc and must not depend on `DoCommandFlag::Execute`. |
| [src/terraform_cmd.h](../../../../src/terraform_cmd.h) | 17-21 | `CmdTerraformLand`, `CmdLevelLand` and their traits (`CommandFlag::AllTiles`, `NoTest` on level land): the terraform entry points gated with `MapLayerConstruction::Terraform`. |
| [src/rail_cmd.h](../../../../src/rail_cmd.h) | 19-39 | Rail command declarations and traits: `CmdBuildSingleRail` is the per-tile proc behind `CmdBuildRailroadTrack`, so gating the single-tile procs covers the drag variants. The check goes right after parameter validation in each proc body. |
| [src/road_cmd.h](../../../../src/road_cmd.h) | 20-30 | Road command declarations and traits: `CmdBuildRoad` is the per-tile proc behind `CmdBuildLongRoad`; road versus tram is decided from the road type, so the check picks `Road` or `Tram`. |
| [src/station_cmd.h](../../../../src/station_cmd.h) | 29-33 | Declarations of `CmdBuildAirport`, `CmdBuildDock`, `CmdBuildRailStation`, `CmdBuildRoadStop`: the station entry points gated in task 4.3. |

### Group 4 - Construction gating

{{MEDIUM}}
- [x] 4.1 Gate `CmdTerraformLand` and `CmdLevelLand` with `MapLayerConstruction::Terraform`.
- [x] 4.2 Gate `CmdBuildSingleRail`, `CmdBuildTrainDepot`, `CmdBuildSingleSignal` and `CmdBuildRailWaypoint` with Rail, Depot, Signal and RailStation respectively.
- [x] 4.3 Gate `CmdBuildRailStation` (RailStation), `CmdBuildRoadStop` and `CmdBuildRoadWaypoint` (RoadStop), `CmdBuildAirport` (Airport) and `CmdBuildDock` (Dock).
- [x] 4.4 Gate `CmdBuildRoad` (Road or Tram by road type) and `CmdBuildRoadDepot` (Depot, plus Tram for tram depots).
- [x] 4.5 Gate `CmdBuildBridge` and `CmdBuildTunnel` on both end tiles.
- [x] 4.6 Gate `CmdBuildCanal`, `CmdBuildLock`, `CmdBuildShipDepot`, `CmdBuildBuoy` (Water or Dock), `CmdPlantTree` (Trees) and `CmdBuildObject` (Object).
- [x] 4.7 Gate `CmdBuildIndustry`, `CmdFoundTown`, `CmdPlaceHouse` and `CmdPlaceHouseArea` (Industry, Town, House).
- [x] 4.8 Record the gating matrix (command, `MapLayerConstruction` kind, allowed layers) as a list under this task. The in-game check that every gated command shows the layer error needs the Underground view and runs in r2mlm5 task 8.4.

Surface allows every kind. Underground allows Rail, Road, RailStation, RoadStop, Depot, Signal, Sign and LayerPortal. Space allows Sign. Signs are not gated, so they can be placed on every layer.

- `CmdTerraformLand`, `CmdLevelLand`: Terraform. Surface.
- `CmdBuildSingleRail`: Rail. Surface, Underground.
- `CmdBuildTrainDepot`: Depot. Surface, Underground.
- `CmdBuildSingleSignal`: Signal. Surface, Underground.
- `CmdBuildRailWaypoint`, `CmdBuildRailStation`: RailStation. Surface, Underground.
- `CmdBuildRoadStop`, `CmdBuildRoadWaypoint`: RoadStop. Surface, Underground.
- `CmdBuildAirport`: Airport. Surface.
- `CmdBuildDock`, `CmdBuildBuoy`: Dock. Surface.
- `CmdBuildRoad`: Road, or Tram when the road type is a tram. Road: Surface, Underground. Tram: Surface.
- `CmdBuildRoadDepot`: Depot, and also Tram for a tram depot. Road depot: Surface, Underground. Tram depot: Surface.
- `CmdBuildBridge`: Bridge, both ends. Surface.
- `CmdBuildTunnel`: Tunnel, both ends. Surface.
- `CmdBuildCanal`, `CmdBuildLock`, `CmdBuildShipDepot`: Water. Surface.
- `CmdPlantTree`: Trees. Surface.
- `CmdBuildObject`: Object. Surface.
- `CmdBuildIndustry`: Industry. Surface.
- `CmdFoundTown`: Town. Surface.
- `CmdPlaceHouse`, `CmdPlaceHouseArea`: House. Surface.

### Area Mandatory Reading - Savegame

_Applies to Group 5._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | 371-389 | `## Changing the format`: new field on an existing table chunk, version enumerator immediately before `MaxVersion`, cross-object repair in `AfterLoadGame`. The `layers` field follows the first recipe. |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | 167-177 | `### RIFF`: 28-bit length and the rule that `Load` consumes exactly the declared bytes. The plane loaders must read `PlaneSize * file_layers` tiles. |
| [src/saveload/saveload.h](../../../../src/saveload/saveload.h) | 419-429 | The tail of `enum class SaveLoadVersion`: `MapLayers` is inserted after `DoubleEndedShips` with the same comment shape. |
| [src/saveload/map_sl.cpp](../../../../src/saveload/map_sl.cpp) | 22-95 | `_map_desc`, `MAPSChunkHandler` (allocation on load, `LoadCheck`), `MAPTChunkHandler`: the pattern repeated by `MAPH` to `MAP8` (lines 97-366), with special cases in `MAP2` (`BigMap`) and `MAPE` (`BridgeWormhole`). |
| [src/saveload/afterload.cpp](../../../../src/saveload/afterload.cpp) | 558-564 | Start of `AfterLoadGame`: the missing planes are initialised here, before the first tile loop, because settings (`freeform_edges`) are loaded after the map chunks. |
| [src/fios.h](../../../../src/fios.h) | 37-80 | `struct LoadCheckData`: the preview record that gains a `map_layers` member next to `map_size_x` / `map_size_y`. |

### Group 5 - Savegame

{{HARD}}
- [x] 5.1 Insert `MapLayers, ///< Saveload version: 369 ...\n Map stores Surface, Underground and Space layers.` immediately before `MaxVersion` in `SaveLoadVersion`.
- [x] 5.2 Add `static uint8_t _map_layers` and a `SaveLoad::Variable<VarFileType::U8>("layers", SLE_GLOBAL_ADDRESS(_map_layers), SaveLoadVersion::MapLayers)` row to `_map_desc`; `Save` writes `Map::LayerCount()`, `Load` defaults `_map_layers` to 1 before reading.
- [x] 5.3 In `MAPSChunkHandler::Load`, allocate `Map::Allocate(_map_dim_x, _map_dim_y, MAX_MAP_LAYERS)` and keep the file's layer count in a file-scope `_map_file_layers` used by the plane loaders.
- [x] 5.4 Change every plane loader (`MAPT` to `MAP8`, including the `MAP2` 8-bit and `MAPE` packed variants) to read `Map::PlaneSize() * _map_file_layers` tiles, and every saver to write `Map::Size()` tiles; comment the 28-bit length limit.
- [x] 5.5 In `MAPSChunkHandler::LoadCheck`, fill a new `LoadCheckData::map_layers` member (default 1).
- [x] 5.6 At the start of `AfterLoadGame`, before the first tile loop, initialise every layer index at or above the file's layer count with `MultiLayerMap::InitialiseLayer`, with a comment explaining the ordering constraint.
- [x] 5.7 Make the old TTO / TTD loader allocate `MAX_MAP_LAYERS` (its reads are surface-only since r2mlm3) and rely on the same `AfterLoadGame` initialisation.
- [ ] 5.8 Build and run a round trip: new game, save, load, save again; digest every plane of both saves and confirm they are identical.

### Area Mandatory Reading - Going live and verification

_Applies to Groups 6-7._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/misc.cpp](../../../../src/misc.cpp) | 92-171 | `InitializeGame`: the allocation call for new games, editor worlds and the pre-load reset; switching it to `MAX_MAP_LAYERS` is what makes layers live. |
| [src/saveload/oldloader_sl.cpp](../../../../src/saveload/oldloader_sl.cpp) | 1527-1531 | TTO allocation inside `LoadOldMapPart1`, changed in task 5.7. |
| [docs/zz-reach2/architecture/desync/archi-desync.md](../../architecture/desync/archi-desync.md) | 166-172 | `### Cache checks at desync level 2`: `CheckCaches` runs every tick at `-d desync=2`; used as the determinism smoke test in task 6.5. |
| [docs/zz-reach2/upgrades/2026-10/tools/baseline-digests.txt](tools/baseline-digests.txt) | Live definitions; no fixed range | Produced by master task 1.8. Reference for I-6 in Group 7. |

### Group 6 - Three live layers

{{MEDIUM}}
- [x] 6.1 Change `InitializeGame` to allocate `MAX_MAP_LAYERS`, with a comment referencing AD-11.
- [x] 6.2 Log the allocated tile memory (bytes of both arrays) at `Facility::Map` debug level in `Map::Allocate`.
- [ ] 6.3 Start a new game, a scenario-editor flat world and a heightmap world; confirm three layers are allocated and the surface looks unchanged.
- [ ] 6.4 Confirm the layer button is now enabled (r2mlm2 task 5.3 logic) and still only changes its icon (rendering arrives in r2mlm5).
- [ ] 6.5 Run a 512x512 game for one in-game year with `-d desync=2` in a Debug build; confirm no cache-check messages.
- [ ] 6.6 Allocate a 4096x4096 three-layer map once and record peak memory here; if it exceeds 1.5 GiB, ask the user before continuing.

### Group 7 - Verification

{{SIMPLE}}
- [ ] 7.1 Build both configurations without new warnings; run `ctest` including the new rule and initialiser tests.
- [ ] 7.2 Regenerate the four baseline saves and compare the surface-plane digests (`--tiles` = one plane) with `baseline-digests.txt` (I-6).
- [ ] 7.3 Load each pre-epic baseline save, confirm three layers exist after load, play one in-game month, save, reload, and confirm all plane digests are stable (I-7).
- [ ] 7.4 Load a TTO save copied from `!kit/TT/` and confirm it gains Underground and Space and plays for one month.
- [ ] 7.5 Open the load dialog on a new save and confirm the preview still shows the map size correctly.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| `Invalid chunk size in RIFF` on load after Group 5 | [src/saveload/saveload.cpp](../../../../src/saveload/saveload.cpp) | `SlLoadChunk` | The size mismatch is not explained by `_map_file_layers` | Confirm how RIFF lengths are checked; `saveload.cpp` is the authority. |
| A pre-epic save fails in `AfterLoadGame` with a tile-type assert | [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | `## AfterLoadGame` (lines 351-369) | The assert is in a loop that runs before task 5.6's initialisation | Move the initialisation earlier or guard the loop; never skip initialisation. |
| Surface digests differ in task 7.2 | [r2mlm3-layer-call-site-migration.md](r2mlm3-layer-call-site-migration.md) | `## Call-site Ledger` | A C2 row touches generation code changed by this plan | Find which surface-only path now sees another layer. |
| The landscape tick becomes much slower in task 3.7 | [docs/zz-reach2/architecture/game-loop/archi-game-loop.md](../../architecture/game-loop/archi-game-loop.md) | `### One level into the calls` (lines 267-309) | The slowdown exceeds three times the surface-only time | Check what else runs per tile; decide with the user whether non-surface planes may tick at a lower rate. |
| A multiplayer join fails after Group 6 | [docs/zz-reach2/architecture/desync/archi-desync.md](../../architecture/desync/archi-desync.md) | `## Why a lockstep sim desyncs` (lines 16-44) | The failure is a desync rather than a load error | Diagnose join-state differences; acceptance belongs to r2mlm10. |

## Planning Provenance

This table records what shaped the plan. It is not an execution reading queue, it must not be preloaded after context compaction, it is non-transitive, and its line ranges record planning use rather than runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | 90-117, 167-177, 351-389, 431-442 | Field-on-table recipe, RIFF length rule, and the chunk order that forces initialisation into `AfterLoadGame`. |
| [src/saveload/map_sl.cpp](../../../../src/saveload/map_sl.cpp) | 22-382 | Showed every plane handler copies `Map::Size()` tiles, defining tasks 5.3 and 5.4. |
| [src/landscape.cpp](../../../../src/landscape.cpp) | 545-557, 800-858 | `DoClearSquare` grass default and the LFSR tile loop. |
| [src/void_cmd.cpp](../../../../src/void_cmd.cpp) | 49-64 | Void tiles flood their neighbours, which would flood the underground without a guard. |
| [src/clear_cmd.cpp](../../../../src/clear_cmd.cpp) | 142-187, 292-349 | Clear-tile drawing and tile loop, the basis for using `ClearGround::Rocks` for Bedrock and Vacuum. |
| Repository grep of command procs (`CmdBuild*`, `CmdPlantTree`, `CmdFoundTown`, `CmdPlaceHouse`, `CmdTerraformLand`) | Declarations in `*_cmd.h` | The gating list in Group 4. |

## Completion Conditions

- New games, editor worlds, loaded saves and TTO saves all have three layers.
- Underground and Space are flat rocks with their own void fences, initialised without `Random()`.
- The tile loop visits every plane; no climate, flooding or growth happens off the surface.
- Every construction command refuses what `MapLayerRules` forbids.
- `SaveLoadVersion::MapLayers` exists; saves round-trip every plane; pre-epic saves upgrade; surface digests match the baseline.
