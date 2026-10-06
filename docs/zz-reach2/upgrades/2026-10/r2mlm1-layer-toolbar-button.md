# R2MLM1 - Layer Vocabulary and the Toolbar Layer Button

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
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 61-63 | `### AD-1 Layer vocabulary`: the exact `MapLayer` enumerators, `MAX_MAP_LAYERS`, and the rule that names and icons live in `src/map_layer_gui.cpp`. This plan is the owner of that vocabulary; later plans consume it unchanged, so the values written here are final. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 94-96 | `### AD-7 Viewing a layer is view state`: `Viewport::layer` is client state and never read by commands or ticks. The button changes view state only and posts no command. Group 7B normalises existing saved camera coordinates without persisting a layer field. |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | 141-170 | `## Engineering Conventions` and `## Build and Test Commands`: the user's comment style (`//comment`, Javadoc on every function and parameter), file naming, the no-move/no-delete rule, and the exact build and test commands used for verification. |
| [src/widgets/toolbar_widget.h](../../../../src/widgets/toolbar_widget.h) | 13-49 | `enum ToolbarNormalWidgets`: its order is the index of `_toolbar_button_procs`, `_menu_clicked_procs` and `_toolbar_button_sprites`, and the offset of each tooltip from `STR_TOOLBAR_TOOLTIP_PAUSE_GAME`. Inserting `WID_TN_MAP_LAYER` shifts every later index, so all parallel arrays and the tooltip block change in the same group. |
| [docs/zz-reach2/architecture/gui/archi-gui.md](../../architecture/gui/archi-gui.md) | 217-223 | `## Invariants`: `OnPaint` and `DrawWidget` only draw, the GUI changes the world only through commands, widget indices name nested-tree nodes. Decides that the button's icon is updated from `OnInvalidateData`, never from `OnPaint`. |

## Outcome and Boundaries

This plan delivers Key Point 1 of the master plan.

- `src/map_layer_type.h` with `enum class MapLayer` and `MAX_MAP_LAYERS` exactly as AD-1 states.
- `Viewport::layer` (default `MapLayer::Surface`) in `src/viewport_type.h`.
- `src/map_layer_gui.h` / `src/map_layer_gui.cpp` exporting `GetMapLayerName`, `GetMapLayerIcon`, `GetNextMapLayer`, `SetViewportLayer` and `CycleMainViewportLayer`.
- `WID_TN_MAP_LAYER` immediately after `WID_TN_SMALL_MAP` in the main toolbar, and `WID_TE_MAP_LAYER` immediately after `WID_TE_SMALL_MAP` in the scenario editor toolbar, with tooltips and a hotkey.

Interim behaviour, by design: the button cycles the main viewport's `layer` over all `MAX_MAP_LAYERS` values and updates its icon and tooltip, but the map drawn does not change yet. r2mlm2 limits the cycle to `MultiLayerMap::GetLayerCount()`; r2mlm5 replaces the body of `SetViewportLayer` so the view really switches. Stand-in icons until r2mlm6 delivers art: Surface `SPR_IMG_PLANTTREES`, Underground `SPR_IMG_TUNNEL_RAIL`, Space `SPR_IMG_AIRPLANESLIST`.

Out of scope: tile storage, drawing, savegame (the view layer is never saved), and any command.

## Implementation Tasks

### Area Mandatory Reading - Layer vocabulary and view state

