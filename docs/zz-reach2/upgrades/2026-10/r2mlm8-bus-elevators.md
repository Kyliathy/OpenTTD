# R2MLM8 - Bus Elevators

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
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 98-100 | `### AD-8 Cross-layer movement only through layer portals`: the bus elevator is a road portal; vehicles keep their heading and are relocated at the shaft edge without wormhole state. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 114-116 | `### AD-12 Version-1 scope`: road elevators carry road vehicles only (no trams); Space has no construction. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 141-170 | `## Engineering Conventions` and `## Build and Test Commands`. |
| [r2mlm7-layer-portal-foundation.md](r2mlm7-layer-portal-foundation.md) | 63-72 | `## Portal Contract`: pair, directions, neighbour mapping, relocation, visibility, tile bits and the command signature. This plan adds road-specific behaviour on top and must not redefine any of it. |
| [docs/zz-reach2/architecture/vehicles/road-vehicles.md](../../architecture/vehicles/road-vehicles.md) | 7-44 | `## Tick` and `## One step`: the road controller loop, `IndividualRoadVehicleController`, frame tables, `VehicleEnterTile` refusals and `RoadFindPathToDest`. Every traversal change hooks into this step. |
| [src/layer_portal_map.h](../../../../src/layer_portal_map.h) | Live definitions; no fixed range | Produced by r2mlm7 Groups 1 and 3: `IsLayerPortalTile`, `GetLayerPortalTransportType`, `GetLayerPortalDoorDirection`, `GetLayerPortalNextTile`, `IsLayerPortalShaftExit` are authoritative. |

## Outcome and Boundaries

- A "Build road elevator" button on the road toolbar (not the tram toolbar) with a door-direction picker. It builds a road portal pair between the viewed layer and the other of Surface and Underground.
- Buses and lorries drive into an elevator, ride it at a capped speed while hidden, and drive out on the other layer heading the same way. Road pathfinding routes through elevators and prices them with `pf.yapf.road_layer_portal_penalty`.
- Underground bus stops, depots and roads work with orders, servicing, autoreplace and the vehicle view.
- Elevator art from r2mlm6 replaces the depot stand-in when the constants exist.

Placement rule: the player picks the door direction `door` on the viewed layer. The command receives `shaft_dir = ReverseDiagDir(door)`. On the other layer the door therefore faces `ReverseDiagDir(door)`; the tooltip says so.

Out of scope: trams, catchment of underground stops, script support.

## Implementation Tasks

### Area Mandatory Reading - Elevator tile and placement GUI

_Applies to Groups 1-2._

Entry gate. r2mlm7 must be complete: `Commands::BuildLayerPortal`, `GetLayerPortalNextTile`, `TranslateIntoLayerPortalPair`, `MultiLayerMap::GetPortalTargetLayer`, `CFollowTrackT::is_portal` and `pf.yapf.road_layer_portal_penalty` must exist. Verify with a grep for each name. If any is missing, stop every group of this plan and finish r2mlm7 first. Do not add a road-only portal command, do not compute the pair tile locally, and do not reuse road tunnels or depots as a substitute for a portal.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/road_map.cpp](../../../../src/road_map.cpp) | 38-80 | `GetAnyRoadBits`: the door-side road bit returned for portal ends (r2mlm7 task 4.3) is what makes neighbouring roads connect and draw towards the door. |
| [src/road_cmd.h](../../../../src/road_cmd.h) | 20-30 | Road command declarations: `CmdBuildRoad` (per-tile) and `CmdConvertRoad`, the two procs that must treat portal ends correctly. |
| [src/road_cmd.cpp](../../../../src/road_cmd.cpp) | 1714-1898 | `DrawTile_Road`: the road depot branch draws the depot layout with road type overlays; the stand-in elevator drawing reuses that path. |
| [src/widgets/road_widget.h](../../../../src/widgets/road_widget.h) | 14-30 | `enum RoadToolbarWidgets`: `WID_ROT_LAYER_ELEVATOR` is inserted after `WID_ROT_BUILD_TUNNEL`; hotkeys use widget values. |
| [src/road_gui.cpp](../../../../src/road_gui.cpp) | 537-750 | `BuildRoadToolbarWindow::GetCursorForWidget`, `GetHighLightStyleForWidget`, `OnClick` and `OnPlaceObject`: the depot button path (`WID_ROT_DEPOT`) is the template for the elevator button. |
| [src/road_gui.cpp](../../../../src/road_gui.cpp) | 938-1015 | `road_hotkeys` and `_nested_build_road_widgets`: hotkey list and the road toolbar part list (the tram toolbar has its own list and stays unchanged). |
| [src/road_gui.cpp](../../../../src/road_gui.cpp) | 1191-1279 | `BuildRoadDepotWindow` and `ShowRoadDepotPicker`: the direction picker the elevator picker is cloned from, with `_road_depot_orientation`. |

