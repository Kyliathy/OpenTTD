# R2MLM9 - Rail Layer Portals (the Special Train Tunnel)

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
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 98-100 | `### AD-8 Cross-layer movement only through layer portals`: the special train tunnel is a rail portal; trains keep their heading and are relocated at the shaft edge without wormhole state. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 141-170 | `## Engineering Conventions` and `## Build and Test Commands`. |
| [r2mlm7-layer-portal-foundation.md](r2mlm7-layer-portal-foundation.md) | 63-72 | `## Portal Contract`: pair, directions, neighbour mapping, relocation, visibility, tile bits and command signature. This plan adds rail-specific behaviour and must not redefine any of it. |
| [docs/zz-reach2/architecture/vehicles/trains.md](../../architecture/vehicles/trains.md) | 7-46 | `## Tick`, `## Speed` and `## One step`: `TrainLocoHandler` passes, `TrainCheckIfLineEnds`, `TrainController` per-part stepping, `ChooseTrainTrack`, reservation and `CheckCompatibleRail`. Every traversal change hooks into this step. |
| [docs/zz-reach2/architecture/vehicles/archi-vehicles.md](../../architecture/vehicles/archi-vehicles.md) | 225-228 | `## Invariants`: deterministic motion, commands mutate and ticks simulate. Traversal across layers must not depend on view state or anything outside the simulation. |
| [src/layer_portal_map.h](../../../../src/layer_portal_map.h) | Live definitions; no fixed range | Produced by r2mlm7 Groups 1 and 3: `IsLayerPortalTile`, `IsLayerPortalShaftExit`, `GetLayerPortalNextTile`, `GetLayerPortalPairTile`, reservation accessors. Authoritative names. |

## Outcome and Boundaries

- A "Build rail layer portal" button on the rail toolbar with a mouth-direction picker. It builds a rail portal pair (surface descent mouth, underground emergence mouth) for the current rail type between the viewed layer and the other of Surface and Underground.
- Trains of any length drive down or up through a portal, hidden inside the mouth, and continue on underground track heading the same way. Line-end detection, reservations, signals and pathfinding treat the portal as continuous track.
- The player builds track, signals, stations, waypoints and depots underground with the normal rail toolbar while viewing the Underground layer.
- Portal art from r2mlm6 replaces the depot stand-in when the constants exist.

Placement rule: the player picks the mouth direction `mouth` on the viewed layer; the command receives `shaft_dir = ReverseDiagDir(mouth)` and the target `MultiLayerMap::GetPortalTargetLayer` of the viewed layer.

Out of scope: signals on portal tiles, level crossings, catenary drawn on the portal tiles (power still works through the rail type).

## Implementation Tasks

### Area Mandatory Reading - Portal tile and placement GUI

_Applies to Groups 1-2._

Entry gate. r2mlm7 must be complete: `Commands::BuildLayerPortal`, `GetLayerPortalNextTile`, `IsLayerPortalShaftExit`, `TranslateIntoLayerPortalPair`, `MultiLayerMap::GetPortalTargetLayer`, the portal cases in `GetReservedTrackbits` and `ExploreSegment`, and `pf.yapf.rail_layer_portal_penalty` must exist. Verify with a grep for each name. If any is missing, stop every group of this plan and finish r2mlm7 first. Do not add a rail-only portal command, do not route trains through tunnel wormhole code, and do not compute the pair tile locally.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/rail_cmd.h](../../../../src/rail_cmd.h) | 19-39 | Rail command declarations: `CmdBuildSingleRail`, `CmdRemoveSingleRail`, `CmdBuildSingleSignal`, `CmdConvertRail`, the procs that must refuse or handle portal tiles. |
| [src/rail_cmd.cpp](../../../../src/rail_cmd.cpp) | 2291-2436 | `DrawTrackBits`: axis track on barren ground, used by the stand-in drawing of the rail end. |
| [src/widgets/rail_widget.h](../../../../src/widgets/rail_widget.h) | 14-31 | `enum RailToolbarWidgets`: `WID_RAT_BUILD_LAYER_PORTAL` is inserted after `WID_RAT_BUILD_TUNNEL`; hotkeys use widget values. |
| [src/rail_gui.cpp](../../../../src/rail_gui.cpp) | 588-744 | `BuildRailToolbarWindow::GetCursorForWidget`, `GetHighLightStyleForWidget`, `OnClick`, `OnHotkey` and `OnPlaceObject`: the depot button path (`WID_RAT_BUILD_DEPOT`) is the template. |
| [src/rail_gui.cpp](../../../../src/rail_gui.cpp) | 894-955 | `hotkeys` of the rail toolbar and `_nested_build_rail_widgets`. |
| [src/rail_gui.cpp](../../../../src/rail_gui.cpp) | 1789-1866 | `BuildRailDepotWindow` and `ShowBuildTrainDepotPicker`: the direction picker the portal picker is cloned from, with `_build_depot_direction`. |

