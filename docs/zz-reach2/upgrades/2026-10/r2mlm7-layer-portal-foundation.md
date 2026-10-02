# R2MLM7 - Layer Portal Foundation

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
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 99-101 | `### AD-8 Cross-layer movement only through layer portals`: the pair model, door side and shaft side, drive-through traversal, the single neighbour function and the no-wormhole movement rule. This plan owns that contract; r2mlm8 and r2mlm9 only consume it. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 87-89 | `### AD-5 Distances are plane distances`: YAPF estimates must use plane coordinates or a portal makes the heuristic inconsistent. Group 3 enforces it in `OctileDistanceCost`. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 119-128 | `## Cross-plan Invariants`, in particular I-3 (only the portal neighbour function crosses layers) and I-8 (every end has a reversed pair). |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 157-174 | `## Engineering Conventions` and `## Build and Test Commands`. |
| [r2mlm7-layer-portal-foundation.md](r2mlm7-layer-portal-foundation.md) | 63-72 | `## Portal Contract` of this plan: tile bits, geometry, neighbour mapping, relocation rule and command signature. The authoritative handoff to r2mlm8 and r2mlm9. |
| [docs/zz-reach2/architecture/landscape/archi-landscape.md](../../architecture/landscape/archi-landscape.md) | 183-207 | `## Dispatch`: every tile type is one `TileTypeProcs` row in `_tile_type_procs`; slots after `Object` are `nullptr`. The new type fills slot 11 and must implement every proc that has no safe default. |
| [src/map_layer.h](../../../../src/map_layer.h) | Live definitions; no fixed range | Produced by r2mlm2 and r2mlm4: `MultiLayerMap::TileInLayer`, `GetRules`, `CheckLayerAllowsConstruction` (with `MapLayerConstruction::LayerPortal`). The portal code builds on these and adds no layer arithmetic of its own. |

## Outcome and Boundaries

Exported by the end of this plan (names are final):

- `TileType::LayerPortal` (value 11) in `src/tile_type.h` and `_tile_type_layer_portal_procs` in slot 11 of `_tile_type_procs`.
- `src/layer_portal_map.h`: `IsLayerPortalTile`, `GetLayerPortalTransportType`, `GetLayerPortalShaftDirection`, `GetLayerPortalDoorDirection`, `GetLayerPortalTargetLayer`, `GetLayerPortalPairTile`, `IsLayerPortalShaftExit`, `GetLayerPortalNextTile`, `HasLayerPortalReservation`, `SetLayerPortalReservation`, `GetLayerPortalReservationTrackBits`, `MakeRailLayerPortal`, `MakeRoadLayerPortal`.
- `src/layer_portal_cmd.h` / `src/layer_portal_cmd.cpp`: `Commands::BuildLayerPortal`, `CmdBuildLayerPortal`, `CcBuildLayerPortal`, `TranslateIntoLayerPortalPair`, `LAYER_PORTAL_TRACKBIT_FACTOR`, the tile procs, and pair removal through `ClearTile_LayerPortal`.
- `src/map_layer.h`: `MultiLayerMap::GetPortalTargetLayer(MapLayer from)`, the version-1 pairing rule.
- `CFollowTrackT::is_portal` and the portal branch in `FollowTileExit` / `CanEnterNewTile`.
- Portal cases in rail type, road presence, road bits, reservation, signal, catenary, infrastructure and smallmap code.
- Settings `pf.yapf.rail_layer_portal_penalty` and `pf.yapf.road_layer_portal_penalty`.

Left to the transport plans: placement GUI, vehicle controller relocation and visibility details, the road elevator ride, cost-provider use of the penalties, and art. Until then portals draw with depot stand-ins.

## Portal Contract

