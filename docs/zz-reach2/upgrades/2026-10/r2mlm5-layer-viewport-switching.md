# R2MLM5 - Viewport Bands and Real Map Switching

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
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 65-74 | `### AD-2 Stacked-plane addressing`: world coordinates are stacked, so a layer is a band of the world Y axis and a viewport shows a layer by looking at that band. Every drawing and picking change in this plan follows from it. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 94-96 | `### AD-7 Viewing a layer is view state`: `Viewport::layer` is client state; nothing in this plan may be read by commands or ticks (I-4). |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 141-170 | `## Engineering Conventions` and `## Build and Test Commands`. |
| [docs/zz-reach2/architecture/gui/archi-viewport.md](../../architecture/gui/archi-viewport.md) | 15-58 | `## Tile to screen` and `## What one draw collects`: `RemapCoords`, the row/column scan in `ViewportAddLandscape`, and the order landscape, vehicles, signs, text effects, sort, draw. The band test, the filters and the layer passed through `_vd` all hang off this pipeline. |
| [docs/zz-reach2/architecture/gui/archi-viewport.md](../../architecture/gui/archi-viewport.md) | 147-153 | `## Invariants`: a viewport reads the map and emits sprites, and never mutates tiles, vehicles or companies. Holds for every task here. |
| [src/map_layer_gui.cpp](../../../../src/map_layer_gui.cpp) | Live definitions; no fixed range | Produced by r2mlm1 Group 2 and bound to the controller by r2mlm2 Group 5. `SetViewportLayer` and `CycleMainViewportLayer` are the only entry points for switching; Group 1 replaces the interim body of `SetViewportLayer`. |

## Outcome and Boundaries

By the end of this plan pressing the layer button really switches the map:

- `SetViewportLayer` moves the view to the same plane position in the target layer's band.
- `ViewportAddLandscape` draws only the viewport's band; everything outside is the black off-map ground.
- Vehicles, signs, station and town labels, text effects and link-graph lines from other layers are not drawn and cannot be clicked.
- Picking, scroll clamping and drag highlights stay inside the band, so build tools place things in the viewed layer.
- Underground draws as bedrock and Space as vacuum (stand-ins until r2mlm6).
- The smallmap has its own layer selector; extra viewports, land info and world screenshots follow the layer.

Helpers exported for later plans: `MapLayer GetLayerOfWorldY(int y)` and `void ScrollMainWindowToTile` switching layers before scrolling.

All C7 rows of the r2mlm3 ledger are migrated by this plan.

## Implementation Tasks

### Area Mandatory Reading - Switching core

_Applies to Group 1._

Entry gate. Live games must allocate three layers (`InitializeGame` passes `MAX_MAP_LAYERS`, r2mlm4 task 6.1), and `src/map_layer.h` must export `MapLayerGround` and `MultiLayerMap::GetRules` (r2mlm4 Group 1). Verify by grepping `src/misc.cpp` and `src/map_layer.h`. If either is missing, stop Groups 2, 5 and 8; Groups 1, 3, 4 and 6 may be coded and unit-built but not accepted. Do not fake layers by drawing the surface band twice, and do not read `Viewport::layer` from any simulation code to work around missing data.

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 201-274 | `MapXYZToViewport` and `InitializeWindowViewport`: how a viewport is centred on a tile or vehicle. The focus tile or vehicle now also decides the initial layer. |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 1986-2046 | `UpdateViewportPosition`: the follow-vehicle branch and the clamp-and-ease branch. Following a vehicle must also follow its layer. |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 2540-2598 | `ScrollWindowTo`, `ScrollWindowToTile`, `ScrollMainWindowToTile`: the jump-to-location paths used by news, lists and vehicle windows; they now switch layer from the target world Y. |
| [src/window_gui.h](../../../../src/window_gui.h) | 242-257 | `struct ViewportData`: `scrollpos_*`, `dest_scrollpos_*`, `follow_vehicle`, all translated by `SetViewportLayer`. |
| [src/landscape.h](../../../../src/landscape.h) | 81-87 | `RemapCoords`: a world Y delta of `d` moves the view by `(2 * d * ZOOM_BASE, d * ZOOM_BASE)`; the band translation uses exactly this. |