### Group 1 - Portal tile behaviour

{{MEDIUM}}
- [x] 1.1 In the rail branch of `DrawTile_LayerPortal`, draw the stand-in: axis track of `GetRailType(tile)` on barren ground plus the rail depot building with its opening on the mouth side.
- [x] 1.2 Make `CmdBuildSingleRail` refuse track on a portal tile (`STR_ERROR_MUST_REMOVE_LAYER_PORTAL_FIRST`) and accept building the neighbour's axis track that connects to the mouth.
- [x] 1.3 Make `CmdRemoveSingleRail` on a portal tile fail with the same message (portals are removed with the demolish tool).
- [x] 1.4 Make `CmdBuildSingleSignal` and the signal-drag commands refuse portal tiles and stop a signal drag at a portal.
- [x] 1.5 Make `CmdConvertRail` convert both ends of a portal together, with the compatibility and vehicle checks used for tunnels.
- [x] 1.6 Confirm electric engines draw power on portal tiles through `HasPowerOnRail` with the portal's rail type, and that no catenary or pylons are drawn on portal tiles.

### Group 2 - Placement GUI

Closure, 2026-10-04, under the goal's explicit operator allowance: The complete placement/cost/removal/infrastructure-window operator check is skipped. Existing pairs and the command implementation are not a runtime accounting pass. Each skipped row is marked `[s]`, never a passing test.

{{MEDIUM}}
- [x] 2.1 Add `WID_RAT_BUILD_LAYER_PORTAL, ///< Build a rail portal to another map layer.` after `WID_RAT_BUILD_TUNNEL`, and add the button to `_nested_build_rail_widgets` with `SPR_IMG_TUNNEL_RAIL` as stand-in sprite and `STR_RAIL_TOOLBAR_TOOLTIP_BUILD_LAYER_PORTAL`.
- [x] 2.2 Add `static DiagDirection _rail_portal_mouth_direction` and `BuildRailLayerPortalWindow`, cloned from `BuildRailDepotWindow` with its own widget enum and `WindowDesc` (unique ini key), showing the four mouth directions and the hint that the underground mouth faces the opposite way.
- [x] 2.3 Return the rail type's tunnel cursor (stand-in) and `HT_RECT` for the button in `GetCursorForWidget` and `GetHighLightStyleForWidget`.
- [x] 2.4 In `OnClick`, open the picker; in `OnPlaceObject`, post `Command<Commands::BuildLayerPortal>::Post(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL, CcBuildLayerPortal, tile, TransportType::Rail, target, ReverseDiagDir(_rail_portal_mouth_direction), _cur_railtype, INVALID_ROADTYPE)`.
- [x] 2.5 Take `target` from `MultiLayerMap::GetPortalTargetLayer` of the main viewport's layer and disable the button when it is `MapLayer::Invalid`; refresh when the layer changes.
- [x] 2.6 Add a free hotkey named `layer_portal` to the rail toolbar hotkeys.
- [s] 2.7 Build and place a portal; confirm both ends appear, cost is charged once, removing either end removes both, and the infrastructure window counts both ends.