_Applies to Groups 1-2._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/viewport_type.h](../../../../src/viewport_type.h) | 30-46 | `struct Viewport`: the plain data struct that receives the new `layer` member. `ViewportData` derives from it, so every viewport (main, extra, vehicle view) inherits the field. |
| [src/viewport.cpp](../../../../src/viewport.cpp) | 209-274 | `InitializeWindowViewport`: the only constructor path for viewports; it centres on a tile, a vehicle, or the main window's position. Task 2.2 initialises `layer` here, copying the main viewport's layer when the focus is `INVALID_TILE`. |
| [src/window_func.h](../../../../src/window_func.h) | 20-20 | `GetMainWindow()`: the accessor `CycleMainViewportLayer` uses to reach the main viewport. |
| [src/CMakeLists.txt](../../../../src/CMakeLists.txt) | 266-270 | The alphabetical `add_files` entries around `map.cpp`, `map_func.h`, `map_type.h`: where the new headers and sources are registered. |
| [src/lang/english.txt](../../../../src/lang/english.txt) | 477-481 | `# Map menu` strings: the neighbourhood where the three layer names are added. |

### Group 1 - Layer vocabulary

{{SIMPLE}}
- [x] 1.1 Create `src/map_layer_type.h` with the standard OpenTTD licence header, `@file` comment and include guard `MAP_LAYER_TYPE_H`, including `core/enum_type.hpp`.
- [x] 1.2 Define `enum class MapLayer : uint8_t { Surface = 0, Underground = 1, Space = 2, End, Invalid = 0xFF }` with a `///<` comment per enumerator, and a Javadoc block on the enum that states the values are the stacking order and must never be reordered.
- [x] 1.3 Define `static constexpr uint MAX_MAP_LAYERS = to_underlying(MapLayer::End);` with a `static_assert(MAX_MAP_LAYERS == 3)` and a comment that AD-1 is the authority.
- [x] 1.4 Register `map_layer_type.h` in `src/CMakeLists.txt` in alphabetical position after `map_func.h`.

### Group 2 - View state and layer GUI helpers

{{MEDIUM}}
- [x] 2.1 Add `MapLayer layer = MapLayer::Surface; ///< Map layer shown by this viewport. View state only, never read by the simulation.` to `struct Viewport` and include `map_layer_type.h` in `viewport_type.h`.
- [x] 2.2 In `InitializeWindowViewport`, set `vp->layer` explicitly: copy `GetMainWindow()->viewport->layer` in the `INVALID_TILE` branch and use `MapLayer::Surface` otherwise; add the business comment explaining the copy.
- [x] 2.3 Add strings `STR_MAP_LAYER_SURFACE`, `STR_MAP_LAYER_UNDERGROUND`, `STR_MAP_LAYER_SPACE` under a new `# Map layers` comment after the map menu strings in `src/lang/english.txt`.
- [x] 2.4 Create `src/map_layer_gui.h` declaring `StringID GetMapLayerName(MapLayer layer)`, `SpriteID GetMapLayerIcon(MapLayer layer)`, `MapLayer GetNextMapLayer(MapLayer layer, uint layer_count)`, `void SetViewportLayer(Window *w, MapLayer layer)` and `void CycleMainViewportLayer()`, each with a Javadoc block and `@param` lines.
- [x] 2.5 Create `src/map_layer_gui.cpp` implementing the name and icon lookups with a `switch` over `MapLayer` (stand-in icons from `## Outcome and Boundaries`), and `GetNextMapLayer` wrapping to `MapLayer::Surface` after `layer_count - 1`.
- [x] 2.6 Implement the interim `SetViewportLayer`: assign `w->viewport->layer`, mark the window dirty, and call `InvalidateWindowData(WindowClass::MainToolbar, 0)`; add a comment that r2mlm5 replaces this body with scroll translation.
- [x] 2.7 Implement `CycleMainViewportLayer` as `SetViewportLayer(GetMainWindow(), GetNextMapLayer(current, MAX_MAP_LAYERS))`, with a comment that r2mlm2 swaps `MAX_MAP_LAYERS` for `MultiLayerMap::GetLayerCount()`.
- [x] 2.8 Register `map_layer_gui.h` and `map_layer_gui.cpp` in `src/CMakeLists.txt`, then build both configurations. Verified 2026-10-03: both targets build in Debug and RelWithDebInfo (`build/verify-Debug.log`, `build/verify-RelWithDebInfo.log`).

### Area Mandatory Reading - Main toolbar wiring