### Group 1 - Switching core

{{MEDIUM}}
- [x] 1.1 Add `MapLayer GetLayerOfWorldY(int y)` to `src/map_layer_gui.cpp` (clamped to the allocated layers) with Javadoc.
- [x] 1.2 Replace the interim body of `SetViewportLayer`: translate `scrollpos_*` and `dest_scrollpos_*` by `RemapCoords(0, (new - old) * Map::SizeY() * TILE_SIZE, 0)`, cancel `follow_vehicle` when the vehicle is in another layer, set `layer`, mark the window dirty, invalidate the toolbar.
- [x] 1.3 In `InitializeWindowViewport`, take the layer from `GetTileLayer(tile)` for a tile focus and from the vehicle's tile for a vehicle focus; keep the r2mlm1 copy of the main layer for `INVALID_TILE`.
- [x] 1.4 In the follow-vehicle branch of `UpdateViewportPosition`, update `vp.layer` from the followed vehicle's tile and invalidate the toolbar when the window is the main window.
- [x] 1.5 In `ScrollWindowTo`, set the viewport layer from `GetLayerOfWorldY(y)` before computing the destination, without the band translation.
- [x] 1.6 Build and check that cycling the layer keeps the same plane position on screen (the view looks at the same x, y in another band). R2MLM1 Group 7B verifies all three band translations with identical saved plane camera coordinates and actual Surface reload captures; [camera-save-review.png](assets/reviews/camera-save-review.png).

### Area Mandatory Reading - Band drawing

_Applies to Group 2._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 171-188 | `struct ViewportDrawer`: the per-draw state `_vd`; gains a `MapLayer layer` member set by `ViewportDoDraw`. |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 1195-1320 | `GetViewportY` and `ViewportAddLandscape`: the inside-map test `IsInsideBS(tilecoord.y, 0, Map::SizeY())` becomes a band test, and off-band tiles take the outside-map path. |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 1814-1879 | `ViewportDoDraw`: sets up `_vd` and calls the add functions; the layer is copied from `vp.layer` here. |
| [src/tile_map.h](../../../../src/tile_map.h) | 35-45 | `TileHeightOutsideMap`: clamps Y to `Map::MaxY()`, which would read surface heights for a lower band. |
| [src/tile_map.h](../../../../src/tile_map.h) | 77-87 | `TilePixelHeightOutsideMap`: the pixel wrapper used by `GetViewportY`. |
| [src/tile_map.cpp](../../../../src/tile_map.cpp) | 70-86 | `GetTilePixelSlopeOutsideMap`: slope of fictive off-map tiles, called from `ViewportAddLandscape`. |
| [src/landscape.cpp](../../../../src/landscape.cpp) | 330-337 | `GetSlopePixelZOutsideMap`: Z of points outside the map, used by vehicles and picking at the edges. |
| [src/void_cmd.cpp](../../../../src/void_cmd.cpp) | 21-30 | `DrawTile_Void`: infinite water when `freeform_edges` is off; off the surface the fence must always be black. |

### Group 2 - Band drawing

Closure, 2026-10-04: task 2.7's every-edge visual operator sweep is skipped under the goal's allowance. Current per-layer views and floor captures exist, but they do not prove all map-edge cases.