### Area Mandatory Reading - Train traversal

_Applies to Group 3._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/train_cmd.cpp](../../../../src/train_cmd.cpp) | 3372-3560 | `TrainController` up to the wagon-follow branch: `GetNewVehiclePos`, the same-tile path, and the new-tile path that derives `enterdir` with `DiagdirBetweenTiles` (line 3411) and, for wagons, `DiagdirBetweenTiles(gp.new_tile, prev->tile)` (line 3535). Both return `Invalid` across layers and need the portal branch. |
| [src/train_cmd.cpp](../../../../src/train_cmd.cpp) | 3945-4021 | `TrainApproachingCrossingTile` and `TrainCheckIfLineEnds`: both look ahead with `moving_front->tile + TileOffsByDiagDir(dir)`; at a portal they would see a dead end and stop or reverse the train. |
| [src/train_cmd.cpp](../../../../src/train_cmd.cpp) | 3131-3135 | `CheckCompatibleRail`: owner and rail type check on the new tile; works for portal ends through `GetTileRailType`. |
| [src/train_cmd.cpp](../../../../src/train_cmd.cpp) | 1940-2019 | `AdvanceWagonsBeforeSwap` and `AdvanceWagonsAfterSwap`: reversal moves wagons with `TrainController`, so a reversal while the train straddles two layers depends on the same portal branch. |
| [src/tunnelbridge_cmd.cpp](../../../../src/tunnelbridge_cmd.cpp) | 1974-2000 | Train branch of `VehicleEnterTile_TunnelBridge`: the hide frame, the show frame and the `track` bookkeeping a portal must not copy (portals keep the axis track, never `Track::Wormhole`). |

### Group 3 - Train traversal

Closure, 2026-10-04, under the goal's explicit operator allowance: The straddling-consist reversal operator run is skipped; the supplied-save round trips do not include a reversal at that exact position. Each skipped row is marked `[s]`, never a passing test.

{{HARD}}
- [x] 3.1 In the new-tile path of `TrainController`, when `IsLayerPortalShaftExit(gp.old_tile, DirToDiagDir(v->GetMovingDirection()))`, call `TranslateIntoLayerPortalPair(gp.old_tile, gp.x, gp.y)`, set `gp.new_tile` through `GetLayerPortalNextTile(gp.old_tile, moving)` and `enterdir` to the movement direction instead of `DiagdirBetweenTiles`; port the existing comments. Final closure audit routes this tile step through the shared neighbour helper (I-3), which returns the same pair for the guarded shaft direction.
- [x] 3.2 In the wagon-follow branch, compute the exit direction towards `prev->tile` with a portal-aware helper (the movement direction when the two tiles are a portal pair) instead of `DiagdirBetweenTiles`.
- [x] 3.3 In `TrainCheckIfLineEnds` and `TrainApproachingCrossingTile`, replace `moving_front->tile + TileOffsByDiagDir(dir)` with `GetLayerPortalNextTile(moving_front->tile, dir)`.
- [x] 3.4 Review the remaining `TileAddByDiagDir` look-aheads in `train_cmd.cpp` (lines 2216, 2459, 2896, 2998, 3477) and switch each one that can start on a portal tile to `GetLayerPortalNextTile`; record each decision as a `//` comment.
- [x] 3.5 In the train branch of `VehicleEnterTile_LayerPortal`, apply the contract's hide and show frames for every part and play the tunnel sound for steam engines like the tunnel code; never set `Track::Wormhole`.
- [x] 3.6 After relocation, update each part's Z position and inclination while hidden, and confirm `CheckGroundVehiclesAtCorrectZ` accepts trains on portal tiles after loading.
- [s] 3.7 Reverse a train while it straddles the two layers (front underground, rear on the surface); confirm `AdvanceWagonsBeforeSwap` / `AfterSwap` keep the consist connected and no `Disconnecting train` fatal error occurs.
- [x] 3.8 Final invariant audit: route the movement tile through `GetLayerPortalNextTile` using the already validated `moving` direction; build both configurations and run the existing portal mapping tests and appropriate regression checks. The result must remain the same pair for every shaft direction.

  Verified 2026-10-04: both targets/configurations build with zero compiler warnings (`build/build-neighbour-closure-{Debug,RelWithDebInfo}.log`). Debug passes 111/111 unit cases, including the portal neighbour/relocation tests; full RelWithDebInfo CTest passes 115/115, including all four regression suites (`build/ctest-neighbour-closure-{Debug,RelWithDebInfo}.log`, 12.06 s / 21.79 s). The helper receives `moving`, the direction already checked by `IsLayerPortalShaftExit`, so it returns the same pair for every shaft orientation. Remaining pair-getter calls in these controllers compare connectivity, not movement destinations.

  A final logged Debug run of `makinpr2-connected-graphics-tested.sav` captures 48 frames over 24 seconds: the train emerges onto the Surface, stops at the connected station, enters the portal and comes back. [train-neighbour-smoke.png](train-neighbour-smoke.png) shows selected authentic frames. `build/makinpr2-review/game-final-neighbour.log` exits 0 with no assertion/disconnected-consist error; the desync command log reports no infrastructure/cache mismatch. The normal save/quit writes `build/save/makinpr2-final-verification.sav`. A news message says Road Vehicle #2 has too few orders; the bus order schedule is not changed by this train check. This smoke does not pass the skipped multi-year or reversal/occupied-path matrices.