### Group 1 - Elevator tile behaviour

{{MEDIUM}}
- [x] 1.1 In the road branch of `DrawTile_LayerPortal`, draw the stand-in with the road depot layout and the road type overlays of `GetRoadTypeRoad(tile)`, door on `GetLayerPortalDoorDirection`.
- [x] 1.2 Make `CmdBuildRoad` accept building the neighbour's road bit towards an elevator door and refuse building road bits on the elevator tile itself (`STR_ERROR_MUST_REMOVE_LAYER_PORTAL_FIRST`).
- [x] 1.3 Make road drawing of a neighbouring tile treat the elevator door as a connection (road ends do not show a dead-end stub).
- [x] 1.4 Make `CmdConvertRoad` convert both ends of a road elevator together, checking compatibility like road depots.
- [x] 1.5 Confirm `VehicleEnterTile_LayerPortal` refuses trams on road portals (`VehicleEnterTileState::CannotEnter`) and `CanEnterNewTile` (r2mlm7 task 3.5) keeps trams out of the pathfinder.

### Group 2 - Placement GUI

{{MEDIUM}}
- [x] 2.1 Add `WID_ROT_LAYER_ELEVATOR, ///< Build road elevator to another map layer.` after `WID_ROT_BUILD_TUNNEL`, and add the button to `_nested_build_road_widgets` with `SPR_IMG_ROAD_DEPOT` as stand-in sprite and `STR_ROAD_TOOLBAR_TOOLTIP_BUILD_LAYER_ELEVATOR`.
- [x] 2.2 Add `static DiagDirection _road_elevator_door_direction` and `BuildRoadElevatorWindow` (cloned from `BuildRoadDepotWindow`, its own widget enum and `WindowDesc` with a unique ini key) showing four door directions and the "door on the other layer faces the opposite way" hint.
- [x] 2.3 In `GetCursorForWidget` and `GetHighLightStyleForWidget`, return the road type's depot cursor (stand-in) and `HT_RECT` for the elevator button.
- [x] 2.4 In `OnClick`, open the elevator picker like the depot button; in `OnPlaceObject`, post `Command<Commands::BuildLayerPortal>::Post(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL, CcBuildLayerPortal, tile, TransportType::Road, target, ReverseDiagDir(_road_elevator_door_direction), INVALID_RAILTYPE, _cur_roadtype)`.
- [x] 2.5 Compute `target` as `MultiLayerMap::GetPortalTargetLayer` of the main viewport's layer (r2mlm7 task 6.8); disable the button when that returns `MapLayer::Invalid` (Space, or a one-layer game), refreshed when the layer changes.
- [x] 2.6 Add a free hotkey (check `road_hotkeys`) named `layer_elevator`.
- [ ] 2.7 Build and place an elevator on the surface; confirm both ends appear, the cost is charged once, and removing either end removes both.

### Area Mandatory Reading - Road vehicle traversal