_Applies to Groups 3-5._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 1340-1373 | `_menu_clicked_procs`: dropdown handlers indexed by widget with `// N` comments. The new button has no dropdown, so it gets a `nullptr` slot at index 5 and every later comment is renumbered. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 1522-1842 | `NWidgetMainToolbarContainer::GetButtonArrangement`: `arrange14` to `arrange20` (two rows of N buttons each) and `arrange_all`. The comment at the top requires each array to hold exactly `2 * N` entries. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 866-884 | `ToolbarZoomInClick` / `ToolbarZoomOutClick`: the push-button pattern (`HandleButtonClick` with the editor or normal widget id) the layer button copies. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 1964-2003 | `ToolbarButtonProc` and `_toolbar_button_procs`: click handlers indexed by widget. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 2006-2186 | `struct MainToolbarWindow`: `OnClick`, `OnHotkey`, `OnInvalidateData` (zoom button state) and `hotkeys`. The layer button's icon refresh and hotkey go here. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 2188-2261 | `_toolbar_button_sprites`, `GetToolbarMaximalImageSize`, `MakeMainToolbar`: widget type and initial sprite per button, and the spacer placed before `WID_TN_SMALL_MAP` (nothing may be inserted between the map and layer buttons). |
| [src/toolbar_gui.h](../../../../src/toolbar_gui.h) | 16-57 | `enum MainToolbarHotkeys`: the new `MTHK_CYCLE_MAP_LAYER` is appended at the end. |
| [src/lang/english.txt](../../../../src/lang/english.txt) | 384-416 | `# Tooltips for the main toolbar` with `###length 31`: the offset-indexed tooltip block. The new tooltip goes right after `STR_TOOLBAR_TOOLTIP_DISPLAY_MAP` and the length becomes 32. |
| [src/rail_gui.cpp](../../../../src/rail_gui.cpp) | 505-514 | `BuildRailToolbarWindow::OnTooltip`: the pattern for a dynamic tooltip with `GuiShowTooltips` and an encoded string. |

### Group 3 - Main toolbar widget and parallel arrays

{{MEDIUM}}
- [x] 3.1 Insert `WID_TN_MAP_LAYER, ///< Cycle the map layer shown by the main viewport.` directly after `WID_TN_SMALL_MAP` in `ToolbarNormalWidgets`.
- [x] 3.2 Insert `STR_TOOLBAR_TOOLTIP_CYCLE_MAP_LAYER :{BLACK}Switch the main view between the surface, underground and space maps` right after `STR_TOOLBAR_TOOLTIP_DISPLAY_MAP` and change `###length 31` to `###length 32`.
- [x] 3.3 Insert `ToolbarMapLayerClick` (declared in Group 5) after `ToolbarMapClick` in `_toolbar_button_procs`, keeping a forward declaration above the array.
- [x] 3.4 Insert `nullptr, // 5` after `MenuClickMap, // 4` in `_menu_clicked_procs` and renumber every following `// N` comment.
- [x] 3.5 Insert `{WID_TN_MAP_LAYER, WWT_PUSHIMGBTN, SPR_IMG_PLANTTREES}` right after the `WID_TN_SMALL_MAP` row of `_toolbar_button_sprites`.
- [x] 3.6 Add `static_assert(std::size(_toolbar_button_procs) == WID_TN_END)` and `static_assert(std::size(_menu_clicked_procs) == WID_TN_SWITCH_BAR)` with a comment explaining the parallel-array contract, then build. Both assertions are present; both configurations build on 2026-10-03.

### Group 4 - Toolbar arrangements