### Area Mandatory Reading - Reservations and signals

_Applies to Group 4._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/pathfinder/archi-pathfinder-rail.md](../../architecture/pathfinder/archi-pathfinder-rail.md) | 131-140 | `## Reservation`: how PBS reservations are extended and cleared along the follower's path. |
| [src/pbs.cpp](../../../../src/pbs.cpp) | 199-383 | `FollowReservation`, `FollowTrainReservation`, `GetTrainForReservation`: reservation walking with special tunnel and bridge skips (lines 219, 323, 368) that must not misfire on portals. |
| [src/train_cmd.cpp](../../../../src/train_cmd.cpp) | 2434-2469 | `ClearPathReservation`: frees the reservation behind the last wagon; across a portal the next tile is the pair. |
| [src/train_cmd.cpp](../../../../src/train_cmd.cpp) | 2564-2665 | `ExtendTrainReservation`: extends along `CFollowTrackRail`, which is portal-aware since r2mlm7. |
| [src/train_cmd.cpp](../../../../src/train_cmd.cpp) | 2952-3018 | `TryPathReserve`: the origin and first-tile handling that uses `TileAddByDiagDir`. |
| [src/signal.cpp](../../../../src/signal.cpp) | 268-410 | `ExploreSegment` with the portal case from r2mlm7 task 4.5: the block that contains a portal spans both layers. |

### Group 4 - Reservations and signals

Closure, 2026-10-04, under the goal's explicit operator allowance: The occupied block/path-signal operator matrix is skipped. R2MLM13 Group 9C successfully places path signals and runs the connected train across layers, but does not test an occupied path/block on both ends. Each skipped row is marked `[s]`, never a passing test.

{{HARD}}
- [x] 4.1 Confirm `ExtendTrainReservation` and `TryPathReserve` reserve through a portal and set the portal reservation bit on both ends.
- [x] 4.2 Make `ClearPathReservation` use `GetLayerPortalNextTile` when the cleared tile is a portal end.
- [x] 4.3 Make the tunnel and bridge skips in `FollowReservation` and `GetTrainForReservation` ignore portal tiles, and confirm `FollowTrainReservation` finds the end of a reservation that crosses layers.
- [s] 4.4 Confirm a train standing on the underground end occupies the signal block that contains the surface end (and the reverse), using block signals on both sides.
- [s] 4.5 Confirm path signals in front of a portal let a train through only when the path across the portal is free.
- [x] 4.6 In TrainController, make the signal-update side passed to `AddSideToSignalBuffer` correct for a part that just crossed a portal.