_Applies to Groups 3-4._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/roadveh.h](../../../../src/roadveh.h) | 23-83 | `RoadVehicleStates` and the frame constants: an elevator ride keeps `state` as a plain trackdir; no new state value is needed. |
| [src/roadveh_cmd.cpp](../../../../src/roadveh_cmd.cpp) | 1194-1359 | The `RDE_NEXT_TILE` block of `IndividualRoadVehicleController`: computes the next tile with `TileOffsByDiagDir`, chooses a trackdir, positions the vehicle from the new tile's frame table, calls `VehicleEnterTile`, then commits tile, state and frame. |
| [src/roadveh_cmd.cpp](../../../../src/roadveh_cmd.cpp) | 1052-1128 | `FollowPreviousRoadVehicle`: articulated parts follow the part ahead onto `tile`; the caller passes the portal-aware tile. |
| [src/roadveh_cmd.cpp](../../../../src/roadveh_cmd.cpp) | 796-865 | `CheckRoadBlockedForOvertaking` and `RoadVehCheckOvertake`: overtaking must never start on or into an elevator. |
| [src/roadveh_cmd.cpp](../../../../src/roadveh_cmd.cpp) | 470-492 | `RoadVehicle::GetCurrentMaxSpeed`: per-part speed limits (curves, bridges); the elevator ride cap goes here. |
| [src/tunnelbridge_cmd.cpp](../../../../src/tunnelbridge_cmd.cpp) | 2001-2025 | Road branch of `VehicleEnterTile_TunnelBridge`: how a road vehicle is hidden and shown at a mouth and how `frame` is kept in sync. |

### Group 3 - Road vehicle traversal

{{HARD}}
- [x] 3.1 In the `RDE_NEXT_TILE` block, replace `v->tile + TileOffsByDiagDir(diagdir)` with `GetLayerPortalNextTile(v->tile, diagdir)` and keep the rest of the block unchanged; the frame table then positions the vehicle on the pair tile's entry edge.
- [x] 3.2 Pass the same portal-aware tile to `FollowPreviousRoadVehicle` for articulated parts.
- [x] 3.3 Forbid U-turns on elevator tiles: a reversing trackdir on a portal tile is never chosen; if the front cannot continue it waits (`cur_speed = 0`).
- [x] 3.4 Refuse overtaking on and into elevator tiles in `CheckRoadBlockedForOvertaking`.
- [x] 3.5 In the road branch of `VehicleEnterTile_LayerPortal`, apply the contract's hide and show frames and assert `frame` stays in sync with the frame table, mirroring the tunnel code.
- [x] 3.6 After the relocation, update the Z position and inclination of each part with `UpdateInclination(true, true)` while hidden, so no visible jump occurs.
- [ ] 3.7 Confirm `RoadVehFindCloseTo` (vehicle ahead detection) works across the relocation: a bus waiting in the shaft blocks the next one on the same layer side.

### Group 4 - The elevator ride

{{MEDIUM}}
- [x] 4.1 Add `static constexpr uint16_t LAYER_ELEVATOR_MAX_SPEED` with a comment explaining the chosen value (slow enough that a ride lasts about two seconds at normal speed).
- [x] 4.2 Cap `RoadVehicle::GetCurrentMaxSpeed` at that value while any part of the vehicle is on a road portal tile.
- [x] 4.3 Play an existing mechanical sound when the front part becomes hidden in an elevator; pick a sound id from `src/sound_type.h` and record the choice. The sound is `SND_21_ROAD_WORKS`.
- [ ] 4.4 Confirm breakdowns, stop orders and the player stop button behave while a bus is inside, and that a bus stopped inside stays hidden and resumes correctly.
- [ ] 4.5 Save and load while a bus is half way through an elevator (front underground, rear on the surface); confirm it continues without a jump.

### Area Mandatory Reading - Road pathfinding

_Applies to Group 5._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/pathfinder/archi-pathfinder-road.md](../../architecture/pathfinder/archi-pathfinder-road.md) | 38-109 | `## Cost`, `## Destination` and `## Vehicle path cache`: per-tile costs, the octile estimate and the per-vehicle path cache that stores tiles of the route. |
| [src/pathfinder/yapf/yapf_road.cpp](../../../../src/pathfinder/yapf/yapf_road.cpp) | 62-182 | `OneTileCost` and `PfCalcCost`: tile and segment costs; the elevator penalty is added here. |
| [src/pathfinder/yapf/yapf_road.cpp](../../../../src/pathfinder/yapf/yapf_road.cpp) | 290-301 | Road `PfCalcEstimate`: uses `OctileDistanceCost`, which r2mlm7 made plane-based; the assert `estimate >= parent->estimate` must keep holding across elevators. |
| [src/roadveh_cmd.cpp](../../../../src/roadveh_cmd.cpp) | 887-996 | `RoadFindPathToDest`: path cache pop, single-trackdir shortcut and the YAPF call. |
| [src/roadveh_cmd.cpp](../../../../src/roadveh_cmd.cpp) | 347-352 | `FindClosestRoadDepot`: depot search used for servicing; must find depots through elevators. |