{{HARD}}
- [x] 2.1 Add `MapLayer layer` to `ViewportDrawer` and set it from `vp.layer` at the top of `ViewportDoDraw`.
- [x] 2.2 Give `TileHeightOutsideMap` and `TilePixelHeightOutsideMap` a `MapLayer layer` parameter (default `MapLayer::Surface`) that clamps Y into that layer's band; update their Javadoc.
- [x] 2.3 Give `GetTilePixelSlopeOutsideMap` and `GetSlopePixelZOutsideMap` the same parameter and pass it through.
- [x] 2.4 In `GetViewportY` and `ViewportAddLandscape`, treat a tile as inside the map only when its stacked Y lies in `_vd.layer`'s band; pass `_vd.layer` to every outside-map helper; port the existing comments.
- [x] 2.5 Keep the bridge visibility logic working for surface viewports and skip it for other layers (no bridges off the surface, AD-12), with a comment.
- [x] 2.6 Make `DrawTile_Void` draw the black bare-land fence for every non-surface layer regardless of `freeform_edges`.
- [s] 2.7 Build, start a game, switch to Underground and Space, scroll to every map edge and confirm only the layer's own band is drawn with a black surround.

### Area Mandatory Reading - Clamping and picking

_Applies to Group 3._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/landscape.cpp](../../../../src/landscape.cpp) | 110-172 | `InverseRemapCoords2`: screen to world with optional clamp to the map; the clamp bounds become the band of a given layer. |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 419-460 | `TranslateXYToTileCoord`, `GetTileFromScreenXY`, `GetTileBelowCursor`: picking passes the viewport's layer to `InverseRemapCoords2`. |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 1921-1949 | `ClampViewportToMap`: keeps the viewport centre on the map; must clamp to its own band. |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 2148-2256 | `SetSelectionTilesDirty`: clamps the dirty area to `Map::SizeY() * TILE_SIZE` and tests `y < Map::MaxY()`; both become band bounds. |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 3094-3306 | `CalcRaildirsDrawstyle`: `CheckOverflow` and asserts against `Map::MaxY()`; band bounds. |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 3597-3607 | `GetViewportStationMiddle`: clamps to the map before `GetSlopePixelZ`; band bounds. |
| [src/terraform_gui.cpp](../../../../src/terraform_gui.cpp) | 216-223 | The southern-border fix-up block at the top of `GUIPlaceProcDragXY` (`TileY(end_tile) == Map::MaxY()`, repeated at line 388): plane Y. |

### Group 3 - Clamping and picking

{{MEDIUM}}
- [x] 3.1 Add a `MapLayer layer` parameter (default `MapLayer::Surface`) to `InverseRemapCoords2` and clamp into that band; update the Javadoc and every caller that has a viewport at hand.
- [x] 3.2 Pass `vp.layer` from `TranslateXYToTileCoord` and `ClampViewportToMap`.
- [x] 3.3 Convert the bounds in `SetSelectionTilesDirty` to the band of the main viewport's layer (the selection lives in the viewport that owns `_thd`).
- [x] 3.4 Convert the `CheckOverflow` bounds and asserts in `CalcRaildirsDrawstyle` to band bounds.
- [x] 3.5 Convert `GetViewportStationMiddle` and the bounds test in `ScrollWindowTo` to band or stacked bounds as appropriate.
- [x] 3.6 Convert both edge fix-ups in `src/terraform_gui.cpp` to `TilePlaneY`.
- [x] 3.7 Mark every C7 row of the r2mlm3 ledger in `viewport.cpp` and `terraform_gui.cpp` as `migrated`.

### Area Mandatory Reading - Filtering other layers

_Applies to Group 4._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/vehicle.cpp](../../../../src/vehicle.cpp) | 1146-1235 | `ViewportAddVehicles`: walks the vehicle viewport hash for the dirty rectangle; vehicles of other layers near the band edge would appear without a layer filter. |
| [src/vehicle.cpp](../../../../src/vehicle.cpp) | 1244-1293 | `CheckClickOnVehicle`: click hit-test against vehicle screen rectangles; filtered by the viewport's layer. |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 1449-1529 | `ViewportAddKdtreeSigns`: emits town, station, waypoint and sign labels found by the sign k-d tree. |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 2291-2370 | `CheckClickOnViewportSign`: click hit-test against the same labels. |
| [src/viewport_kdtree.h](../../../../src/viewport_kdtree.h) | 19-47 | `struct ViewportSignKdtreeItem`: the item type and id from which the label's tile, and therefore layer, is derived. |
| [src/texteff.cpp](../../../../src/texteff.cpp) | 22-64 | `struct TextEffect` and `AddTextEffect`: floating text has only screen coordinates, so it needs a stored layer to be filtered. |
| [src/texteff.cpp](../../../../src/texteff.cpp) | 116-135 | `DrawTextEffects`: where the filter is applied. |