{{MEDIUM}}
- [x] 4.1 Insert `WID_TN_MAP_LAYER` directly after `WID_TN_SMALL_MAP` in `arrange_all`.
- [x] 4.2 In each of `arrange14` to `arrange20`, insert `WID_TN_MAP_LAYER` directly after every `WID_TN_SMALL_MAP` entry.
- [x] 4.3 Restore exactly N entries per row in each split array by removing one entry from the affected row, choosing in this order: a button that appears in both rows (never `WID_TN_SWITCH_BAR`), then `WID_TN_SUBSIDIES`, then `WID_TN_LEAGUE`, `WID_TN_STORY`, `WID_TN_GOAL`; record each removal as a comment on the row.
  - Subsidies, dropped from `arrange14` and `arrange15`, is offered by the Towns menu since r2mlm11. Lower-page menu checks passed on 2026-10-04; the upper pages have no Towns button. R2MLM11 Group 2 records that test-plan mismatch and the working F6 fallback.
- [x] 4.4 Add a `static_assert` per split array that its length is `2 * N`, so a future insertion cannot unbalance the rows.
- [x] 4.5 Build, start the game, and resize the window through every split width (14 to 20 buttons) plus full width; confirm the layer button always sits immediately right of the map button and no row overflows.

  Verified 2026-10-04 in Debug with Interface size 250%: client widths 728, 780, 832, 884, 936, 988 and 1040 pixels select arrangements 14..20; 1750 pixels shows the full row. Both pages were captured at every split width. Wherever the Small map button is present, the Layer button follows it directly. All buttons fit the main-toolbar rows. Evidence: [toolbar-width-review.png](assets/reviews/toolbar-width-review.png), original captures `build/toolbar-review/{14..20}-{upper,lower}.png` and `full-visible.png`, log `build/toolbar-review/game.log`.

### Group 5 - Button behaviour, icon and hotkey

Closure, 2026-10-04: task 5.5's operator check of the intermediate three-layer, icon-only build is skipped under the goal's operator allowance. R2MLM5 replaces that behaviour with real map switching and R2MLM12 replaces button cycling with a four-layer dropdown; the final build cannot exhibit the old behaviour. Current controls are checked in Group 7. No historical runtime pass is inferred.

{{MEDIUM}}
- [x] 5.1 Implement `static CallBackFunction ToolbarMapLayerClick(Window *w)` with a Javadoc block: call `CycleMainViewportLayer()`, then `w->HandleButtonClick` with `WID_TE_MAP_LAYER` in the editor and `WID_TN_MAP_LAYER` otherwise, play `SndClickBeep()`, return `CallBackFunction::None`.
- [x] 5.2 In `MainToolbarWindow::OnInvalidateData`, set the button sprite with `this->GetWidget<NWidgetCore>(WID_TN_MAP_LAYER)->SetSprite(GetMapLayerIcon(GetMainWindow()->viewport->layer))` and mark the widget dirty; set it once in the constructor as well.
- [x] 5.3 Override `MainToolbarWindow::OnTooltip` for `WID_TN_MAP_LAYER` only, showing a new `STR_TOOLBAR_TOOLTIP_MAP_LAYER_CURRENT :{BLACK}Switch the main view between the surface, underground and space maps. Now showing: {STRING}` with the current layer name; return `false` for every other widget.
- [x] 5.4 Append `MTHK_CYCLE_MAP_LAYER` to `MainToolbarHotkeys`, handle it in `OnHotkey` by calling `ToolbarMapLayerClick(this)`, and add `Hotkey(0, "cycle_map_layer", MTHK_CYCLE_MAP_LAYER)`; if a free single key exists after grepping every `HotkeyList` in `src/`, bind it and record the choice here. Bound to N. L is terraform, M is the small map, and V opens an extra view. The editor hotkey stays unbound, as task 6.5 says.
- [s] 5.5 Build, start a new game, click the button three times and use the hotkey; confirm the icon and tooltip go Surface, Underground, Space, Surface and that nothing else on screen changes (interim behaviour).

### Area Mandatory Reading - Scenario editor toolbar