- **Pair.** A portal is two tiles: `A` at plane (x, y) in layer `L` and `B = MultiLayerMap::TileInLayer(A, T)` in the target layer `T`, with `|L - T| == 1`. Version 1 builds only Surface and Underground pairs.
- **Directions.** `GetLayerPortalShaftDirection(A) = d` is the heading of a vehicle that goes down the shaft from `A`. `GetLayerPortalShaftDirection(B) = ReverseDiagDir(d)`. The door of each end faces the reverse of its shaft direction. So on the surface the door faces `ReverseDiagDir(d)`, underground it faces `d`.
- **Neighbour mapping.** `GetLayerPortalNextTile(t, exitdir)` returns `GetLayerPortalPairTile(t)` when `t` is a portal and `exitdir == GetLayerPortalShaftDirection(t)`; otherwise `TileAddByDiagDir(t, exitdir)`. A vehicle heading `d` therefore passes `A - d`, `A`, `B`, `B + d` and keeps heading `d`.
- **Relocation.** A vehicle that steps across the shaft edge of `A` is placed on the entry edge of `B` at the same in-tile offset across the axis: `TranslateIntoLayerPortalPair(A, x, y)` adds the layer offset `(T - L) * Map::SizeY() * TILE_SIZE` to `y` and moves the position back one tile along the heading. No wormhole state is used.
- **Visibility.** On `A` a vehicle heading into the shaft is hidden when it reaches the door visibility frame; on `B` it is shown again when it passes the door frame heading out. Frames reuse `_tunnel_visibility_frame`.
- **Tile bits.** `m5` bits 1..0 shaft direction, bits 3..2 `TransportType` (Rail or Road), bit 4 rail reservation, bits 6..5 target layer. `m1` bits 4..0 owner. Rail: `m8` bits 5..0 rail type. Road: `m4` bits 5..0 road type, `m7` bits 4..0 road owner, `m8` bits 11..6 tram type (always `INVALID_ROADTYPE` in version 1). These positions mirror tunnel tiles so the existing rail-type and road-type accessors work unchanged.
- **Track and road bits.** An end offers the axis track (or axis road) in both directions. Only the door side connects to a neighbour for building and drawing purposes.
- **Command.** `CmdBuildLayerPortal(DoCommandFlags flags, TileIndex tile, TransportType transport_type, MapLayer target_layer, DiagDirection shaft_dir, RailType railtype, RoadType roadtype)` builds both ends or fails without side effects. Removing either end removes both.

## Tile-type Switch Ledger

Filled by tasks 4.8 and 5.7. One row per `switch` over `GetTileType` (or `case TileType::TunnelBridge`) that a portal tile can reach.

| File:line | Function | Handling | Done |
| --- | --- | --- | --- |

## Implementation Tasks

### Area Mandatory Reading - Tile type and accessors

_Applies to Group 1._

Entry gate. `src/map_layer.h` must export `MultiLayerMap::TileInLayer`, `MapLayerConstruction::LayerPortal` and `CheckLayerAllowsConstruction` (r2mlm2, r2mlm4), and `SaveLoadVersion::MapLayers` must exist (r2mlm4 task 5.1). Verify with a grep for each name. If any is missing, stop every group; nothing in this plan is safe without them. Do not add `TileType::LayerPortal` to a tree whose saves cannot mark it with `MapLayers`, do not approximate pairs with tunnel tiles, and do not hard-code a layer offset in place of `TileInLayer`.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/tile_type.h](../../../../src/tile_type.h) | 41-64 | `enum class TileType` and the `TileType::End <= TileType::MaxSize` assert: `LayerPortal` is inserted before `End`. |
| [src/tunnelbridge_map.h](../../../../src/tunnelbridge_map.h) | 17-43 | `GetTunnelBridgeDirection`, `GetTunnelBridgeTransportType`: the `m5` bit pattern the portal accessors mirror. |
| [src/tunnelbridge_map.h](../../../../src/tunnelbridge_map.h) | 85-120 | `HasTunnelBridgeReservation`, `SetTunnelBridgeReservation`, `GetTunnelBridgeReservationTrackBits`: the reservation accessors the portal copies for rail ends. |
| [src/tunnel_map.h](../../../../src/tunnel_map.h) | 43-86 | `MakeRoadTunnel` and `MakeRailTunnel`: how a tunnel end initialises every byte, including road and tram owners and rail type. |
| [docs/zz-reach2/architecture/landscape/tunnelbridge-tiles.md](../../architecture/landscape/tunnelbridge-tiles.md) | 5-41 | `## Ramp versus tunnel` to `## Road, tram, and rail on the ramp`: the documented bit layout behind the contract's tile bits. |