### Group 5 - Road pathfinding

{{HARD}}
- [x] 5.1 Add `pf.yapf.road_layer_portal_penalty` once per elevator traversal in `PfCalcCost` (when the segment passes from one end to its pair), with a comment.
- [ ] 5.2 Confirm `RoadFindPathToDest` path-cache entries that end on the pair tile are accepted (the cached tile is the pair, not the plain neighbour).
- [ ] 5.3 Confirm the estimate assert holds on routes through elevators in a Debug build (no `NOT_REACHED`, no assert).
- [ ] 5.4 Confirm `FindClosestRoadDepot` and automatic servicing find a depot on the other layer within `maximum_go_to_depot_penalty`.
- [ ] 5.5 Confirm `RoadVehicle::SetDestTile` and lost-vehicle news behave when the destination is on the other layer and no elevator exists (the bus is reported lost, it does not crash).
- [x] 5.6 Add a Debug-only consistency check in `RoadFindPathToDest` that the chosen next tile equals `GetLayerPortalNextTile` of the current tile and exit direction.

### Area Mandatory Reading - Underground network and art

_Applies to Groups 6-7._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [r2mlm6-layer-graphics-assets.md](r2mlm6-layer-graphics-assets.md) | 56-76 | `## Sprite Contract`: `SPR_MAP_LAYER_ROAD_ELEVATOR_SURFACE` and `_UNDERGROUND` (base + dir * 2 + part), `SPR_IMG_BUILD_ROAD_ELEVATOR`, `SPR_CURSOR_ROAD_ELEVATOR`. |
| [src/table/sprites.h](../../../../src/table/sprites.h) | Live definitions; no fixed range | Produced by r2mlm6 Group 6. If the constants are absent, skip Group 7 and leave the stand-ins; do not add placeholder constants. |
| [docs/zz-reach2/architecture/vehicles/archi-vehicles.md](../../architecture/vehicles/archi-vehicles.md) | 165-177 | `## Depots and servicing`: depot entry, servicing and autoreplace, exercised underground in Group 6. |

### Group 6 - Underground bus network

{{MEDIUM}}
- [ ] 6.1 Build underground roads, a drive-through and a bay bus stop and a road depot in the Underground view; confirm each command succeeds and the land info shows the Underground layer.
- [ ] 6.2 Confirm an underground station takes its name and town from the town above (r2mlm3 projection) and appears in the station list.
- [ ] 6.3 Give a bus orders between a surface stop and an underground stop; confirm it loads, unloads and pays by plane distance.
- [ ] 6.4 Confirm buying, selling, servicing and autoreplace in an underground depot.
- [ ] 6.5 Confirm the vehicle view follows a bus into the Underground layer and back, and that "location" buttons switch the main view's layer.

The r2mlm6 sprite constants are not in `src/table/sprites.h`, so this group stays skipped and the depot stand-in remains.

### Group 7 - Elevator art

2026-10-03: r2mlm6 publishes the elevator constants and all four directional back/front pairs. Drawing and toolbar/cursor swaps are implemented and build in Debug. Visibility checks wait for the user's representative save.

{{SIMPLE}}
- [x] 7.1 Draw the elevator back part as a ground child sprite and the front part as a parent sprite with a bounding box that lets vehicles pass beneath, for both ends, using the contract offsets.
- [x] 7.2 Switch the toolbar button sprite to `SPR_IMG_BUILD_ROAD_ELEVATOR` and the cursor to `SPR_CURSOR_ROAD_ELEVATOR`.
- [ ] 7.3 Check the visibility frames against the new art (the bus disappears behind the door frame, not in front of it) and adjust only the frame constants if needed.

### Area Mandatory Reading - Verification