Context has compacted 1 times while executing this group.

### Group 4 - Filtering other layers

Closure, 2026-10-04: task 4.6's sign-display/hit-test operator check is skipped under the goal's allowance. No click-isolation pass is inferred from the filter implementation.

{{MEDIUM}}
- [x] 4.1 In `ViewportAddVehicles`, skip vehicles whose layer (from `v->tile`, or `TileVirtXY` of the position for effect and disaster vehicles) differs from `_vd.layer`.
- [x] 4.2 In `CheckClickOnVehicle`, skip vehicles of other layers than `vp.layer`.
- [x] 4.3 In `ViewportAddKdtreeSigns` and `CheckClickOnViewportSign`, derive each label's tile (station `xy`, town `xy`, sign position) and skip other layers.
- [x] 4.4 Store a `MapLayer` in `TextEffect`, set it in `AddTextEffect` from a new parameter defaulting to the main viewport layer, pass the tile's layer from the cost-animation callers, and filter in `DrawTextEffects`.
- [x] 4.5 Filter the link-graph overlay so links are drawn only when both stations are in the viewport's layer.
- [s] 4.6 Build and confirm in a game with a sign placed underground that it shows only in the Underground view and cannot be clicked from the surface.

### Area Mandatory Reading - Layer ground

_Applies to Group 5._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/clear_cmd.cpp](../../../../src/clear_cmd.cpp) | 141-187 | `DrawTile_Clear`: ground drawing per `ClearGround`. Underground and Space tiles are `ClearGround::Rocks`; the layer's `MapLayerGround` decides the sprite instead. |
| [src/map_layer.h](../../../../src/map_layer.h) | Live definitions; no fixed range | Produced by r2mlm4 Group 1: `MapLayerGround`, `MultiLayerMap::GetRules(TileIndex)`. Authoritative for which ground to draw. |
| [src/table/sprites.h](../../../../src/table/sprites.h) | Live definitions; no fixed range | After r2mlm6 Group 7, `SPR_MAP_LAYER_UNDERGROUND_FLOOR` and `SPR_MAP_LAYER_SPACE_FLOOR` exist. Until then the stand-ins are `SPR_FLAT_ROCKY_LAND_1` (bedrock) and `SPR_FLAT_BARE_LAND` with `PALETTE_ALL_BLACK` (vacuum). |

### Group 5 - Layer ground

Closure, 2026-10-04: visual operator tasks 5.4 and 5.5 are skipped under the goal's allowance. The multi-climate sweep is not performed. The old uniform-black stand-in is superseded by R2MLM6/R2MLM13's sparse-star Space floor (AD-10); current editor captures show that floor, without claiming the obsolete stand-in check passed.

{{MEDIUM}}
- [x] 5.1 In `DrawTile_Clear`, branch on `MultiLayerMap::GetRules(ti->tile).ground` before the `ClearGround` switch: Bedrock and Vacuum draw their flat sprite and return; Natural keeps today's code unchanged.
- [x] 5.2 Add `static SpriteID GetLayerGroundSprite(MapLayerGround ground)` in `src/clear_cmd.cpp` returning the stand-ins, with a comment naming the r2mlm6 constants that replace them.
- [x] 5.3 If `SPR_MAP_LAYER_UNDERGROUND_FLOOR` exists (grep `src/table/sprites.h`), use it with a `TileHash`-based variant pick; otherwise keep the stand-in and leave the task for r2mlm6 task 7.2.
- [s] 5.4 Confirm visually that underground rail and road tiles draw on bare ground (forced by r2mlm4 task 3.3) and that no snow or desert appears off the surface in arctic and tropic games.
- [s] 5.5 Confirm Space draws as uniform black vacuum, distinct from the off-map fence by a one-pixel grid line or not at all; record the choice. The code choice is the same black as the off-map fence (`SPR_FLAT_BARE_LAND` with `PALETTE_ALL_BLACK`) and no extra grid. Seeing it still waits on a build.