### Group 1 - Tile type and accessors

{{MEDIUM}}
- [ ] 1.1 Insert `LayerPortal, ///< Entry or exit of a pair of tiles connecting two map layers.` before `End` in `enum class TileType`.
- [ ] 1.2 Create `src/layer_portal_map.h` with the licence header and include guard, including `tile_map.h`, `map_layer.h`, `rail_map.h` and `road_map.h`.
- [ ] 1.3 Implement `IsLayerPortalTile`, `GetLayerPortalTransportType`, `GetLayerPortalShaftDirection`, `GetLayerPortalDoorDirection` and `GetLayerPortalTargetLayer` with asserts on the tile type and Javadoc that cites the contract bits.
- [ ] 1.4 Implement `GetLayerPortalPairTile` with `MultiLayerMap::TileInLayer` and a Debug assert that the pair is a portal with the reversed shaft direction (I-8).
- [ ] 1.5 Implement `HasLayerPortalReservation`, `SetLayerPortalReservation` and `GetLayerPortalReservationTrackBits`, asserting the rail transport type.
- [ ] 1.6 Implement `MakeRailLayerPortal(Tile t, Owner o, DiagDirection shaft_dir, MapLayer target, RailType rt)` and `MakeRoadLayerPortal(Tile t, Owner o, DiagDirection shaft_dir, MapLayer target, RoadType road_rt)`, initialising every byte as the tunnel constructors do.
- [ ] 1.7 Register `layer_portal_map.h` in `src/CMakeLists.txt` and build.

### Area Mandatory Reading - Tile procs

_Applies to Group 2._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/tile_cmd.h](../../../../src/tile_cmd.h) | 210-232 | `struct TileTypeProcs`: which procs have defaults (`get_tile_track_status_proc`, `change_tile_owner_proc`, `get_foundation_proc`) and which must be provided. |
| [src/landscape.cpp](../../../../src/landscape.cpp) | 52-89 | The `extern const TileTypeProcs` list and `_tile_type_procs`: slot 11 is the first `nullptr`. |
| [src/tunnelbridge_cmd.cpp](../../../../src/tunnelbridge_cmd.cpp) | 1949-2098 | `_tunnel_visibility_frame` and `VehicleEnterTile_TunnelBridge`: the frame arithmetic for hiding and showing vehicles at a mouth, reused by the portal's enter-tile proc. |
| [src/tunnelbridge_cmd.cpp](../../../../src/tunnelbridge_cmd.cpp) | 1854-1917 | `GetTileTrackStatus_TunnelBridge` and `ChangeTileOwner_TunnelBridge`: patterns for axis track status and for owner changes that also fix infrastructure counts. |
| [src/tunnelbridge_cmd.cpp](../../../../src/tunnelbridge_cmd.cpp) | 2138-2150 | `_tile_type_tunnelbridge_procs`: the designated-initialiser layout of a full proc table. |
| [src/rail_cmd.cpp](../../../../src/rail_cmd.cpp) | 2291-2436 | `DrawTrackBits`: track drawing on barren ground (`PALETTE_TO_BARE_LAND`), used for the rail stand-in floor. |

### Group 2 - Tile procs