_Applies to Group 8._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/vehicles/archi-vehicles.md](../../architecture/vehicles/archi-vehicles.md) | 225-228 | `## Invariants`: vehicle motion is deterministic and commands mutate while ticks simulate; the checks below must not introduce GUI-dependent movement. |
| [r2mlm7-layer-portal-foundation.md](r2mlm7-layer-portal-foundation.md) | 63-72 | `## Portal Contract`: the behaviour being verified end to end. |

### Group 8 - Verification

{{MEDIUM}}
- [ ] 8.1 Scenario: surface town road, elevator, underground road loop with a bus stop and a depot, back up through a second elevator; run three buses for one in-game year at fast forward in a Debug build; no assert, no lost vehicle.
- [ ] 8.2 Try to remove an elevator while a bus is inside; confirm the refusal message and that nothing changes.
- [ ] 8.3 Let the owning company go bankrupt in a test game (cheat money to negative); confirm both elevator ends are removed or transferred and no vehicle remains hidden.
- [ ] 8.4 Save and reload the scenario at several moments and compare all plane digests before and after reload.
- [ ] 8.5 Build both configurations without new warnings; run `ctest`.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| A bus jumps or flickers visibly at the shaft edge | [src/roadveh_cmd.cpp](../../../../src/roadveh_cmd.cpp) | The `RDE_NEXT_TILE` block (lines 1194-1359) | The vehicle is not hidden at the moment of relocation | Check the frame at which `VehicleEnterTile_LayerPortal` hides the part. |
| `FatalError("Disconnecting road vehicle.")` after Group 3 | [src/roadveh_cmd.cpp](../../../../src/roadveh_cmd.cpp) | `FollowPreviousRoadVehicle` (lines 1052-1128) | The front and the rear are on different layers | Make sure every part uses the portal-aware tile. |
| Buses queue forever in front of an elevator | [docs/zz-reach2/architecture/pathfinder/archi-pathfinder-road.md](../../architecture/pathfinder/archi-pathfinder-road.md) | `## What one node means` (lines 20-37) | YAPF returns no path through the elevator | Check that the follower's portal branch produces the pair tile with the right trackdir. |
| Articulated vehicles split across layers incorrectly | [docs/zz-reach2/architecture/vehicles/road-vehicles.md](../../architecture/vehicles/road-vehicles.md) | `## One step` (lines 29-44), the curve-frame paragraph | A part ends more than one tile behind the front | Reconcile per-part movement with relocation. |
| Elevator art draws in front of vehicles | [docs/zz-reach2/architecture/gui/archi-viewport.md](../../architecture/gui/archi-viewport.md) | `## Sprite sort` (lines 60-79) | Bounding boxes look right but order is still wrong | Adjust parent sprite boxes. |

## Planning Provenance

This table records what shaped the plan. It is not an execution reading queue, it must not be preloaded after context compaction, it is non-transitive, and its line ranges record planning use rather than runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [src/roadveh_cmd.cpp](../../../../src/roadveh_cmd.cpp) | 347-352, 470-492, 796-865, 887-1128, 1145-1359 | Showed that the next tile is computed in one place and positions come from the new tile's frame table, so relocation is a one-line neighbour change plus visibility. |
| [src/road_gui.cpp](../../../../src/road_gui.cpp) | 537-750, 938-1015, 1191-1279 | Depot button and picker as the GUI template; separate road and tram part lists. |
| [src/road_cmd.cpp](../../../../src/road_cmd.cpp) | 1130-1222, 1714-1898 | Depot build, removal and drawing as the stand-in. |
| [docs/zz-reach2/architecture/vehicles/road-vehicles.md](../../architecture/vehicles/road-vehicles.md) | 1-59 | Controller structure and stop and depot handling. |
| User request | "simple buses that can take an elevator and go underground" | Scope: road only, drive-through elevator, ride as a speed cap instead of a new vehicle state. |

## Completion Conditions

- The road toolbar builds road elevators between Surface and Underground with a chosen door direction.
- Buses and lorries cross layers through elevators, hidden while riding, without visual jumps, asserts or lost vehicles.
- Road pathfinding routes and services through elevators; underground stops and depots work.
- Elevator art is used when the r2mlm6 constants exist; otherwise the depot stand-in remains.