### Area Mandatory Reading - Rail pathfinding

_Applies to Group 5._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [docs/zz-reach2/architecture/pathfinder/archi-pathfinder-rail.md](../../architecture/pathfinder/archi-pathfinder-rail.md) | 33-64 | `## Segment cache` and `## Cost walk`: segments follow the track until a junction or signal, costs are cached per segment start, and the cost walk adds per-tile penalties. |
| [src/pathfinder/yapf/yapf_costrail.hpp](../../../../src/pathfinder/yapf/yapf_costrail.hpp) | 136-158 | `CYapfCostRailT::OneTileCost`: per-tile cost; the portal penalty is added when the segment walk crosses from one end to its pair. |
| [src/pathfinder/yapf/yapf_costrail.hpp](../../../../src/pathfinder/yapf/yapf_costrail.hpp) | 295-350 | Start of `PfCalcCost` up to the segment state: where the walk calls the follower tile by tile. |
| [src/pathfinder/yapf/yapf_destrail.hpp](../../../../src/pathfinder/yapf/yapf_destrail.hpp) | 185-196 | Rail `PfCalcEstimate` with the `estimate >= parent->estimate` assert, which must hold across portals now that the estimate is plane-based. |
| [docs/zz-reach2/architecture/pathfinder/archi-pathfinder.md](../../architecture/pathfinder/archi-pathfinder.md) | 280-291 | `## Invariants`: deterministic costs, the cache-versus-no-cache check at desync level and the closed-node consistency rule. |

### Group 5 - Rail pathfinding

Closure, 2026-10-04, under the goal's explicit operator allowance: The year-long cached/uncached search, nearest-depot/safe-wait and reversal operator matrix is skipped. Short supplied-save Debug smoke is recorded separately, with no year-long claim. Each skipped row is marked `[s]`, never a passing test.

{{HARD}}
- [x] 5.1 Add `pf.yapf.rail_layer_portal_penalty` once per traversal in the segment cost walk, with a comment explaining it is charged on leaving the shaft side.
- [x] 5.2 Confirm segment caching across portals: a segment may span both layers; `YapfNotifyTrackLayoutChange` is called on portal build and removal (r2mlm7 tasks 6.6 and 7.3).
- [s] 5.3 Run a Debug build with `-d desync=2` (which runs the cached and uncached searches side by side) on a network of two portals for one in-game year; confirm no mismatch warning.
- [s] 5.4 Confirm `YapfTrainFindNearestDepot` and `YapfTrainFindNearestSafeTile` work through portals, and that a portal tile is never chosen as a safe waiting position.
- [s] 5.5 Confirm `YapfTrainCheckReverse` does not reverse trains in front of a portal mouth.

### Area Mandatory Reading - Underground track building and art

_Applies to Groups 6-7._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [r2mlm6-layer-graphics-assets.md](r2mlm6-layer-graphics-assets.md) | 56-76 | `## Sprite Contract`: `SPR_MAP_LAYER_RAIL_PORTAL_SURFACE` and `_UNDERGROUND` (base + dir * 2 + part), `SPR_IMG_BUILD_RAIL_LAYER_PORTAL`, `SPR_CURSOR_RAIL_LAYER_PORTAL`. |
| [src/table/sprites.h](../../../../src/table/sprites.h) | Live definitions; no fixed range | Produced by r2mlm6 Group 6. If the constants are absent, skip Group 7 and keep the stand-in; never add placeholder constants. |
| [docs/zz-reach2/architecture/landscape/rail-tiles.md](../../architecture/landscape/rail-tiles.md) | 51-74 | `## Ground under the rail`: ground types; underground track stays `RailGroundType::Barren` (r2mlm4 task 3.3). |

### Group 6 - Underground track building