{{HARD}}
- [ ] 2.1 Create `src/layer_portal_cmd.cpp` and register it; define `_tile_type_layer_portal_procs` with every proc listed in `## Outcome and Boundaries`, and add it to slot 11 of `_tile_type_procs` and to the `extern` list in `src/landscape.cpp`.
- [ ] 2.2 Implement `DrawTile_LayerPortal` with stand-ins: the rail depot layout for rail ends and the road depot layout for road ends, door on `GetLayerPortalDoorDirection`, axis track or road on barren ground; leave a comment naming the r2mlm8 and r2mlm9 art swap tasks.
- [ ] 2.3 Implement `GetSlopePixelZ_LayerPortal` (flat tile), `GetTileDesc_LayerPortal` (new strings `STR_LAI_LAYER_PORTAL_RAIL` and `STR_LAI_LAYER_PORTAL_ROAD` with the target layer name) and an empty `TileLoop_LayerPortal`.
- [ ] 2.4 Implement `GetTileTrackStatus_LayerPortal`: axis trackdirs in both directions for the matching transport (and road sub-type), nothing otherwise, with the reservation bit reported for rail.
- [ ] 2.5 Implement `ChangeTileOwner_LayerPortal` updating both ends and the infrastructure counts (see Group 7 for the factor).
- [ ] 2.6 Implement the terraform proc and the build-above proc as refusals (`STR_ERROR_MUST_REMOVE_LAYER_PORTAL_FIRST`), and the foundation proc as none.
- [ ] 2.7 Implement `VehicleEnterTile_LayerPortal` with the generic visibility rule of the contract: compute the frame like `VehicleEnterTile_TunnelBridge`, hide on the shaft-bound door frame, show on the door-bound frame, return `{}` otherwise; leave vehicle-type specifics to r2mlm8 and r2mlm9.
- [ ] 2.8 Build both configurations; confirm `ctest` still passes.

### Area Mandatory Reading - Neighbour contract and follower

_Applies to Group 3._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/pathfinder/follow_track.hpp](../../../../src/pathfinder/follow_track.hpp) | 28-52 | `struct CFollowTrackT` members: `is_tunnel`, `is_bridge`, `is_station`, `tiles_skipped` and their reset in `Init`. `is_portal` joins them. |
| [src/pathfinder/follow_track.hpp](../../../../src/pathfinder/follow_track.hpp) | 122-177 | `Follow`: the order exit check, `FollowTileExit`, track status, enter check, 90-degree check. |
| [src/pathfinder/follow_track.hpp](../../../../src/pathfinder/follow_track.hpp) | 209-245 | `FollowTileExit`: the tunnel and bridge branch that jumps to the other end; the portal branch sits next to it. |
| [src/pathfinder/follow_track.hpp](../../../../src/pathfinder/follow_track.hpp) | 298-407 | `CanEnterNewTile`: per-type entry checks, including "tunnel holes can be entered only from proper direction", which the portal mirrors for its door. |
| [src/pathfinder/yapf/yapf_common.hpp](../../../../src/pathfinder/yapf/yapf_common.hpp) | 106-131 | `OctileDistanceCost`: the shared estimate for rail, road and ship order searches. |
| [docs/zz-reach2/architecture/pathfinder/archi-pathfinder.md](../../architecture/pathfinder/archi-pathfinder.md) | 230-238 | The closed-node consistency check and the octile heuristic: why the estimate must use plane coordinates. |

### Group 3 - Neighbour contract and follower

