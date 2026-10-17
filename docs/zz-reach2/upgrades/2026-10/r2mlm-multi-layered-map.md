# R2MLM Master Plan - Multi-Layered Map (Surface, Underground, Deep Underground, Outer Space)

This plan summarises the multi-layered map epic. It holds no tasks: every task lives in the plan that owns it, listed in `## Plan Index`. Its Architecture Decisions and Cross-plan Invariants are the canonical authority every plan consumes. A plan's acceptance checks test it against them, not against the plan's own restatement. Where a plan contradicts an AD, the AD wins unless the user approves a change.

Until 2026-10-03 this file also held checkpoint tasks. Group 1 (toolchain and baseline) was done, and its results are in `## Build and Test Commands` and `## Baseline Data`. Groups 2 to 8 ran each plan to its Completion Conditions and repeated checks those plans already hold. The one check no plan held, a combined bus and train run across a save, is now r2mlm10 task 2.8. The removed text is in commit `d4abc0a7dc`.

**Artifact-layout migration (2026-10-09):** this plan consumes the parent [Development artifact layout](r2mlvs-multilayer-vehicles-and-structures.md#development-artifact-layout), implemented by [R2FC](r2fc-file-cleanup.md). It supersedes earlier docs asset/review/helper placement and fixed runtime-config destinations below. New native homes use `mlm/runtime/`, generated feature reviews use `mlm/reviews/`, logs remain under `logs/`, and reusable code/curated inputs keep their code/test owners. Paths and focused native/cold verification are complete; R2FC records the results and retained-input identities. Repository-relative executable commands run from the checkout root through their context-aware wrappers. Historical pass records remain dated evidence and are not new acceptance claims. No gameplay task is completed by this migration and the separate post-demo fixture-pruning gate remains.

## Outcome

OpenTTD gains three extra maps that sit under and above the existing land map. The player chooses a layer from the toolbar button next to the map button. Road tunnels connect Surface and Underground; road elevators connect either to Deep Underground. Trains drive through a special tunnel onto tracks the player builds underground. Every layer is simulated every tick, saved, loaded, and kept in sync in multiplayer.

## Key Points

1. **Layer button.** The main toolbar button immediately to the right of the map button opens Surface, Underground, Deep Underground and Outer Space in that display order. The scenario editor, extra viewport and smallmap buttons use the same dropdown for their own views. Each button shows its view's icon; `N` still cycles the main view.
2. **Independent maps under a multi-layered map controller.** Underground, Deep Underground and Space are complete, independent maps with their own terrain, rules and drawing. `MultiLayerMap` sits above the existing `Map` controller, owns the set of layers, and provides layer semantics.
3. **Buses take tunnels and elevators.** Road tunnels join Surface to Underground at normal speed. Road elevators join either upper layer to Deep Underground; buses stop hidden for 370 game ticks and leave from standstill. The road pathfinder routes through both kinds and charges elevators for their wait.
4. **Trains take a special tunnel.** A rail layer portal, drawn with a new graphical asset, carries trains from surface track to underground track. The player continues building track, stations, depots and signals underground.
5. **Graphical assets.** Layer icons, underground and space ground, elevators and portals are generated with AI image calls and packed into OpenTTD's own extra GRF.
6. **Acceptance.** Savegames, multiplayer determinism, performance and documentation are verified for the whole epic.

## Plan Index

Status: `[X]` every required task is complete and each authorised skip is recorded, `[~]` work remains, `[ ]` none is yet. Task marker `[s]` means explicitly skipped under the goal's human-operator/gap allowance, never a passing test. Reconciled on 2026-10-04: code, generated assets and automated checks are delivered; each remaining operator/review gate is explicitly skipped at its owning group. Closure under this allowance does not claim full runtime, multiplayer, art or performance acceptance. Whoever closes a plan's last task sets its box here to `[X]`.

| Status | Plan | Point | Delivers | Depends on |
| --- | --- | --- | --- | --- |
| [X] | [r2mlm1-layer-toolbar-button.md](r2mlm1-layer-toolbar-button.md) | 1 | `MapLayer` vocabulary, `Viewport::layer`, the layer button next to the map button in the main and scenario-editor toolbars | none |
| [X] | [r2mlm2-layer-addressing-and-controller.md](r2mlm2-layer-addressing-and-controller.md) | 2 | Stacked-plane addressing, `Map` extension, `MultiLayerMap` core, plane distances, unit tests | r2mlm1 |
| [X] | [r2mlm3-layer-call-site-migration.md](r2mlm3-layer-call-site-migration.md) | 2 | Classification and migration of every map-geometry call site, surface digest check | r2mlm2 |
| [X] | [r2mlm4-layer-world-lifecycle-and-savegame.md](r2mlm4-layer-world-lifecycle-and-savegame.md) | 2 | Layer rules, plane initialisers, per-layer tile loop, construction gating and savegame; three-layer staging extended to four by r2mlm12 | r2mlm3 |
| [X] | [r2mlm5-layer-viewport-switching.md](r2mlm5-layer-viewport-switching.md) | 1, 2 | Viewport bands, picking, filtering, layer ground, smallmap per layer, real switching behind the button | r2mlm4 |
| [X] | [r2mlm6-layer-graphics-assets.md](r2mlm6-layer-graphics-assets.md) | 5 | AI-generated icons, ground, elevator and portal sprites in `openttd.grf` | r2mlm1; runs in parallel with r2mlm2 to r2mlm5; **Groups 3 and 4 need an image-generation harness** |
| [X] | [r2mlm7-layer-portal-foundation.md](r2mlm7-layer-portal-foundation.md) | 3, 4 | `TileType::LayerPortal`, pair command, neighbour contract, follower, signal and reservation integration | r2mlm5 |
| [X] | [r2mlm8-bus-elevators.md](r2mlm8-bus-elevators.md) | 3 | Road elevators: placement GUI, road vehicle traversal, pathfinding, underground bus routes | r2mlm7 |
| [X] | [r2mlm9-rail-layer-portals.md](r2mlm9-rail-layer-portals.md) | 4 | Rail portals: placement GUI, train traversal, reservations, signals, underground track building | r2mlm7 |
| [X] | [r2mlm10-multi-layer-acceptance.md](r2mlm10-multi-layer-acceptance.md) | 6 | Automated save/digest and regression checks, invariant records and architecture documentation; full operator acceptance explicitly skipped | r2mlm8, r2mlm9, r2mlm13 |
| [X] | [r2mlm11-split-toolbar-subsidies.md](r2mlm11-split-toolbar-subsidies.md) | 1 | A Subsidies entry in the Towns menu whenever the Subsidies button is off screen in a split toolbar (review finding L-2) | r2mlm1 |
| [X] | [r2mlm12-deep-underground-tunnels-and-elevators.md](r2mlm12-deep-underground-tunnels-and-elevators.md) | 1, 3 | Deep Underground as a fourth layer (savegame 370), road tunnels to the Underground, timed road elevators to Deep Underground, layer dropdowns on all four layer buttons; original stand-ins replaced by r2mlm13 | r2rvm2, r2mlm6, Group 7 (art) of r2mlm8 and r2mlm9 |
| [X] | [r2mlm13-layer-art-review-and-new-sprites.md](r2mlm13-layer-art-review-and-new-sprites.md) | 5 | Generated 66-sprite development set, supplied-save foreground repairs, extended sprite/capture tools; full gallery and human art approval explicitly skipped | r2mlm12; **Groups 6 to 8 need an image-generation harness** |

The review plans check the epic's code against this plan and fix what they find. They belong to no point.

| Status | Plan | Delivers | Depends on |
| --- | --- | --- | --- |
| [X] | [r2rvm-review-map.md](map-layers/r2rvm-review-map.md) | The review findings (C-1 to L-12) and fix decisions (FD-1 to FD-12), the pre-epic baseline, the epic build, the regression run and the local fixes (Groups 1 to 7) | r2mlm1 to r2mlm10 written |
| [X] | [r2rvm2-review-map-2.md](map-layers/r2rvm2-review-map-2.md) | Groups 8 to 18 of the same review: portal rules, vehicle controllers, signal walk, ownership, display layer, command guards and strings, then scenarios S1 to S7 with the user and the closure | r2rvm |

Order:

- r2mlm1, r2mlm2, r2mlm3, r2mlm4, r2mlm5 and r2mlm7 run in that order. r2mlm8 and r2mlm9 may run in either order after r2mlm7.
- r2mlm6 and r2mlm11 may run any time after r2mlm1. Art from r2mlm6 is optional for r2mlm1 to r2mlm11: each consumer has a stand-in and an explicit swap task.
- r2mlm12 was done on 2026-10-04 with its original stand-in stage superseded by r2mlm13. r2mlm13 was done on 2026-10-04 with the 66-sprite development set and supplied-save repairs. r2mlm10's automated acceptance records run last; the goal authorises skipping human/operator gates, each recorded at its owning group. Full gallery, art approval, multiplayer, traffic and performance acceptance remain future validation rather than passing results.

## Execution Notes

- **Coding and test staging, 2026-10-03.** The user requested completing code and generated assets now, with in-game testing postponed until they provide a representative save. Runtime, scenario and human-review boxes stay open. Earlier plans remain partial while those checks are pending; completed code and build tasks are checked with their evidence. Coding for later plans may be staged after their code/art prerequisites exist, without claiming the pending runtime or human-review prerequisites passed.

- **Pause, 2026-10-04.** At the user's request, finish r2mlm13 Group 2 and pause with compilable Debug and RelWithDebInfo builds. Resume with capture tooling (Group 3) and the user's representative savegame. Do not launch the game during this coding stage.

- **Goal resumed, 2026-10-04.** The active goal now authorises continuing code, generated art and automation past human-operator tasks and gates. Each affected group records exactly which operator work was skipped; a skip is never evidence of a passing runtime or art review. Missing gallery saves do not block capture-tool implementation, offline asset checks or integration. The previous goal turn made progress: it delivered the extended sprite pipeline and verified both build configurations. Unit checks can run without the user's save. Keep acceptance evidence separate from skipped operator work.

- **Paused again at the user's request, 2026-10-04.** Automatic goal continuation resumed after the earlier pause; the user requested another pause to create the savegame. Both targets build in Debug and RelWithDebInfo with zero compiler warnings, and both configurations pass 110/110 unit cases (`build/build-capture-*.log`, `build/ctest-capture-*.log`). The integrated graphics remain the 44-sprite set. R2MLM13 Group 3 capture tooling is delivered; Group 6 has three new raw candidates, not yet fitted, reviewed or wired. Resume from Group 6 after the user resumes the goal; use the representative save for the pending runtime/art work.

- **Building resumed by the user, 2026-10-04.** Continue while the user prepares the savegame. R2MLM13 now integrates offsets 44..65: Deep Underground icon/floors, road tunnel icon/shared-raw cursor, and eight whole-picture road tunnel ends with cut fronts. Original offsets 0..43 still replay byte for byte through `--legacy44`. Pickers draw the generated assets. The new floors use native tile coverage and show no transparent seams in their offline patches. Both configurations build without compiler warnings and pass 110/110 unit cases; the last art-only rebuild also passes (`build/build-projected-floor-*.log`, `build/build-portal-rail-conversion-*.log`, `build/ctest-portal-rail-conversion-*.log`). Offline review boards are `mlm/reviews/map-layers/legacy-review/import-20261009/images/r2mlm13-d-replaced.png` and `mlm/reviews/map-layers/legacy-review/import-20261009/images/r2mlm13-d-new.png`; they are development picks, not human approval. Game zooms, vehicle occlusion, game warnings and runtime acceptance await the save.

- **Completed-task reconciliation, 2026-10-04.** Completed build/unit tasks in r2mlm2..r2mlm9 and r2mlm11 are checked against the final 110-case runs. Review coding statuses are reconciled separately from pending scenarios. R2RVM2 task 18.8 fixes L-13 (far-end train refresh) and M-12 (rail pair validation/accounting); both builds and unit suites pass. Plans with pending runtime/human acceptance remain `[~]`, so closure does not imply tests or user decisions that have not happened.

- **Representative save supplied, 2026-10-04.** `build/save/makinpr2.sav` is version 370 with four 256x256 planes; original SHA256 `090f204f01cbbbd87cc5c1ae488fd06597bf079e2a8a7959d97e44dc1db4e30a`. It loads in Debug. Runtime review resumes for the reported station/signal and housing-orientation issues; test changes use a separate save. R2MLM13 Group 9B repairs eight far-facing fronts from their existing generated backs. Both configurations build without compiler warnings and pass 110/110 unit cases (`build/build-front-coverage-*.log`, `build/ctest-front-coverage-*.log`). Human art approval and remaining scenarios stay open.

- **Representative-save fixes and diagnosis, 2026-10-04.** R2MLM13 Group 9C repairs the eight near-facing fronts as well, retaining generated door frames and covering native track overlays with the solid roof/walls. Debug captures confirm the repair; both configurations build with zero compiler warnings. The source and copied GRF match SHA256 `3817870b424be800cbf0bcee7a922f7f47e933be2d69594b2d5b9e4a8e17e115`. The original 44-row replay is unchanged. The save's rail platforms were disconnected on row 197 beside the running track on row 198. A separate `build/save/makinpr2-connected.sav` adds connected platforms to the same stations and path signals; the train then stops and makes cross-layer round trips. No station/pathfinder change was needed for that report. Original save preserved. R2MLM13 records 63 static captures, traffic captures and the remaining gallery coverage gaps; full acceptance and human art approval remain open.

- **Toolbar and saved-camera verification, 2026-10-04.** R2MLM1 and R2MLM11 are closed with explicit operator skips. Debug verifies both pages at every main-toolbar split width, the full row, editor arrangements and four-layer controls. R2MLM11's lower-page menu fallback works; its upper-page tests at widths 14..16 requested a Towns button absent from the approved layouts, so those cases are skipped rather than claimed as passes. F6 remains available there. R2MLM1 Group 7B corrects VIEW camera coordinates so a non-surface save reloads onto Surface at the same plane position. Both configurations build without compiler warnings and pass 110/110 unit cases (`build/build-camera-save-*.log`, `build/ctest-camera-save-*.log`). Saves from all three non-surface layers, plus the portal/station fixture, preserve all ten map chunks across paused round trips. Captures and exact evidence are recorded in those plans. Full epic runtime acceptance remains open.

- **Final baseline and regression evidence, 2026-10-04.** R2RVM2 task 18.1 regenerates all four baseline worlds with the final four-layer build: every Surface digest matches the immutable version-368 reference. All four upgrades and reloads preserve all ten complete map chunks. Exact lines are in `docs/zz-rhr/upgrades/2026-10/r2fc-f-retained-records.md#final-layer-save-digests`. Group 18B fixes an empty-console-input assertion found by legacy testing; both configurations build without compiler warnings and pass 111/111 unit cases, and the full release CTest run passes 115/115 including all four regression suites. The TTD scenario used by the plans loads, plays 31 days and round-trips four planes in Debug. Actual TTO name-conversion failures reproduce in the pre-epic build and are recorded below. R2RVM2 is closed under the goal's allowance, with user scenarios and decisions explicitly skipped rather than passed. Full epic acceptance remains open.

- **Plan closure, 2026-10-04.** All thirteen implementation plans and both review plans are closed under the goal's explicit operator/gate allowance. Every completed task has its evidence and every skipped operator task has a note at its group; `[s]` is never a passing test. The [closure audit](../../../zz-rhr/upgrades/2026-10/r2fc-f-retained-records.md#historical-plan-closure) counts 621 completed tasks, 129 explicit skips and zero pending boxes across 15 plans. The final invariant audit routes the train movement destination through `GetLayerPortalNextTile` with its validated direction. Both configurations build with zero compiler warnings; Debug passes 111/111 unit cases and full release CTest passes 115/115 (`build/build-neighbour-closure-*.log`, `build/ctest-neighbour-closure-*.log`). A final 48-frame/24-second Debug train smoke confirms Surface emergence, station stop, entry and return; it exits normally with no assertion or cache mismatch (`build/makinpr2-review/game-final-neighbour.log`, `mlm/reviews/map-layers/legacy-review/import-20261009/images/train-neighbour-smoke.png`). All games and tests are closed and the startup script is restored empty. The original supplied save retains its SHA256. Use `build/save/makinpr2-connected.sav` for the connected-platform/path-signal test setup, or `makinpr2-final-verification.sav` for the latest tested traffic state. Full human/operator acceptance remains unverified as recorded in the plans.

- **Image generation.** r2mlm6 Groups 3 and 4 and r2mlm13 Groups 6 to 8 make sprites with an AI image tool. This Codex harness supports generation from prompts and reference images and has generated the integrated 66-sprite development set. Final picks still await human art approval. Every other group runs anywhere. Both plans say this where those groups start.
- **Groups that need the user.** r2rvm2 Group 17 and r2mlm12 Group 10 are scenario runs with the user. r2mlm13 tasks 5.4 and 8.2 ask the user to judge and approve the art. r2rvm2 Group 18 and r2mlm12 task 11.7 ask the user for decisions.
- **Running the game.** Tell the user before starting `openttd.exe`. Start it from `build/` with a `-c` config in `build/`. Screenshots are PrintWindow captures of the game window, and input is posted to the game window only, never sent through the user's mouse or keyboard. `mlm/tools/run/ottd_window.ps1` does both.

## Architecture Decisions

### AD-1 Layer vocabulary (owner: r2mlm1) (amended by r2mlm12, 2026-10-03)

`enum class MapLayer : uint8_t { Surface = 0, Underground = 1, Space = 2, DeepUnderground = 3, End, Invalid = 0xFF }` lives in `src/map_layer_type.h`, with `MAX_MAP_LAYERS = to_underlying(MapLayer::End)` (4). The numeric values are the storage order and are implicit in every saved `TileIndex`, so they are never reordered. Deep Underground is appended to preserve version-369 saves. Display order is Surface, Underground, Deep Underground, Outer Space, defined separately in `src/map_layer_gui.cpp` alongside `GetMapLayerName`, `GetMapLayerIcon` and `BuildMapLayerDropDown`.

### AD-2 Stacked-plane addressing (owner: r2mlm2)

All layers share the one existing `TileIndex` space. Layer `L` is a full plane of `Map::SizeX() x Map::SizeY()` tiles stored at `[L * Map::PlaneSize(), (L + 1) * Map::PlaneSize())`. Because `TileXY(x, y) = (y << LogX) + x`, this is the same as stacking the planes along the Y axis.

- `TileY(t)` keeps its formula and returns the **stacked** Y (`L * SizeY + plane_y`). `TileXY(TileX(t) + dx, TileY(t) + dy)` therefore stays in the layer of `t`.
- `TilePlaneY(t)` returns `plane_y`, `GetTileLayer(t)` returns `L`, and `TileXYL(x, plane_y, L)` builds a tile.
- World coordinates are stacked the same way (`y_pos` includes the layer band), so `TileVirtXY`, the vehicle tile hash, the vehicle viewport hash, signs and the k-d trees keep working without a layer field.
- Each plane keeps its own void fence: `IsInnerTile` tests `TilePlaneY`. One-tile neighbour steps from a non-void tile never leave its layer.

Rejected alternatives: swapping an "active layer" tile array during the tick (every layer must simulate every tick, pools store raw `TileIndex`, every command would need a layer argument); a separate layer field with plane-local `TileY` (every existing `TileXY(x, y)` call would silently fall back to the surface); a per-tile "underground" flag in one plane (layers would not be independent maps and the view could not switch maps).

### AD-3 Two controllers (owner: r2mlm2; view part r2mlm1 and r2mlm5)

- `Map` (`src/map_func.h`) stays the low-level storage and geometry controller. `Map::Allocate(size_x, size_y, layer_count)`. `Map::Size()` becomes the tile count of all layers (storage bound, `IsValidTile` bound, `Map::Iterate()` range). `Map::PlaneSize()` is one layer. `Map::SizeX/SizeY/MaxX/MaxY/LogX/LogY` stay per plane. `Map::LayerCount()` reports allocated layers.
- `MultiLayerMap` (`src/map_layer.h`, `src/map_layer.cpp`) is the multi-layered map controller and the only API for layer semantics: `GetLayerCount`, `IsValidLayer`, `IterateLayer`, `TileInLayer`, `ProjectToSurface`, `RandomTileInLayer`, `GetRules` and `InitialiseLayer`. `RunTileLoop` in `src/landscape.cpp` uses `GetLayerBase` and `TileInLayer` to apply the shared plane cursor to each allocated layer; the tile procs obey the layer rules. Gameplay code never adds `Map::PlaneSize()` to a tile by hand.
- Switching maps for the player is view state owned by `src/map_layer_gui.cpp`: `SetViewportLayer(Window *w, MapLayer layer)` and `CycleMainViewportLayer()`.

### AD-4 Independent layers (owners: r2mlm4, r2mlm5)

Each layer is an independent map: own void fence, own `MapLayerRules` (allowed construction, terraform, water, trees, towns, industries, climate, flat base height, ground style), own plane initialiser, own ground drawing, own smallmap view and own viewport band. Surface generation is unchanged: for the same seed it produces a bit-identical surface plane. Initialising any non-surface layer never calls `Random()`.

### AD-5 Distances are plane distances (owners: r2mlm2, r2mlm3)

`DistanceManhattan`, `DistanceSquare`, `DistanceMax` and `DistanceMaxPlusManhattan` measure in the plane (`TilePlaneY`). Portals are vertical, so plane distance is the true travel distance and cargo payment cannot be inflated by a layer offset. Code that needs "same layer" tests `GetTileLayer` equality explicitly. YAPF estimates (`OctileDistanceCost`) also use plane coordinates: a stacked-Y estimate overestimates across a portal and trips the closed-node consistency `NOT_REACHED` in `yapf_base.hpp`. Town association (nearest town, local authority) projects to the surface with `MultiLayerMap::ProjectToSurface`.

### AD-6 Every layer is simulated (owner: r2mlm4)

`CallVehicleTicks` already walks the whole vehicle pool. `RunTileLoop` applies the shared LFSR cursor `_cur_tileloop_tile` to each layer plane in turn, so no new saved state is needed. Climate, flooding and growth effects run only where `MapLayerRules` allow them.

### AD-7 Viewing a layer is view state (owners: r2mlm1, r2mlm5)

`Viewport::layer` selects which band a viewport draws and picks. It is client state: commands and tick code never read it (desync invariant 3). Each layer button chooses its view's layer; `N` cycles the main viewport through allocated layers in display order.

The selected layer is not saved. VIEW camera metadata is projected back into the Surface band before saving, so loading starts on Surface at the same plane position and zoom. This GUI metadata does not affect the saved simulation or which map planes are stored.

### AD-8 Cross-layer movement only through layer portals (owner: r2mlm7) (amended by r2mlm12, 2026-10-03)

`TileType::LayerPortal` (value 11) is a pair of tiles at the same plane (x, y), built and removed together by `Commands::BuildLayerPortal`. `MultiLayerMap::IsPortalPairAllowed` permits rail and road tunnels between Surface and Underground, and road elevators between Deep Underground and either upper layer. Numeric layer adjacency is irrelevant. `GetLayerPortalKind` derives these three kinds from transport and layers; no kind bit is stored. Each end has one door side and one shaft side, and the pair's shaft direction is reversed. Leaving through the shaft enters the pair through its shaft, preserving heading. `GetLayerPortalNextTile(tile, exitdir)` is the single neighbour function used by followers, vehicle controllers and the signal walk. Traversal is a tile-edge relocation without wormhole state; visibility toggles at door frames. Tunnels keep normal speed; road elevators stop for 370 ticks before committing that step.

### AD-9 Savegame (owner: r2mlm4)

`SaveLoadVersion::MapLayers` (369) adds the `MAPS` `layers` field. The RIFF planes `MAPT` to `MAP8` store every layer in `TileIndex` order. Files without the field load as one surface plane, and `AfterLoadGame` initialises missing planes. `LayerPortal` tiles only exist at or after version 369. `SaveLoadVersion::DeepUnderground` (370) saves `RoadVehicle::elevator_wait` and permits four stored layers; older files claiming four layers are rejected.

### AD-10 Graphics (owner: r2mlm6; extended by r2mlm13, 2026-10-04)

New sprites live in OpenTTD's extra GRF (`media/baseset/openttd`) as Action 5 type `0x1C` "Map layer graphics", mapped to `SPR_MAP_LAYER_BASE` in `src/table/sprites.h`. Images are generated with an AI image tool and quantised to the DOS palette by a repeatable script. The original r2mlm6 contract continues to offset 65 in r2mlm13's `## Sprite Contract Extension`; added road tunnel backs and fronts are cut from one whole picture per door (D-1). The original 44 rows remain replayable through `LEGACY_MANIFEST`. All sixteen original live fronts now take solid roof/wall coverage from their generated back panel; near-facing fronts also retain the aligned doorway frame. The live count is 66.

### AD-11 Behaviour-neutral staging (owners: r2mlm2 to r2mlm4)

r2mlm2 and r2mlm3 keep live games at `Map::LayerCount() == 1` and exercise three layers only in unit tests. r2mlm4 switches new games and loaded games to `MAX_MAP_LAYERS`. Each earlier plan can therefore merge without changing gameplay.

### AD-12 Version-1 scope (amended by r2mlm12, 2026-10-03)

In scope: four layers; choosing the view; road tunnels and rail portals between Surface and Underground; road elevators from either upper layer to Deep Underground; rail, road, stations, depots and signals in Underground. Deep Underground permits roads, road stops, road depots, signs and elevators, with no trains or rail construction. Out of scope (see `## Deferred Points`): construction in Space beyond signs; trams, ships and aircraft crossing layers; towns, houses, industries, trees or water off Surface; underground station catchment; script API and NewGRF layer awareness.

## Cross-plan Invariants

- **I-1** `t == GetTileLayer(t) * Map::PlaneSize() + TilePlaneY(t) * Map::SizeX() + TileX(t)` for every tile.
- **I-2** One-tile steps (`TileAddByDiagDir`, `TileAddByDir`) from a non-void tile stay in its layer.
- **I-3** During simulation only `GetLayerPortalNextTile`, built on `MultiLayerMap::TileInLayer`, yields a tile in another layer.
- **I-4** `Viewport::layer` and other view state never influence commands, ticks or `Random()`.
- **I-5** `Map::Size()` bounds the storage of all layers; per-plane logic uses `Map::PlaneSize()` and `TilePlaneY()`.
- **I-6** The same seed yields an identical surface plane before and after the epic.
- **I-7** A save written by this epic round-trips every layer; a pre-epic save loads, gains the missing planes, and plays.
- **I-8** Every `TileType::LayerPortal` end has a pair whose shaft direction is reversed.

r2mlm10 task 6.3 records the evidence for each invariant.

## Deferred Points

These were considered and deliberately left out of the 2026-10 plans. A later epic should start from here.

- Construction and units in Space (space elevator, rockets, orbital stations).
- Metro catchment: underground stations serving the houses directly above them.
- Trams through elevators, ships and aircraft across layers.
- Script API (`ScriptMapLayer`) and NewGRF variables that expose the layer.
- Changing the layer count of an existing game beyond the `AfterLoadGame` upgrade.
- A rail elevator and trains in Deep Underground.
- A rule for the Underground tile beneath a Surface-to-Deep elevator; currently that independent tile remains untouched.
- A setting for the 370-tick elevator wait and its pathfinder cost (currently about 10 tiles).
- Sprites for other zoom levels and 32bpp layer art; the current generated set uses one normal-zoom 8bpp DOS-palette contract.
- Open review decision M-9 (r2rvm2 task 18.2): user acceptance of full-plane memory/tick costs, now four planes after r2mlm12. No mitigation or acceptance is inferred while the operator decision is pending.
- Open review decision L-2 (r2rvm2 task 18.3): Pause/Fast-forward availability on the two pages at split widths 15..20. Subsidies coding is delivered by r2mlm11; the remaining layout decision awaits the user.
- Legacy TTO custom-name conversion: `!kit/TT/trt16.sv1` and `trt17.sv1` fail during `AfterLoadGame` with `Incorrect conversion of custom name string`; `trt16.sv1` reproduces in the unchanged pre-epic executable (R2RVM2 Group 18B). Repair of the pre-existing name conversion is deferred; no TTO compatibility pass is inferred. The commonly cited `trt00.sv0` is a TTD scenario and passes its load/month/round-trip checks.
- Operator acceptance skipped under the goal allowance: the full multiplayer/join and traffic/save matrices, large-map/framerate/memory measurements, complete construction/refusal and ownership scenarios, every-direction/all-scale/blitter art gallery and human art approval. Each owning group records its exact skip and available partial evidence. These remain unverified; completion of code and plans under the allowance is not release acceptance.

## Engineering Conventions

- **Comment style (user rule, overrides `CODINGSTYLE.md` comment syntax).** Inline comments are written exactly as `//comment` with no space after `//`. Every function and every parameter gets a Javadoc block (`/** ... @param ... @return ... */`). `if` and loop blocks get a short business-level comment; obvious statements get none. When refactoring, port existing comments and update them; never drop them.
- **Naming.** Capitalise type names and product terms in prose (Layer, Tile, Map, MultiLayerMap); refer to variables in their exact casing. Code follows OpenTTD naming: `this->member`, `_global`, CamelCase functions, `snake_case` variables, CamelCase scoped enumerators, file suffixes `_type.h`, `_map.h`, `_cmd.h` / `_cmd.cpp`, `_gui.cpp`.
- **Files.** New sources are registered in `src/CMakeLists.txt` (`add_files`), new tests in `src/tests/CMakeLists.txt`. No created source, test or JSON file name starts with `r2`. No Markdown in source directories. Helper scripts go in `docs/zz-reach2/upgrades/2026-10/tools/`.
- **Repository safety.** Never run `git checkout`, `git reset` or `git stash`, and never move or delete files; copying is allowed. Stand-ins are retired by editing code, not by deleting files.
- **Strings.** Only `src/lang/english.txt` is edited; translations fall back to English.

## Build and Test Commands

Every plan reuses these. They were verified on 2026-10-03.

- Toolchain: Visual Studio Build Tools 2022 (17.14, MSVC 14.44, Windows SDK 10.0.26100.0, ATL component `Microsoft.VisualStudio.Component.VC.ATL` for the vcpkg `breakpad` port), Git 2.56 (vcpkg needs a Git newer than 2.33). Visual Studio Community is not used: its install on this machine is incomplete and has no vcpkg, so `CMAKE_GENERATOR_INSTANCE` pins the Build Tools instance.
- CMake 3.31.6-msvc6: `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe` (not on `PATH`; call it by full path or prepend its folder to `PATH` in the session).
- vcpkg 2026-09-26, a standalone clone (as in `COMPILING.md`) at `D:\Codez\Tools\vcpkg` (upstream `dd6ff8e6b0`, 2026-10-03, bootstrapped with `bootstrap-vcpkg.bat -disableMetrics`). The configure builds the dependencies from `vcpkg.json`. Do not use the vcpkg bundled with Build Tools (`BuildTools\VC\vcpkg`, 2025-04-01): its helper scripts download an MSYS2 runtime that no longer exists (see `### Known configure failures`). When changing the toolchain file of an existing `build/`, add `--fresh`.
- Configure: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_GENERATOR_INSTANCE="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools" -DCMAKE_TOOLCHAIN_FILE="D:/Codez/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static`
- Build: `cmake --build build --config RelWithDebInfo --target openttd openttd_test` (use `--config Debug` when asserts are needed).
- Unit tests: `ctest --test-dir build -C RelWithDebInfo --output-on-failure`
- Run: from `build/`, `.\RelWithDebInfo\openttd.exe`. Base graphics come from the TTD files in `build/baseset/` (`## Baseline Data`).
- Surface digest: `python mlm/tools/savegames/savegame_plane_digest.py <save>`. `--tiles N` hashes only the first N tiles of each plane, so the surface plane of a layered save can be compared with a one-layer save.

### Known configure failures

Each was met and fixed on 2026-10-03. Never edit `vcpkg.json` to work around a failure; `COMPILING.md` lists the documented dependencies.

- **Every port "does not exist"** (`breakpad does not exist`, `liblzma does not exist`, ...). Git 2.33.1 rejects `git checkout-index --ignore-skip-worktree-bits`, so vcpkg extracts an empty versions tree. Fix: a newer Git for Windows (winget `Git.Git`).
- **The `atl` port fails** because `atlbase.h` is not in `INCLUDE`: the MSVC 14.44 toolset of Build Tools has no ATL. Fix: the Build Tools component `Microsoft.VisualStudio.Component.VC.ATL` ("C++ ATL for latest v143 build tools").
- **liblzma fails with `msys2-runtime-3.5.4-2-x86_64.pkg.tar.zst: failed: status code 404`** on every mirror. The bundled vcpkg (2025-04-01) pins an MSYS2 runtime that MSYS2 has removed, and `vcpkg_fixup_pkgconfig` needs it. Fix: the standalone vcpkg above, with `--fresh`.

With all three fixed, the configure took 9 minutes and built 13 vcpkg packages: the 7 manifest ports plus `atl`, `libdisasm`, `libogg`, `opus`, `vcpkg-cmake` and `vcpkg-cmake-config`. GRFCodec was not found, as intended: `openttd.grf` then comes from the copy in git.

## Baseline Data

The working tree already held the epic when the baseline was due, so the pre-epic reference was built from a `git archive 886ef09a09` export in `D:\Codez\Sandbox\OpenTTD-baseline` (r2rvm Groups 1 and 2).

- **Warnings and tests.** 0 distinct compiler warnings in Debug and in RelWithDebInfo, both pre-epic and epic. Pre-epic unit tests: 94 of 94; epic: 103 of 103 at the time (r2rvm tasks 1.4, 1.5, 4.3 and 4.4).
- **Base set.** `build/baseset/` holds `trg1.grf`, `trgc.grf`, `trgh.grf`, `trgi.grf`, `sample.cat` and `gm.cat` copied from `!kit/TTD/`, and `trgt.grf` copied from `!kit/TT/`. They match the MD5s in `media/baseset/orig_dos.obg`, `orig_dos.obs` and `orig_dos.obm`. `!kit/TTD/trgt.grf` does not (MD5 `fcde1d7e…`, expected `e30e8a39…`), and the game would report it as corrupt.
- **Missing translations.** On an en-US system the game picks American English. A development build then shows "This translation misses N strings": the strings the epic added to `english.txt` only, as `## Engineering Conventions` intend.
- **Baseline saves.** The retained copies now live in [OpenTTD-baseline/build/save/](../../../../OpenTTD-baseline/build/save/) inside this repository: `baseline_<seed>_<x>x<y>.sav` for seeds 1234 and 98765 at map sizes 8x8 (256 x 256) and 9x10 (512 x 1024), savegame version 368, plus `baseline_reload_` copies loaded and saved again. Their original producer was the external export above. They are made with `build/baseline.cfg` (`map_x`, `map_y` under `[game_creation]`) and `scripts/game_start.scr`, not `on_dedicated.scr`, which runs before the world exists. r2rvm `### Group 2 - Baseline saves and surface digests` has the exact commands; a plan that regenerates "the four baseline saves" reuses them with its own build folder. The local [retention manifest](../../../../OpenTTD-baseline/retained-files.json) records exact source identities and retained build/test logs.
- **Digests.** `docs/zz-rhr/upgrades/2026-10/r2fc-f-retained-records.md#baseline-plane-digests` holds 80 digest lines: the four saves and their four reload copies, whose planes equal the originals. It is the reference for I-6.

## Planning Provenance

This table records what shaped the plan. It is not a reading queue, it is non-transitive, and its line ranges record planning use rather than runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/README.md](../../architecture/README.md) | 1-30 | Map of subsystem notes used to pick the docs each sub-plan cites. |
| [docs/zz-reach2/architecture/initial-inventory.md](../../architecture/initial-inventory.md) | 185-203 | `### 1. Command as the only world mutation`, `### 2. Pool-indexed game objects`, `### 3. Tile map as a packed array`. Commands are the determinism boundary, pools store typed IDs and raw `TileIndex` values, and the world is one packed `Tile` array. This is why AD-2 addresses layers inside `TileIndex`: pools, orders and command arguments need no format change, and no command gains a layer argument. |
| [docs/zz-reach2/architecture/landscape/archi-landscape.md](../../architecture/landscape/archi-landscape.md) | 24-60, 116-207, 272-276 | Tile storage, indexing, void fence and dispatch facts that led to stacked-plane addressing (AD-2) instead of an active-layer swap. The `TileXY` formula, power-of-two axes and the void-border rule of `IsInnerTile` / `SetTileType` (116-159) are what AD-2 stacks and what keeps one-tile steps inside a layer (I-2). |
| [src/map_func.h](../../../../src/map_func.h) | 198-367 | `struct Map`, the existing map controller (`LogX`, `SizeX`, `SizeY`, `Size`, `MaxY`, `WrapToMap`, `ScaleByLandProportion`, `Iterate`), which AD-3 extends and places `MultiLayerMap` above. |
| [docs/zz-reach2/architecture/desync/archi-desync.md](../../architecture/desync/archi-desync.md) | 368-376 | `## Invariants`: same command stream, same RNG draws, no GUI-only state in the simulation. They constrain AD-4 (layer initialisation draws no `Random()`) and AD-7 (`Viewport::layer` is view state only). |
| [docs/zz-reach2/architecture/gui/archi-viewport.md](../../architecture/gui/archi-viewport.md) | 15-58, 98-128 | Projection and draw pipeline; showed that a viewport band plus filtering is enough to switch maps (AD-7). |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | 90-117, 167-177, 351-389 | RIFF map planes and versioning rules behind AD-9. |
| [docs/zz-reach2/architecture/pathfinder/archi-pathfinder.md](../../architecture/pathfinder/archi-pathfinder.md) | 178-246 | The closed-node consistency check that forces plane-based estimates (AD-5). |
| [docs/zz-reach2/architecture/vehicles/archi-vehicles.md](../../architecture/vehicles/archi-vehicles.md) | 35-77, 198-228 | Vehicle positions, hashes and tick order; confirmed stacked world coordinates need no vehicle layer field. |
| [docs/zz-reach2/architecture/landscape/tunnelbridge-tiles.md](../../architecture/landscape/tunnelbridge-tiles.md) | 5-41 | Tunnel bit layout used as the template for the portal tile (AD-8). |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 1342-1373, 1522-1842, 1971-2261 | Toolbar arrays and arrangements that decided the button placement and the r2mlm1 decomposition. |
| [src/pathfinder/follow_track.hpp](../../../../src/pathfinder/follow_track.hpp) | 209-245 | `FollowTileExit` tunnel branch; showed one neighbour function can carry cross-layer movement. |

## Completion Conditions

- Every row of both tables in `## Plan Index` is `[X]`.
- r2mlm10 task 6.3 records evidence for each Cross-plan Invariant, and the surface digests of the final build match `baseline-digests.txt` (I-6).
- The layer button selects Surface, Underground, Deep Underground and Outer Space; `N` cycles the main view in that display order. Movement, save and multiplayer code implements the canonical contracts; actual verified runs and authorised operator skips are recorded by their owning plans. Full operator acceptance is not inferred from closure under the allowance.