_Applies to Group 6._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/widgets/toolbar_widget.h](../../../../src/widgets/toolbar_widget.h) | 51-76 | `enum ToolbarEditorWidgets`: `WID_TE_MAP_LAYER` is inserted after `WID_TE_SMALL_MAP`, shifting the editor arrays. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 1845-1960 | `NWidgetScenarioToolbarContainer::GetButtonArrangement`: `arrange_all`, `arrange_nopanel`, `arrange_switch` (two halves). The panel-width arithmetic counts entries, so every array gains exactly one entry. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 2279-2360 | `_scen_toolbar_dropdown_procs`, `_scen_toolbar_button_procs`, `enum class MainToolbarEditorHotkeys`. The hotkey note requires button-like hotkeys to equal their widget value. |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 2540-2605 | Editor `hotkeys`, `_nested_toolb_scen_inner_widgets`, `MakeScenarioToolbar`: the editor builds its buttons from an explicit part list, with explicit tooltips. |
| [src/lang/english.txt](../../../../src/lang/english.txt) | 418-433 | `# Extra tooltips for the scenario editor toolbar`: where the editor tooltip string is added. |

### Group 6 - Scenario editor toolbar

{{SIMPLE}}
- [x] 6.1 Insert `WID_TE_MAP_LAYER, ///< Cycle the map layer shown by the main viewport.` after `WID_TE_SMALL_MAP` in `ToolbarEditorWidgets`.
- [x] 6.2 Insert `nullptr, // 9` after `MenuClickMap, // 8` in `_scen_toolbar_dropdown_procs`, renumber the comments, and insert `ToolbarMapLayerClick` after `ToolbarScenMapTownDir` in `_scen_toolbar_button_procs`.
- [x] 6.3 Add `NWidget(WWT_PUSHIMGBTN, Colours::Grey, WID_TE_MAP_LAYER), SetSpriteTip(SPR_IMG_PLANTTREES, STR_SCENEDIT_TOOLBAR_CYCLE_MAP_LAYER_TOOLTIP),` directly after the `WID_TE_SMALL_MAP` part, before the following spacer, and add the tooltip string.
- [x] 6.4 Insert `WID_TE_MAP_LAYER` after every `WID_TE_SMALL_MAP` entry in `arrange_all`, `arrange_nopanel` and both halves of `arrange_switch`.
- [x] 6.5 Add `CycleMapLayer = WID_TE_MAP_LAYER` to `MainToolbarEditorHotkeys`, handle it in the editor window's hotkey dispatch, and add `Hotkey(0, "cycle_map_layer", MainToolbarEditorHotkeys::CycleMapLayer)`.
- [x] 6.6 Refresh the editor button sprite from `ScenarioEditorToolbarWindow::OnInvalidateData`, mirroring task 5.2.

### Area Mandatory Reading - Verification

_Applies to Group 7._

| File name | Line Ranges | Description |
| --- | --- | --- |
| [src/tests/test_window_desc.cpp](../../../../src/tests/test_window_desc.cpp) | 79-99 | `WindowDesc - NWidgetParts properly closed` and `WindowDesc - NWidgetPart validity`: the unit tests that catch a malformed editor part list after task 6.3. |
| [src/map_layer_gui.cpp](../../../../src/map_layer_gui.cpp) | Live definitions; no fixed range | Produced by Group 2. `CycleMainViewportLayer` and `SetViewportLayer` are the authoritative behaviour being verified. R2MLM5 and R2MLM12 supersede the interim body and three-layer controls. |

### Group 7 - Verification