{{HARD}}
- [ ] 3.1 Implement `IsLayerPortalShaftExit(TileIndex tile, DiagDirection exitdir)` and `GetLayerPortalNextTile(TileIndex tile, DiagDirection exitdir)` in `src/layer_portal_map.h` exactly as the contract states, with Javadoc that cites I-3.
- [ ] 3.2 Implement `TranslateIntoLayerPortalPair(TileIndex portal, int &x, int &y)` in `src/layer_portal_cmd.cpp` (declared in `layer_portal_cmd.h`) following the contract's relocation rule, with an assert that the result lies on the pair tile.
- [ ] 3.3 Add `bool is_portal` to `CFollowTrackT`, reset it in `Init` and in every place that resets `is_tunnel`.
- [ ] 3.4 In `FollowTileExit`, add the portal branch before the normal step: when `IsLayerPortalShaftExit(old_tile, exitdir)`, set `is_portal`, `new_tile = GetLayerPortalPairTile(old_tile)` and `tiles_skipped = 0`.
- [ ] 3.5 In `CanEnterNewTile`, refuse entering a portal from a side other than its door unless `is_portal` is set, and refuse when the end's transport type does not match the follower's.
- [ ] 3.6 Change `OctileDistanceCost` to use `TilePlaneY` for both tiles, with a comment citing the consistency check.
- [ ] 3.7 Add unit tests: for a hand-made pair in a 64x64x3 map, `GetLayerPortalNextTile` maps every shaft exit to the pair and every other exit to the plain neighbour, in both layers and all four directions; `TranslateIntoLayerPortalPair` lands on the pair's entry edge.

### Area Mandatory Reading - Rail and road core integration

_Applies to Group 4._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/rail.cpp](../../../../src/rail.cpp) | 34-62 | `GetTileRailType`: per-type switch returning the rail type or `INVALID_RAILTYPE`. |
| [src/road_map.cpp](../../../../src/road_map.cpp) | 16-80 | `MayHaveRoad` and `GetAnyRoadBits`: road presence and the "entrance treated as road piece" rule for depots and tunnels, which the portal door follows. |
| [src/pbs.cpp](../../../../src/pbs.cpp) | 18-187 | `GetReservedTrackbits`, `TryReserveRailTrack`, `UnreserveRailTrack`: per-type reservation cases. |
| [src/signal.cpp](../../../../src/signal.cpp) | 268-410 | `ExploreSegment`: the signal-block walk with the tunnel case that jumps to the other end and the `enterdir` / `exitdir` convention. |
| [docs/zz-reach2/architecture/landscape/rail-tiles.md](../../architecture/landscape/rail-tiles.md) | 75-83 | `## Path reservation`: where reservation bits live per tile family. |

### Group 4 - Rail and road core integration

{{HARD}}
- [ ] 4.1 `GetTileRailType`: return the rail type for rail portal ends.
- [ ] 4.2 `MayHaveRoad`: true for road portal ends; confirm `GetRoadTypeRoad`, `GetRoadOwner` and `HasTileRoadType` work through the mirrored bits.
- [ ] 4.3 `GetAnyRoadBits`: return the door-side road bit for road portal ends (both bits on the axis when `straight_tunnel_bridge_entrance` is set), with a comment.
- [ ] 4.4 `GetReservedTrackbits`, `TryReserveRailTrack`, `UnreserveRailTrack`: add rail portal cases using the portal reservation accessors.
- [ ] 4.5 `ExploreSegment`: add the portal case: entering through the door continues on the pair entered through its shaft side; entering through the shaft continues through the door to the next tile; detect trains on both ends.
- [ ] 4.6 The second `TunnelBridge` case in `src/signal.cpp` and both cases in `src/elrail.cpp`: add portal handling (no catenary drawn on portal ends; track bits reported for power checks).
- [ ] 4.7 `src/road.cpp`: add the portal case next to the tunnel case.
- [ ] 4.8 Fill `## Tile-type Switch Ledger` with every `case TileType::TunnelBridge` site found by grep, and mark the ones handled in this group.

### Area Mandatory Reading - Remaining tile-type switches

_Applies to Group 5._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/smallmap_gui.cpp](../../../../src/smallmap_gui.cpp) | 399-411 | `_tiletype_importance`: one entry per tile type; slot 11 needs a value and the colour helpers need a portal colour. |
| [src/newgrf_commons.cpp](../../../../src/newgrf_commons.cpp) | 428-443 | `GetNearbyTileInformation`: reports the raw tile type to NewGRFs; a portal is reported as `Railway` or `Road` by transport so GRFs never see value 11. |
| [src/saveload/company_sl.cpp](../../../../src/saveload/company_sl.cpp) | 128-272 | `AfterLoadCompanyStats`: recounts infrastructure per tile type; portal ends must be counted with `LAYER_PORTAL_TRACKBIT_FACTOR`. |