### Area Mandatory Reading - Smallmap per layer

_Applies to Group 6._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/smallmap_gui.cpp](../../../../src/smallmap_gui.cpp) | 850-956 | `SetNewScroll` (clamps with `Map::MaxY()`), `DrawMapIndicators` (the main viewport rectangle) and `DrawSmallMapColumn` (samples tiles with `TileXY` and skips `yc >= Map::MaxY()`). |
| [src/smallmap_gui.cpp](../../../../src/smallmap_gui.cpp) | 957-1019 | `DrawVehicles` and `DrawTowns`: vehicles are filtered by layer; towns only on the surface. |
| [src/smallmap_gui.cpp](../../../../src/smallmap_gui.cpp) | 1134-1217 | `RemapTile`, `PixelToTile`, `ComputeScroll`: smallmap pixel and tile conversion, kept in plane coordinates. |
| [src/smallmap_gui.cpp](../../../../src/smallmap_gui.cpp) | 1330-1410 | `GetTileColours`: per-tile colour lookup over a `TileArea`, which must be built in the selected layer. |
| [src/smallmap_gui.cpp](../../../../src/smallmap_gui.cpp) | 1488-1505 | `SmallMapCenterOnCurrentPos`: centres on the main viewport, which now carries a layer. |
| [src/smallmap_gui.cpp](../../../../src/smallmap_gui.cpp) | 1705-1827 | `SmallMapWindow::OnClick`: map clicks scroll the main viewport; the new layer button is handled here. |
| [src/widgets/smallmap_widget.h](../../../../src/widgets/smallmap_widget.h) | 14-37 | `SmallMapWidgets`: the new `WID_SM_MAP_LAYER` button id. |

### Group 6 - Smallmap per layer

{{MEDIUM}}
- [x] 6.1 Add `MapLayer layer` to `SmallMapWindow`, initialised from the main viewport's layer, and include the layer name in the caption.
- [x] 6.2 Add `WID_SM_MAP_LAYER` (push image button using `GetMapLayerIcon`) next to `WID_SM_CENTERMAP`, cycling `this->layer` with `GetNextMapLayer` and forcing a refresh.
- [x] 6.3 Make `DrawSmallMapColumn`, `GetTileColours` and `SetNewScroll` work in plane coordinates and build tiles with `TileXYL(x, y, this->layer)`.
- [x] 6.4 Filter `DrawVehicles` by layer; draw towns and industry names only for the surface.
- [x] 6.5 Draw the main viewport rectangle in `DrawMapIndicators` only when the main viewport shows `this->layer`.
- [x] 6.6 Make map clicks and `SmallMapCenterOnCurrentPos` switch the main viewport to `this->layer` (via `SetViewportLayer`) and scroll with the stacked world Y.
- [x] 6.7 Mark the `smallmap_gui.cpp` C7 ledger rows `migrated`.

### Area Mandatory Reading - Extra viewports, land info and screenshots

_Applies to Group 7._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/viewport_gui.cpp](../../../../src/viewport_gui.cpp) | 53-181 | `ExtraViewportWindow`, `ShowExtraViewportWindow`, `ShowExtraViewportWindowForTileUnderCursor`: extra views get their layer from the focus tile (Group 1). |
| [src/misc_gui.cpp](../../../../src/misc_gui.cpp) | 65-308 | `LandInfoWindow`: builds the land-area information lines; a "Layer" line is added. |
| [src/screenshot.cpp](../../../../src/screenshot.cpp) | 200-276 | `SetupScreenshotViewport`: the world screenshot spans the whole map using `Map::Size() - 1` as the south tile; it must span the main viewport's band. |
| [src/screenshot.cpp](../../../../src/screenshot.cpp) | 298-346 | `HeightmapCallback` and `MakeHeightmapScreenshot`: the heightmap screenshot walks `Map::Iterate()` and sizes the image by `Map::SizeX()` / `Map::SizeY()`; surface only. The minimap screenshot (`MinimapScreenCallback`, used at line 515) and the size check in `MakeScreenshotWithConfirm` (lines 359-383) follow the same rule. |