Closure, 2026-10-04, under the goal's explicit operator allowance: The complete underground construction, naming/payment, servicing/autoreplace and vehicle-follow operator matrix is skipped. Group 9C of R2MLM13 proves connected platforms and station stops in the supplied game; it does not pass every row here. Each skipped row is marked `[s]`, never a passing test.

{{MEDIUM}}
- [s] 6.1 In the Underground view, build plain track, diagonal track, autorail, signals, a waypoint, a rail station and a train depot; confirm each succeeds and appears only underground.
- [s] 6.2 Confirm the underground rail station takes its name and town from the town above and shows in the station list.
- [s] 6.3 Buy a train in the underground depot, give it orders to a surface station through a portal, and confirm it loads, unloads and pays by plane distance.
- [s] 6.4 Confirm servicing, "go to depot" and autoreplace across layers.
- [s] 6.5 Confirm the vehicle view follows a train into the Underground layer and back, and that the train's location button switches the main view.

The R2MLM6 sprite constants and art are now integrated; the obsolete missing-constants note is superseded by Group 7's delivery record.

### Group 7 - Portal art

Closure, 2026-10-04, under the goal's explicit operator allowance: The every-direction entering-train and monorail/maglev operator checks are skipped. R2MLM13 records all existing rail-end static captures and a Surface-SW traffic burst after the front repair; that subset is not the full matrix. Each skipped row is marked `[s]`, never a passing test.

2026-10-03: r2mlm6 publishes the portal constants and all four directional back/front pairs. Drawing and toolbar/cursor swaps are implemented and build in Debug. Rail-type sprites and reservation tint are retained. Visibility and monorail/maglev runtime checks wait for the user's representative save.

{{SIMPLE}}
- [x] 7.1 Draw the portal back part as a ground child sprite and the front part (the mouth arch) as a parent sprite with a bounding box that lets the train pass beneath, for both ends, using the contract offsets.
- [x] 7.2 Switch the toolbar sprite to `SPR_IMG_BUILD_RAIL_LAYER_PORTAL` and the cursor to `SPR_CURSOR_RAIL_LAYER_PORTAL`.
- [s] 7.3 Check that the train disappears inside the mouth (not in front of it) for each direction and adjust only the visibility frames if needed.
- [s] 7.4 Check monorail and maglev portals: the track drawn on the portal tile uses the portal's rail type sprites.

### Area Mandatory Reading - Verification

_Applies to Group 8._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [r2mlm7-layer-portal-foundation.md](r2mlm7-layer-portal-foundation.md) | 63-72 | `## Portal Contract`: the behaviour being verified end to end. |
| [docs/zz-reach2/architecture/desync/archi-desync.md](../../architecture/desync/archi-desync.md) | 166-172 | `### Cache checks at desync level 2`: the determinism smoke test used in task 8.5. |

### Group 8 - Verification

Closure, 2026-10-04, under the goal's explicit operator allowance: The multi-train two-year run, straddling save, occupied/reserved removal, conversion and year-long cache operator scenarios are skipped. Rail-pair conversion/accounting code is fixed by R2RVM2 task 18.8 and tested at build/unit level, without a conversion-scenario pass. Each skipped row is marked `[s]`, never a passing test.

{{MEDIUM}}
- [s] 8.1 Scenario: surface main line with a station, a portal down, an underground loop with a station, block and path signals, a second portal back up; run four trains of different lengths for two in-game years at fast forward in a Debug build; no assert, no stuck or lost train.
- [s] 8.2 Save and reload while a long train straddles both layers; confirm it continues and every plane digest is stable across a second save.
- [s] 8.3 Try removing a portal with a train on it and with a reservation through it; confirm both are refused and nothing changes.
- [s] 8.4 Convert the portal's rail type with trains present and absent; confirm the tunnel-like rules apply.
- [s] 8.5 Run the scenario with `-d desync=2` for one in-game year; confirm no cache-check or YAPF cache mismatch messages.
- [x] 8.6 Build both configurations without new warnings; run `ctest`.

  Verified 2026-10-04: both targets/configurations build with exit 0, zero compiler warnings; both unit runs pass 110/110. Evidence: `build/build-portal-rail-conversion-{Debug,RelWithDebInfo}.log`, `build/ctest-portal-rail-conversion-{Debug,RelWithDebInfo}.log`. Rail conversion now also includes the r2rvm2 task 18.8 pair-accounting/consist fixes. Vehicle traversal, conversion and visual checks remain pending the representative save.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| `FatalError("Disconnecting train")` after Group 3 | [src/train_cmd.cpp](../../../../src/train_cmd.cpp) | The wagon-follow branch of `TrainController` (around line 3510) | A wagon's `prev` is on the pair tile | Fix the portal-aware exit direction for wagons. |