### Group 5 - Remaining tile-type switches

{{MEDIUM}}
- [ ] 5.1 `src/rail_cmd.cpp` (four cases) and `src/road_cmd.cpp` (four cases): add portal handling or an explicit `//not applicable to layer portals` comment per case.
- [ ] 5.2 `src/town_cmd.cpp` (two cases): towns never grow into, connect to or demolish portals; add the case with that comment.
- [ ] 5.3 `src/water_cmd.cpp` (two cases): portals are never flooded and never count as water.
- [ ] 5.4 `src/newgrf_commons.cpp`, `src/newgrf_railtype.cpp`, `src/newgrf_roadtype.cpp`: report portals as their transport's base type and resolve rail and road type sprites for them.
- [ ] 5.5 `src/smallmap_gui.cpp`: give slot 11 an importance and draw portals in the rail or road colour of the routes and owners map types.
- [ ] 5.6 `src/saveload/company_sl.cpp`: count portal ends in `AfterLoadCompanyStats`; `src/saveload/afterload.cpp` and `oldloader_sl.cpp` cases: confirm portals cannot exist in files before `SaveLoadVersion::MapLayers` and add a Debug assert.
- [ ] 5.7 Grep for remaining `switch (GetTileType(` statements with a `default: NOT_REACHED()`; add a portal case to each one reachable for a portal tile and record it in the ledger.
- [ ] 5.8 Mark every ledger row done and build.

### Area Mandatory Reading - Build and remove command

_Applies to Groups 6-7._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/commands/archi-commands.md](../../architecture/commands/archi-commands.md) | 9-37 | `## Pipeline`: test run then execute, money handling, and network replay; the portal command must change nothing when `DoCommandFlag::Execute` is clear. |
| [src/command_type.h](../../../../src/command_type.h) | 206-214 | The `BuildBridge` to `BuildTunnel` enumerators of `enum class Commands` (the enum spans lines 200-384 and ends with `End`): `BuildLayerPortal` is added right after `BuildTunnel`, with the same `///<` comment style. |
| [src/tunnelbridge_cmd.h](../../../../src/tunnelbridge_cmd.h) | 1-28 | Command header shape: declarations, `DEF_CMD_TRAIT` with flags and `CommandType::LandscapeConstruction`, callback declaration. `layer_portal_cmd.h` copies it. |
| [src/command.cpp](../../../../src/command.cpp) | 26-57 | The `*_cmd.h` include list that builds the command table; `layer_portal_cmd.h` is added alphabetically. |
| [src/road_cmd.cpp](../../../../src/road_cmd.cpp) | 1130-1222 | `CmdBuildRoadDepot` and `RemoveRoadDepot`: validation order (type, direction, slope, clear, bridge height), infrastructure update and removal checks (`CheckTileOwnership`, `EnsureNoVehicleOnGround`). |
| [src/tunnelbridge_cmd.cpp](../../../../src/tunnelbridge_cmd.cpp) | 862-933 | `DoClearTunnel`: removing a two-ended rail structure safely, including reservations, signal updates and `YapfNotifyTrackLayoutChange`. |

### Group 6 - Build command