### Group 7 - Extra viewports, land info and screenshots

Closure, 2026-10-04: task 7.1's complete operator matrix of extra-view opening paths is skipped under the goal's allowance. The hotkey/caption smoke in R2RVM2 task 16.6 passes, but it does not cover all menu and vehicle-window paths.

{{SIMPLE}}
- [s] 7.1 Confirm extra viewports opened from the map menu, the hotkey and vehicle windows start in the right layer.
- [x] 7.2 Add `WID_EV_MAP_LAYER` to the extra viewport window, cycling its own layer with `SetViewportLayer`.
- [x] 7.3 Add a `STR_LAND_AREA_INFORMATION_MAP_LAYER :{BLACK}Layer: {LTBLUE}{STRING}` line to `LandInfoWindow`.
- [x] 7.4 Make the world screenshot cover the main viewport's band and set the screenshot viewport's `layer`.
- [x] 7.5 Make the heightmap and minimap screenshots iterate the surface plane only.
- [x] 7.6 Mark the `screenshot.cpp` C7 ledger rows `migrated`.

### Area Mandatory Reading - Button finish and verification

_Applies to Group 8._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [r2mlm4-layer-world-lifecycle-and-savegame.md](r2mlm4-layer-world-lifecycle-and-savegame.md) | 147-147 | Task 4.8 of r2mlm4 and the gating matrix recorded beneath it during execution (the list grows below this line): the commands, kinds and allowed layers verified in task 8.4. Read only that task's list. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 2006-2186 | `MainToolbarWindow`: the icon and tooltip refresh added by r2mlm1 Group 5, now reflecting real switching. |
| [docs/zz-reach2/upgrades/2026-10/tools/baseline-digests.txt](tools/baseline-digests.txt) | Live definitions; no fixed range | Produced by r2rvm task 2.5 (master plan `## Baseline Data`); rendering changes must not alter saved surface data. |

### Group 8 - Button finish and verification

Closure, 2026-10-04: operator tasks 8.3..8.5 are skipped under the goal's allowance for the full tooltip, construction/refusal and news/list matrices. Current four-layer switching and camera translation pass R2MLM1 Group 7, and the supplied-save station/signal diagnosis is in R2MLM13 Group 9C; those subsets do not pass the complete matrices here. Surface neutrality is verified below.

{{SIMPLE}}
- [x] 8.1 Use `SPR_IMG_MAP_LAYER_SURFACE`, `SPR_IMG_MAP_LAYER_UNDERGROUND` and `SPR_IMG_MAP_LAYER_SPACE` in `GetMapLayerIcon` if they exist in `src/table/sprites.h`; otherwise keep the stand-ins.
- [x] 8.2 Build both configurations without new warnings; run `ctest`.

  Verified 2026-10-04: both targets/configurations build, exit 0, zero compiler warnings; both unit runs pass 110/110. Evidence: `build/build-portal-rail-conversion-{Debug,RelWithDebInfo}.log`, `build/ctest-portal-rail-conversion-{Debug,RelWithDebInfo}.log`. Visual and savegame checks remain open for the representative save.