{{SIMPLE}}
- [x] 7.1 Build RelWithDebInfo and Debug with no new warnings compared with the warning baseline in master plan `## Baseline Data`. Both builds succeeded on 2026-10-03 with zero compiler warnings (`build/verify-Debug.log`, `build/verify-RelWithDebInfo.log`).
- [x] 7.2 Run `ctest`; all baseline tests pass, including the `WindowDesc` tests. Existing Debug evidence: `build/ctest-Debug.log`, 106/106 passed, including both `WindowDesc` cases. No runtime checks are inferred from this result.
- [x] 7.3 In a Debug build, cycle the layer in a new game and in the scenario editor; confirm no assert fires and the button stays next to the map button.

  Verified 2026-10-04: `N` cycles the new game's main view through Surface, Underground, Deep Underground and Space. The editor dropdown selects all four layers in that display order. Its Layer button follows Small map in the full, no-title-panel and both split arrangements (client widths 1750, 1400 and 728 pixels at 250%). The editor hotkey remains unbound. Evidence: [scenario-layer-review.png](assets/reviews/scenario-layer-review.png), `build/toolbar-review/editor-{full,no-title,split-upper,split-lower}.png`, `game.log` and `editor.log`; both game runs exit normally with no assertion.
- [x] 7.4 Save and reload a game while viewing Underground; confirm the game loads and the view starts on Surface (the layer is not saved, AD-7). Group 7B fixes the camera band and verifies this for all three non-surface layers.
- [x] 7.5 Grep `src/` for `->layer` and `.layer` uses added by this plan; confirm none is reachable from command or tick code (I-4). GUI reads are in the view helpers, viewport initialisation, toolbar icon/tooltip and, following Group 7B, `SaveViewportBeforeSaveGame` to normalise saved camera coordinates. The latter serialises GUI metadata only; it does not select any map data or simulation behaviour.

### Area Mandatory Reading - Saved camera position

_Applies to Group 7B._

| File name | Authoritative symbols | Purpose |
| --- | --- | --- |
| [r2mlm-multi-layered-map.md](r2mlm-multi-layered-map.md) | AD-2, AD-7 | World coordinates include the layer band; viewport layer is client state and does not persist in a save. |
| [src/saveload/misc_sl.cpp](../../../../src/saveload/misc_sl.cpp) | `SaveViewportBeforeSaveGame`, `ResetViewportAfterLoadGame` | The VIEW chunk keeps camera coordinates but reload creates a Surface viewport. |
| [src/map_layer_gui.cpp](../../../../src/map_layer_gui.cpp) | `SetViewportLayer` | Authoritative projection of a layer's world-Y offset into scroll coordinates. |
| [docs/zz-reach2/architecture/savegame/archi-savegame.md](../../architecture/savegame/archi-savegame.md) | `## Invariants a change must not break` | Preserve existing chunk fields and old-save loading; the correction affects GUI coordinates only. |

Context has compacted 1 times while executing this group.

### Group 7B - Preserve plane camera position across save/load

Added 2026-10-04 from task 7.4's Debug run. Saving on Underground stores its stacked camera offset, then loading creates a Surface viewport and clamps those coordinates near the map edge. The layer correctly resets, but the intended plane position is lost. Store VIEW coordinates projected into the Surface band without saving a layer field.

{{SIMPLE}}
- [x] 7B.1 Correct `SaveViewportBeforeSaveGame` to subtract the current layer's projected band offset; retain the headless sentinel and zoom handling.
- [x] 7B.2 Build both configurations and run appropriate checks. Save paused games from each non-surface layer, reload on Surface and confirm the same plane position; compare all map-plane digests across a paused round trip.
- [x] 7B.3 Record captures and logs, then reconcile task 7.4 and the master plan. The VIEW schema and savegame version remain unchanged.

  Verified 2026-10-04: both configurations build with zero compiler warnings and pass 110/110 unit cases (`build/build-camera-save-{Debug,RelWithDebInfo}.log`, `build/ctest-camera-save-{Debug,RelWithDebInfo}.log`). Actual Debug saves from Underground, Deep Underground and Space reload onto Surface at the intended plane position. All three store the same VIEW record `fffffd2800002fe801` (x=-728, y=12264, zoom=1). Each paused load/save round trip preserves all ten map chunks byte for byte across four 256x256 planes; results are in `build/toolbar-review/camera-roundtrip-results.txt`. The portal/station fixture also preserves all ten chunks (`portals-roundtrip-a.txt`, `portals-roundtrip-b.txt`).

  Evidence: [camera-save-review.png](assets/reviews/camera-save-review.png), original `build/toolbar-review/camera-{underground,deep,space}-{saving,loaded}.png`, fixtures `build/save/camera-<layer>-{repaired,roundtrip}.sav`, `build/save/portals-camera-roundtrip-{a,b}.sav`, and `build/toolbar-review/camera-fixed.log`. Existing VIEW fields and version 370 are retained. Surface saves are unchanged by the zero-band subtraction; previously written non-surface camera offsets are not retroactively repaired. The original `makinpr2.sav` is preserved.