{{MEDIUM}}
- [ ] 6.1 Add `BuildLayerPortal, ///< build a pair of layer portal tiles` after `BuildTunnel` in `Commands`.
- [ ] 6.2 Create `src/layer_portal_cmd.h` declaring `CmdBuildLayerPortal` with the contract signature, `DEF_CMD_TRAIT(Commands::BuildLayerPortal, CmdBuildLayerPortal, CommandFlags({CommandFlag::Auto, CommandFlag::NoWater}), CommandType::LandscapeConstruction)` and `CommandCallback CcBuildLayerPortal`; include it in `src/command.cpp`.
- [ ] 6.3 Validate parameters: transport Rail or Road, valid and available rail type or road (not tram) type, valid shaft direction, valid target layer adjacent to the tile's layer, and Surface / Underground only in version 1.
- [ ] 6.4 Validate both ends: inner tiles, flat, no bridge above, `CheckLayerAllowsConstruction(..., MapLayerConstruction::LayerPortal)`, and a door-side neighbour that is an inner tile.
- [ ] 6.5 Clear both ends with `Command<Commands::LandscapeClear>::Do` and add the costs; add a portal price of `_price[Price::BuildTunnel] * 4` per pair and document the choice.
- [ ] 6.6 On execute, make both ends with the contract's shaft directions, update infrastructure, mark both tiles dirty, and for rail ends add the sides to the signal buffer and call `YapfNotifyTrackLayoutChange`.
- [ ] 6.7 Implement `CcBuildLayerPortal` playing the construction sound at the clicked tile; add strings `STR_ERROR_CAN_T_BUILD_LAYER_PORTAL` and `STR_ERROR_LAYER_PORTAL_NEEDS_FLAT_LAND`.
- [ ] 6.8 Add `MultiLayerMap::GetPortalTargetLayer(MapLayer from)` (Surface to Underground, Underground to Surface, `MapLayer::Invalid` otherwise) as the single version-1 pairing rule, used by task 6.3 and by the placement GUIs of r2mlm8 and r2mlm9.

### Group 7 - Removal, ownership and infrastructure

{{MEDIUM}}
- [ ] 7.1 Define `LAYER_PORTAL_TRACKBIT_FACTOR` (1 rail piece per rail end, 2 road pieces per road end, matching depots) with a comment.
- [ ] 7.2 Implement `ClearTile_LayerPortal`: refuse with `DoCommandFlag::Auto`, check ownership, `EnsureNoVehicleOnGround` on both ends, refuse a reserved rail portal, then clear both ends with `DoClearSquare`, update infrastructure and refund `_price[Price::ClearTunnel] * 4`.
- [ ] 7.3 For rail ends, add both sides to the signal buffer and call `YapfNotifyTrackLayoutChange` on removal.
- [ ] 7.4 Confirm `ChangeTileOwner_LayerPortal` handles company bankruptcy and merging (both ends move together or both are removed).
- [ ] 7.5 Confirm `AfterLoadCompanyStats` and the live counters agree after build, removal and owner change (compare the infrastructure window before and after a save and load).

### Area Mandatory Reading - Pathfinder settings and verification

_Applies to Group 8._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/settings_type.h](../../../../src/settings_type.h) | 528-563 | `struct YAPFSettings`: where the two penalties are added. |
| [src/table/settings/pathfinding_settings.ini](../../../../src/table/settings/pathfinding_settings.ini) | 341-348 | The `pf.yapf.road_stop_penalty` entry: the `[SDT_VAR]` shape with `from = SaveLoadVersion::...`, default and range. |
| [src/tests/map_layer.cpp](../../../../src/tests/map_layer.cpp) | Live definitions; no fixed range | Produced by r2mlm2 Group 6: the test file the portal tests extend. |

### Group 8 - Pathfinder penalty settings and verification