- [s] 8.3 Cycle the layer in a new game: each layer draws only itself, the plane position is kept, and the icon and tooltip follow.
- [s] 8.4 In the Underground view, build road, rail, a bus stop, a rail station and a depot; confirm they appear only underground. Try every forbidden command from the r2mlm4 task 4.8 matrix in Underground and Space and confirm the layer error.
- [s] 8.5 Open a news item or the station list for an underground station and confirm the main view jumps to the Underground layer.
- [x] 8.6 Compare the surface digests of the four baseline saves with `baseline-digests.txt`; they must still match. Every Surface chunk matches for both seeds/sizes; [tools/final-layer-save-digests.txt](tools/final-layer-save-digests.txt), R2RVM2 task 18.1.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| Sprites from the surface bleed into the Underground view near the band edge | [docs/zz-reach2/architecture/gui/archi-viewport.md](../../architecture/gui/archi-viewport.md) | `## Sprite sort` (lines 60-79) | The bleed is a parent sprite, not ground | Find which add function emitted it and add the missing layer filter. |
| The view jumps to the wrong place after `SetViewportLayer` | [docs/zz-reach2/architecture/gui/archi-viewport.md](../../architecture/gui/archi-viewport.md) | `## Zoom` (lines 81-96) | The error grows with zoom level | Check virtual versus screen coordinates in the translation. |
| Black lines or gaps appear when scrolling a band | [docs/zz-reach2/architecture/gui/archi-blitter.md](../../architecture/gui/archi-blitter.md) | `## The frame that feeds the blitter` (lines 87-148) | The gaps are not explained by dirty-block marking | Diagnose redraw of scrolled regions. |
| Smallmap colours are wrong for one map type only | [src/smallmap_gui.cpp](../../../../src/smallmap_gui.cpp) | The `GetSmallMap*Pixels` helper for that type (lines 439-630) | The helper reads a tile outside the selected layer | Find the helper that builds its own tiles. |
| Following a vehicle flickers between layers | [docs/zz-reach2/architecture/vehicles/archi-vehicles.md](../../architecture/vehicles/archi-vehicles.md) | `## Pool and chain` (lines 35-45) | The vehicle's moving front and its tile disagree | Decide which part's tile defines the followed layer. |

## Planning Provenance

This table records what shaped the plan. It is not an execution reading queue, it must not be preloaded after context compaction, it is non-transitive, and its line ranges record planning use rather than runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [docs/zz-reach2/architecture/gui/archi-viewport.md](../../architecture/gui/archi-viewport.md) | 7-58, 98-153 | The draw and click pipeline; showed one band test plus filters is enough. |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 171-188, 1195-1320, 1814-1879, 1923-2046, 2148-2256, 2498-2598, 3094-3306 | Every place that bounds or converts viewport coordinates; drove Groups 1 to 4. |
| Repository grep of `Map::MaxY`, `Map::SizeY`, `Map::Size`, `Map::Iterate` in `viewport.cpp`, `smallmap_gui.cpp`, `screenshot.cpp`, `terraform_gui.cpp` | Ten, three, five and two hits | The C7 work list taken over from r2mlm3. |
| [src/vehicle.cpp](../../../../src/vehicle.cpp) | 600-690, 1117-1293 | Vehicle hashes keyed by stacked coordinates; confirmed filtering rather than re-hashing. |
| [src/void_cmd.cpp](../../../../src/void_cmd.cpp), [src/clear_cmd.cpp](../../../../src/clear_cmd.cpp) | 21-30, 141-187 | Fence and ground drawing behind Groups 2 and 5. |

## Completion Conditions

Closed 2026-10-04 under the goal's operator allowance, with explicit skips at Groups 2, 4, 5, 7 and 8. Code, current band/camera checks and Surface digest checks are delivered. Final builds have zero compiler warnings, both unit suites pass 111/111 and full release CTest passes 115/115. The original full visual/runtime criteria below remain future validation criteria.

- The layer button switches the main view between three visibly different maps at the same plane position.
- No sprite, label, text effect or click from another layer leaks into a view.
- Build tools place construction in the viewed layer; forbidden construction shows the layer error.
- Smallmap, extra viewports, land info and world screenshots respect the layer.
- All C7 ledger rows are migrated, tests pass, and surface digests are unchanged.