| A train brakes or reverses in front of a portal mouth | [docs/zz-reach2/architecture/vehicles/trains.md](../../architecture/vehicles/trains.md) | `## Speed` (lines 27-36), the line-end paragraph | `TrainCheckIfLineEnds` already uses the portal neighbour | Find another look-ahead that still uses a plain neighbour. |
| Reservations stay behind after a train passes a portal | [docs/zz-reach2/architecture/landscape/rail-tiles.md](../../architecture/landscape/rail-tiles.md) | `## Path reservation` (lines 75-83) | The leftover bit is on a portal end | Check which accessor `ClearPathReservation` reaches. |
| YAPF hits `NOT_REACHED` on a closed node | [docs/zz-reach2/architecture/pathfinder/archi-pathfinder.md](../../architecture/pathfinder/archi-pathfinder.md) | `## Node expansion` (lines 178-246) | The estimate is plane-based already | Look for a negative or missing step cost at the portal. |
| A desync appears in multiplayer with trains crossing layers | [docs/zz-reach2/architecture/desync/archi-desync.md](../../architecture/desync/archi-desync.md) | `## What is compared` (lines 45-118) | The dump shows vehicle state divergence | Diagnose; multiplayer acceptance belongs to r2mlm10. |

## Planning Provenance

This table records what shaped the plan. It is not an execution reading queue, it must not be preloaded after context compaction, it is non-transitive, and its line ranges record planning use rather than runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [src/train_cmd.cpp](../../../../src/train_cmd.cpp) | 1940-2019, 2210-2346, 2434-3018, 3131-3135, 3372-3686, 3945-4021 | Inventory of every place a train looks at the next tile, which defined Groups 3 and 4. |
| [src/pbs.cpp](../../../../src/pbs.cpp) | 18-383 | Reservation helpers and their tunnel special cases. |
| [src/rail_gui.cpp](../../../../src/rail_gui.cpp) | 588-955, 1789-1866 | Rail toolbar depot button and picker as the GUI template. |
| [src/pathfinder/yapf/yapf_costrail.hpp](../../../../src/pathfinder/yapf/yapf_costrail.hpp) | 22-350 | Segment cost walk; where the portal penalty belongs. |
| User request | "trains which should enter a special tunnel, represented by a new graphical asset ... where the player can continue building tracks" | Scope: a dedicated rail portal with its own art, drive-through, and full track building underground. |

## Completion Conditions

Closed 2026-10-04 with the operator skips recorded in each affected group above. Coding and automated checks are delivered: zero compiler warnings, 111/111 unit cases in both configurations and 115/115 full release CTest cases (`build/build-console-input-*.log`, `build/ctest-console-input-*.log`, `build/ctest-console-final-RelWithDebInfo.log`). The original full runtime criteria below remain future validation criteria; closure under the goal allowance does not mean skipped scenarios passed.

- The rail toolbar builds rail portals between Surface and Underground for any rail type, with a chosen mouth direction.
- Trains of any length cross layers through portals, hidden inside the mouth, with correct reservations, signals and pathfinding, and survive save, load and reversal mid-portal.
- Track, signals, stations, waypoints and depots can be built underground and used by trains.
- Portal art is used when the r2mlm6 constants exist; otherwise the stand-in remains.