## Consult on Demand

| Trigger | File name | Start at | Expand only when | Consultation purpose |
| --- | --- | --- | --- | --- |
| The toolbar shows a wrong tooltip on any button after task 3.2 | [docs/eints.md](../../../../docs/eints.md) | `### I want to add a new string.` (lines 70-73) | A translator tool or CI check complains about the `###length` block | Confirm that `###length` is a translation-tool marker and that translations must not be edited; the offset arithmetic in `MakeMainToolbar` stays the runtime authority. |
| A split toolbar row overflows or a button disappears in task 4.5 | [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | `NWidgetToolbarContainer::SetupSmallestSize` and `AssignSizePosition` | The overflow persists with balanced arrays | Understand how `arrangeable_count` and spacers drive layout. |
| `SetSprite` on the nested widget does not repaint | [docs/zz-reach2/architecture/gui/archi-gui.md](../../architecture/gui/archi-gui.md) | `## Widget tables` (lines 32-50) | The widget stays stale after `SetWidgetDirty` | Check how nested widgets store sprite data and dirty state. |
| A hotkey key choice collides with an existing binding | [src/hotkeys.cpp](../../../../src/hotkeys.cpp) | `HotkeyList` registration | Two lists claim the same key in one window context | Decide whether the key is safe; leave the default unbound if unsure. |

## Planning Provenance

This table records what shaped the plan. It is not an execution reading queue, it must not be preloaded after context compaction, it is non-transitive, and its line ranges record planning use rather than runtime authority.

| File name | Sections or Line Ranges used while planning | Planning contribution |
| --- | --- | --- |
| [src/toolbar_gui.cpp](../../../../src/toolbar_gui.cpp) | 425-480, 1340-1373, 1522-1842, 1964-2261, 2279-2605 | Showed the four parallel arrays and two arrangement sets that every new toolbar button must touch, which set the Group 3 to Group 6 split. |
| [src/lang/english.txt](../../../../src/lang/english.txt) | 384-433 | The offset-indexed tooltip block and `###length` rule behind task 3.2. |
| [src/toolbar_gui.h](../../../../src/toolbar_gui.h) | 16-57 | Hotkey enum layout for task 5.4. |
| [docs/zz-reach2/architecture/gui/archi-gui.md](../../architecture/gui/archi-gui.md) | 52-138 | Event-hook surface (`OnClick`, `OnInvalidateData`, `OnTooltip`) used to place the icon refresh outside `OnPaint`. |
| [src/table/sprites.h](../../../../src/table/sprites.h) | 1248-1272, 1533 | Existing GUI sprites chosen as stand-in icons. |

## Completion Conditions

- `MapLayer` and `MAX_MAP_LAYERS` exist in `src/map_layer_type.h` exactly as AD-1 states.
- `Viewport::layer` exists, defaults to Surface, and is never read by command or tick code.
- The main toolbar and the scenario editor toolbar show the layer button immediately right of the map button at every toolbar width.
- The interim icon-only cycle is superseded by R2MLM5 and AD-1/AD-7 as amended by R2MLM12: the button selects a layer from the four-layer dropdown, and `N` cycles the main view in display order. Group 5 records the skipped historical operator check; Group 7 verifies the final controls.
- Both configurations build without new warnings and all baseline tests pass.