{{SIMPLE}}
- [ ] 8.1 Add `rail_layer_portal_penalty` and `road_layer_portal_penalty` to `YAPFSettings` with `///<` comments.
- [ ] 8.2 Add both `[SDT_VAR]` entries with `from = SaveLoadVersion::MapLayers`, default `4 * YAPF_TILE_LENGTH`, range 0 to 1000000, `cat = SC_EXPERT`, and their help strings.
- [ ] 8.3 Build and confirm both appear in the advanced settings window under pathfinding.
- [ ] 8.4 Add accessor round-trip tests for both `Make*LayerPortal` functions and an I-8 test that a pair built by the same code as `CmdBuildLayerPortal`'s execute step has reversed shaft directions.
- [ ] 8.5 Build both configurations without new warnings; run `ctest`.
- [ ] 8.6 Grep that no code outside `src/layer_portal_*` and `src/pathfinder/follow_track.hpp` calls `GetLayerPortalPairTile` for movement (I-3); signal and reservation code may call `GetLayerPortalNextTile`.
- [ ] 8.7 Confirm the `## Tile-type Switch Ledger` has no open row.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| YAPF hits `NOT_REACHED` in `AddNewNode` after Group 3 | [src/pathfinder/yapf/yapf_base.hpp](../../../../src/pathfinder/yapf/yapf_base.hpp) | `AddNewNode` | The estimate is already plane-based | Find a negative or too-small step cost around portals. |
| A rail type NewGRF draws wrong sprites on portals | [docs/zz-reach2/architecture/newgrf/features.md](../../architecture/newgrf/features.md) | The feature table (lines 1-29), row `RailTypes` | The issue is a sprite group lookup rather than drawing order | Find the rail type feature handler, then read `GetCustomRailSprite` in `src/newgrf_railtype.cpp` to see which sprite groups apply to tunnel-like tiles. |
| Signal blocks across a portal show the wrong state | [docs/zz-reach2/architecture/landscape/rail-tiles.md](../../architecture/landscape/rail-tiles.md) | `## Signals` (lines 84-117) | The fault is not in `ExploreSegment` traversal | Understand signal state bits. |
| A switch outside the ledger asserts on a portal tile | [src/landscape.cpp](../../../../src/landscape.cpp) | `_tile_type_procs` (lines 52-89), then the asserting `switch (GetTileType(...))` from the stack trace | The caller is reachable from a portal tile | Confirm the proc table is complete, then add the missing case and a row in `## Tile-type Switch Ledger`. |
| The command's test and execute costs differ | [docs/zz-reach2/architecture/commands/archi-commands.md](../../architecture/commands/archi-commands.md) | `## Command flags` (lines 152-184) | The difference comes from clearing both ends | Decide whether `CommandFlag::NoTest` is justified; prefer making test and execute agree. |

## Planning Provenance

This table records what shaped the plan. It is not an execution reading queue, it must not be preloaded after context compaction, it is non-transitive, and its line ranges record planning use rather than runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [src/pathfinder/follow_track.hpp](../../../../src/pathfinder/follow_track.hpp) | 28-460 | Showed tunnels already jump tiles in one follower branch, so one portal branch carries every pathfinder. |
| [src/tunnelbridge_cmd.cpp](../../../../src/tunnelbridge_cmd.cpp) | 644-933, 1854-2150 | Tunnel build, removal, track status, owner change and visibility frames; the template for the procs. |
| [src/signal.cpp](../../../../src/signal.cpp) | 268-410 | Signal walk with a tunnel jump; decided the portal case shape. |
| Repository grep of `case TileType::TunnelBridge` | 36 cases in 17 files | Sized Groups 4 and 5 and the switch ledger. |
| [src/rail_map.h](../../../../src/rail_map.h), [src/road_map.h](../../../../src/road_map.h) | `RailTileType` (23-27), `RoadTileType` (22-26) | Considered portals as rail and road sub-types; rejected because `else` branches would silently treat them as depots, while a new tile type fails loudly. |
| [docs/zz-reach2/architecture/pathfinder/archi-pathfinder.md](../../architecture/pathfinder/archi-pathfinder.md) | 178-246 | Heuristic consistency requirement behind task 3.6. |

## Completion Conditions

- `TileType::LayerPortal` exists with a complete proc table, and every reachable tile-type switch handles it.
- `CmdBuildLayerPortal` builds and removes pairs atomically with correct costs, ownership and infrastructure.
- `GetLayerPortalNextTile` is used by the follower, signal and reservation code; YAPF estimates are plane-based.
- Unit tests cover accessors, neighbour mapping, relocation and the pair invariant.
- The pathfinder penalty settings exist and are saved from `SaveLoadVersion::MapLayers`.
